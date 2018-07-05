/*******************************************************************************
* cnnode.cc
* 
* Go-Back-N protocol.
*
* Author: Yan-Song Chen
* Date  : Jul 4th, 2018
*******************************************************************************/
#include <common.h>
#include <myudp.h>
#include <unordered_map>
#include <dvnode.h>
#include <thread>
#include <vector>
#include <math.h>

typedef struct {
  bool type;  // true: send, false: receive
  int sendcnt;
  int ackcnt;
  int rcvcnt;
  int dropcnt;
  float lossrate;
} record_t;

typedef std::unordered_map<port_t, struct sockaddr_in> addrmap_t;
typedef std::unordered_map<port_t, record_t> profile_t;

// Drop manager
dv_t droprate;
profile_t counter;
std::atomic_flag cntlock = ATOMIC_FLAG_INIT;

bool proc_running = true;
bool pauseprobe = true;
port_t myport;
int fd;

// unorder_map is not thread safe, hence need to use spinlock to protect
// from racing conditions
std::unordered_map<port_t, int> seq, ack;
std::atomic_flag acklock = ATOMIC_FLAG_INIT; 
std::atomic_flag seqlock = ATOMIC_FLAG_INIT;

addrmap_t edges;

void probe_proc(port_t dest,struct sockaddr_in addr) {
  std::string header = "PROBE:" + std::to_string(myport) + ':';
  while (proc_running) {
    if (pauseprobe)usleep(100000);
    grab_lock(seqlock);
    std::string outmsg = header + std::to_string(seq[dest]);
    release_lock(seqlock); // udpsend blocks the process, release lock first
    udpsend(fd, addr, outmsg);
    grab_lock(seqlock);
    seq[dest] = (seq[dest]+1)%10;
    release_lock(seqlock);
    grab_lock(cntlock);
    counter[dest].sendcnt++;
    release_lock(cntlock);
    usleep(10000);
  }
}

addrmap_t sendaddr, rcvaddr;
void __broadcast(const addrmap_t& neighbors,
    std::string msg) {
  for (auto neighbor : neighbors) {
    udpsend(fd, neighbor.second, msg);
  }
}

inline int __find_colon(char* buf, ssize_t n) {
  for (int i = 0; i < n; i++) {
    if (buf[i] == ':') return i;
  }
  return -1;
}

inline std::string __message_type(char*& buf, ssize_t& n) {
  for (int i = 0; i < n; i++) {
    if (buf[i] == ':') {
      std::string ret(buf, i);
      buf += (i+1);
      n -= (i+1);
      return ret;
    }
  }
  MY_ERROR_STREAM << "error: __message_type failed" << std::endl;
  throw std::exception();
}


//intmap_t rcvcnt, dropcnt, sendcnt, ackcnt;

void __take_or_drop_pck(char*& buf, ssize_t& n) {
  //MY_INFO_STREAM << "__take_or_drop_pck: " << buf << std::endl;
  int i = 0;
  for (; i < n; i++) {
    if (buf[i] == ':') break;
  }
  buf[i] = '\0';
  port_t p = cstr_to_port(buf);
  char data = buf[i+1];
  //MY_INFO_STREAM << "receive probing data: " << data << std::endl;
  auto addr = rcvaddr.find(p);
  if (addr == rcvaddr.end()) {
    MY_ERROR_STREAM << "warning: " << p << " not in rcvaddr" << std::endl;
    return;
  }
  grab_lock(cntlock);
  counter[p].rcvcnt++;
  release_lock(cntlock);
  if (rand_float() < droprate[p]) {
    grab_lock(cntlock);
    counter[p].dropcnt++;
    release_lock(cntlock);
  } else {
    std::string outmsg = "ACK:" + std::to_string(myport) + ':' + data;
    udpsend(fd, addr->second, outmsg);
  }
  return;
}

void __increment_ack(char*& buf, ssize_t n) {
  int i;
  if ((i = __find_colon(buf, n)) == -1) {
    MY_ERROR_STREAM << "error: __increment_ack failed" << std::endl;
    return;
  }
  buf[i] = '\0';
  port_t p = cstr_to_port(buf);
  int newack = std::stoi(buf + i + 1);
  //MY_INFO_STREAM << "new ack = " << newack << std::endl;
  grab_lock(acklock);
  ack[p] = (ack[p]+1)%10;
  release_lock(acklock);
  grab_lock(cntlock);
  counter[p].ackcnt++;
  release_lock(cntlock);
}

float __divide(int a, int b) {
  return roundf(static_cast<float>(a)/b*100)/100;
}

int main(int argc, char** argv) {
  // Parse arguments
  myport = cstr_to_port(argv[1]);
  //MY_INFO_STREAM << "myport = " << myport << std::endl;
  struct sockaddr_in myaddr;
  set_udp_addr(myaddr, myport);
  init_udp(fd, myaddr);


  int rcvarg = 0, sendarg = 0;
  for (int i = 1; i < argc; i++) {
    if (std::string(argv[i]) == "receive") {
      rcvarg = i;
    } else if (std::string(argv[i]) == "send") {
      sendarg = i;
    }
  }
  if (!rcvarg || !sendarg) {
    MY_ERROR_STREAM << "error: missing receive or send arguments" << std::endl;
    exit(1);
  } else {
    MY_INFO_STREAM << "rcvarg = " << rcvarg << ", sendarg = " << sendarg << std::endl;  
  }

  // Create receive list
  for (int i = rcvarg+1; i < sendarg; i+=2) {
    port_t p = cstr_to_port(argv[i]);
    float f = std::stof(argv[i+1]);
    droprate[p] = f;

    struct sockaddr_in s;
    set_udp_addr(s, p);
    rcvaddr[p] = s;
    counter[p] = {false, 0, 0, 0, 0};
  }
  // Create send list
  pauseprobe = true;
  for (int i = sendarg+1; i < argc; i++) {
    if (std::string(argv[i]) == "last") {
      break; // trigger dv broadcast
    }
    port_t p = cstr_to_port(argv[i]);
    struct sockaddr_in s;
    set_udp_addr(s, p);
    sendaddr[p] = s;

    grab_lock(acklock); grab_lock(seqlock);
    seq[p] = ack[p] = 0;
    release_lock(seqlock); release_lock(acklock);

    grab_lock(cntlock);
    //rcvcnt[p] = dropcnt[p] = sendcnt[p] = ackcnt[p] = 0;
    counter[p] = {true, 0, 0, 0, 0};
    release_lock(cntlock);

    std::thread probing(probe_proc, p, s);
    probing.detach();
  }

  
  int update_times = 0;
  pauseprobe = false;
  struct sockaddr_in otheraddr;
  socklen_t addrlen = sizeof(otheraddr);
  char buf[bufferSize];

probing:
  update_times = 0;
  timestamp_t last_dv_update = TIMESTAMP_NOW;
  while (true) {
    if (checktimeout(last_dv_update,1000)) {
      grab_lock(cntlock);
      //MY_ERROR_STREAM << "1 second up, update dv table" << std::endl;
      for (auto it : counter) {
        float lossrate;
        if (it.second.type) { // sender
          lossrate = __divide(it.second.sendcnt - it.second.ackcnt,
              it.second.sendcnt);
          MY_INFO_STREAM << "Link to " << it.first << ": " << it.second.sendcnt
            << " packets sent, " << it.second.sendcnt - it.second.ackcnt
            << " lost, loss rate = " << lossrate << std::endl;
        } else {
          lossrate = __divide(it.second.dropcnt, it.second.rcvcnt);
          MY_INFO_STREAM << "Link to " << it.first << ": " << it.second.rcvcnt
            << " packets sent, " << it.second.dropcnt
            << " lost, loss rate = " << lossrate << std::endl;
        }
        
      }
      release_lock(cntlock);
      last_dv_update = TIMESTAMP_NOW;
      if (++update_times == 5) {
        MY_INFO_STREAM << "5 second up, broadcast myvec" << std::endl;
        goto dv_update;
      }
    }
    ssize_t ret = recvfrom(fd, buf, bufferSize, 0, (struct sockaddr*)&otheraddr,
      &addrlen);
    buf[ret] = '\0';
    //MY_INFO_STREAM << buf << std::endl;
    char* bufcpy = buf;
    std::string msgtype = __message_type(bufcpy, ret);
    if (msgtype == "PROBE") {
      __take_or_drop_pck(bufcpy, ret);
    } else if (msgtype == "ACK") {
      __increment_ack(bufcpy, ret);
    } else if (msgtype == "DV") {
      goto dv_update;
    } else {
        MY_INFO_STREAM << "unimlemented message: " << buf << std::endl;
    }
  }
  
dv_update:
  MY_INFO_STREAM << "dv_update phase" << std::endl;
  last_dv_update = TIMESTAMP_NOW;
  while (true) {
    if (checktimeout(last_dv_update,1000)) {
      goto probing;
    }
  }
  return 0;
}