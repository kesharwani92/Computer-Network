#include <common.h>
#include <thread>
#include <atomic>
#include <string>
#include <queue>
#define GBNNODE_DEBUG 1 // Debugging mode

// Shared memory accross threads
bool proc_running = true;

std::atomic_flag msglock = ATOMIC_FLAG_INIT; // lock for "messages"
std::queue<std::string> messages;

std::atomic_flag acklock = ATOMIC_FLAG_INIT; // lock for ACKs
size_t ack;

int fd;
struct sockaddr_in src, dest;

inline void __grab_lock(std::atomic_flag& lock) {
  while (lock.test_and_set(std::memory_order_acquire));
}

inline void __release_lock(std::atomic_flag& lock) {
  lock.clear(std::memory_order_release);
}

inline void __send(const std::string& msg) {
  if (sendto(fd, msg.c_str(), msg.size(), 0, (struct sockaddr *)&dest,
      sizeof(dest)) < 0) {
    std::cerr << "error: failed while sending " << msg << std::endl;
    std::cerr << "error: " << strerror(errno) << std::endl;
    throw std::exception();
  }
}
// Multi-threaded functions

// Takes in command line input and process command type
void cin_proc() {
  while (proc_running) {
    std::string line;
    std::getline(std::cin, line);

    size_t delim = line.find(' ');
    if (line.substr(0, delim) != "send") {
      #if GBNNODE_DEBUG
      std::cerr << "warning: support only send operation" << std::endl;
      #endif
      continue;
    }

    std::string message = line.substr(delim+1, line.size()-delim);

    #if GBNNODE_DEBUG
    std::cout << "cin_proc: got " << message << std::endl;
    #endif
    __grab_lock(msglock);
    messages.push(message);
    __release_lock(msglock);
  }
}

#ifndef CHECK_TIMEOUT
#define CHECK_TIMEOUT(base, ms) if (std::chrono::high_resolution_clock::now()\
    - base < std::chrono::milliseconds(ms)) goto timeout_phase;
#endif
#ifndef RESET_TIMER
#define RESET_TIMER(base) base = std::chrono::high_resolution_clock::now();
#endif
// Pops out the top message and send it over UDP
void gbn_proc(const int& window) {
  while (proc_running) {
    if (messages.empty()) continue;
    __grab_lock(msglock);
    std::string buf = messages.front();
    #if GBNNODE_DEBUG
    printmytime();
    std::cout << "gbn_proc: send " << buf << std::endl;
    #endif
    messages.pop();
    __release_lock(msglock);
    
    // Finite State Machine
    size_t base = 0, nextseq = 0;
    // TODO: use chrono timer
    auto timebase = std::chrono::high_resolution_clock::now();

send_phase:
    CHECK_TIMEOUT(timebase,500);
    for (; nextseq < base + window && nextseq < buf.size(); nextseq++) {
      std::string msg = std::to_string(nextseq) + ":" + buf[nextseq];
      __send(msg);
    }

recvack_phase:
    CHECK_TIMEOUT(timebase,500);
    if (acklock.test_and_set(std::memory_order_acquire)) {
      base = ack + 1;
      if (base == nextseq) {
        RESET_TIMER(timebase);
        __release_lock(acklock);
        goto send_phase;
      } else if (base == buf.size()) {
        __release_lock(acklock);
        goto finish_phase;
      }
      __release_lock(acklock);
    }
    goto send_phase;

timeout_phase:
    for (size_t i = base; i < nextseq; i++) {
      std::string msg = std::to_string(nextseq) + ":" + buf[nextseq];
      __send(msg);
    }
    RESET_TIMER(timebase);
    goto send_phase;

finish_phase:
    std::string msg = "end";
    __send(msg);
  } // End of while (proc_running)
}

int main(int argc, char** argv) {
  #if GBNNODE_DEBUG
  std::cout << "Arguments" << "Window size = " << std::stoi(argv[1]) << std::endl
      << "Source port: " << cstr_to_port(argv[2]) << std::endl
      << "Destination port: " << cstr_to_port(argv[3]) << std::endl;
  #endif
  const int window = std::stoi(argv[1]);
  set_udp_addr(src, cstr_to_port(argv[2]));
  set_udp_addr(dest, cstr_to_port(argv[3]));
  initialize(fd, src);
  std::thread user_cin(cin_proc);
  std::thread send_gbn(gbn_proc, window);
  user_cin.join();
  send_gbn.join();
  close(fd);
  return 0;
}