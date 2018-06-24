#include <common.h>
#include <signal.h>
#include <thread>

#include <string>
#include <queue>

#define GBNNODE_DEBUG 0 // Debugging mode

// Shared memory accross threads
bool proc_running = true;
int window;
std::atomic_flag msglock = ATOMIC_FLAG_INIT; // lock for "messages"
std::queue<std::string> messages;

static int fd;
static struct sockaddr_in src, dest;

std::atomic_flag acklock = ATOMIC_FLAG_INIT; // lock for ACKs
size_t ack = 0;

// The user input thread
// Takes in command line input and process command type
void cin_proc() {
  while (proc_running) {
    std::string line;
    std::getline(std::cin, line);

    size_t delim = line.find(' ');
    if (line.substr(0, delim) != "send") {
      MY_ERROR_STREAM << "warning: support only send operation" << std::endl;
      continue;
    }
    std::string message = line.substr(delim+1, line.size()-delim);
    grab_lock(msglock);
    messages.push(message);
    release_lock(msglock);
  }
}

// Packet-dropping manager
static bool dropmode; // 1 for deterministic, 0 for probabilistic
static int dropsum;
static int dropcount;
static int dropn;
static float dropprob;
inline float rand_float() {
  return static_cast<float>(rand()) / RAND_MAX;
}
bool drop_packet() {
  return (dropmode) ? (++dropsum % dropn) == 0 : rand_float() < dropprob;
}

// The go-back-n protocol thread
void __timeout_handler(int signo);
void __gbn_fsm(const std::string& buf);
void gbn_proc() {
  struct sigaction sa;
  sa.sa_handler = &__timeout_handler;
  sigaction (SIGALRM, &sa, 0);
  while (proc_running) {
    if (messages.empty())
      continue;

    grab_lock(msglock);
    std::string buf = messages.front();
    messages.pop();
    release_lock(msglock);

    __gbn_fsm(buf);
  }
}

static bool timeout_flag;
void __timeout_handler(int sig) {
  MY_INFO_STREAM << "timeout" << std::endl;
  timeout_flag = true;
}

// Implementation of GBN Finite State Machine
void __gbn_fsm(const std::string& buf) {
  size_t base = 0, nextseq = 0;
  timeout_flag = false;
  ualarm(500000, 0);

send_state:
  if (timeout_flag)
    goto timeout_state;
  for (; nextseq < base + window -1 && nextseq < buf.size(); nextseq++) {
    std::string msg = "SEQ:" + std::to_string(nextseq) + ":" + buf[nextseq];
    udpsend(fd, dest, msg);
    MY_INFO_STREAM << "packet" << nextseq << ' ' << buf[nextseq] << " sent"
        << std::endl;
  }

chkack_state:
  if (timeout_flag)
    goto timeout_state;
  grab_lock(acklock);
  base = ack + 1;
  if (base == buf.size()) {
    ack = 0;
    release_lock(acklock);
    ualarm(0, 0); // on Linux, setting zero will cancel the alarm
    goto finish_fsm;
  } else if (base == nextseq) {
    release_lock(acklock);
    ualarm(500000, 0);
    goto send_state;
  }
  release_lock(acklock);
  goto send_state;

timeout_state:
  for (size_t i = base; i < nextseq && i < buf.size(); i++) {
    std::string msg = "SEQ:" + std::to_string(i) + ":" + buf[i];
    udpsend(fd, dest, msg);
    MY_INFO_STREAM << "packet" << i << ' ' << buf[i] << " sent" << std::endl;
  }
  ualarm(500000, 0);
  timeout_flag = false;
  goto chkack_state;

  // Shouldn't go to here anyway
  #if GBNNODE_DEBUG
  MY_ERROR_STREAM << "__gbn_fsm: something went wrong... shouldn't be here"
      << std::endl;
  #endif
  ualarm(0, 0); 
  proc_running = false;
  return;

finish_fsm:
  std::string msg = "END";
  udpsend(fd, dest, msg);
  std::cout << "[Summary] " << "packets dropped, loss rate = ??"
      << std::endl;
}


// The main thread listens to the bound port
void recv_proc() {
  int expected = 0;
  struct sockaddr_in rcvaddr;
  socklen_t addrlen = sizeof(rcvaddr);
  char buf[BUFSIZE];
  ack = 0;
  while (proc_running) {
    ssize_t ret;
    while ((ret = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr*)&rcvaddr,
        &addrlen)) == -1 && errno == EINTR);
    if (ret > 0) {
      #if GBNNODE_DEBUG
      std::cout << "recv_proc: receive message - " << buf << std::endl;
      #endif

      buf[ret] = 0;
      std::string inmsg(buf, ret);
      size_t pos0 = inmsg.find(':');
      if (inmsg.substr(0, pos0) == "ACK") {
        size_t pos1 = inmsg.find(':', pos0+1);
        int newack = stoi(inmsg.substr(pos0+1, pos1-pos0-1));
        if (drop_packet()) {
          dropcount++;
          MY_INFO_STREAM << "ACK" << newack << " discarded" << std::endl;
        } else {
          MY_INFO_STREAM << "ACK" << newack << " received, window moved to "
              << newack+1 << std::endl;
          grab_lock(acklock);
          ack = newack;
          release_lock(acklock);
        }
      } else if (inmsg.substr(0, pos0) == "SEQ"){
        size_t pos1 = inmsg.find(':', pos0+1);
        int seq = stoi(inmsg.substr(pos0+1, pos1-pos0-1));
        if (drop_packet()) {
          dropcount++;
          MY_INFO_STREAM << "packet" << seq << ' ' << inmsg[pos1+1]
              << " discarded" << std::endl;
        } else if (seq == expected) {
          MY_INFO_STREAM << "packet " << seq << ' ' << inmsg[pos1+1]
              << " received" << std::endl;
          std::string outmsg = "ACK:" + std::to_string(expected);
          udpsend(fd, dest, outmsg);
          MY_INFO_STREAM << "ACK" << expected << " sent, expecting " 
              << expected+1 << std::endl;
          expected++;
        } else {
          MY_INFO_STREAM << "packet " << seq << ' ' << inmsg[pos1+1]
              << " received" << std::endl;
          std::string outmsg = "ACK:" + std::to_string(expected-1);
          udpsend(fd, dest, outmsg);
          MY_INFO_STREAM << "ACK" << expected-1 << " sent, expecting "
              << expected << std::endl;
        }

      } else if (inmsg == "END") {
        expected = 0;
        grab_lock(acklock);
        ack = 0;
        release_lock(acklock);
        std::cout << "[Summary] " << "? / ?"
            << "packets discarded, loss rate = "
            << "??" << std::endl;
      }
    } else {
      MY_ERROR_STREAM << "error: recvfrom failed" << std::endl;
      MY_ERROR_STREAM << "error: " <<  strerror(errno) << std::endl;
      throw std::exception();
    }
  }
}

int main(int argc, char** argv) {
  // Convert arguments to global variables
  window = std::stoi(argv[1]);
  set_udp_addr(src, cstr_to_port(argv[2]));
  set_udp_addr(dest, cstr_to_port(argv[3]));

  if (std::string(argv[4]) == "-d") {  
    dropmode = true;
    dropn = std::stoi(argv[5]);
  } else if (std::string(argv[4]) == "-p") {
    dropmode = false;
    dropprob = std::stof(argv[5]);
  } else {
    MY_ERROR_STREAM << "error: invalid drop mode" << std::endl;
    exit(1);
  }

  initialize(fd, src);
  std::thread user_cin(cin_proc);
  std::thread send_gbn(gbn_proc);
  recv_proc();
  user_cin.join();
  send_gbn.join();
  close(fd);
  return 0;
}