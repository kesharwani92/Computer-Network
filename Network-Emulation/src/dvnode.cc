#include <dvnode.h>
#include <thread>
#include <vector>

void __broadcase(int fd, const std::vector<struct sockaddr_in>& neighbors,
    std::string msg) {
  //MY_INFO_STREAM << "Network kickoff" << std::endl;
  for (auto neighbor : neighbors) {
    udpsend(fd, neighbor, msg);
  }
}

inline void __print_table(port_t myport, dv_t& myvec,
    std::unordered_map<port_t, port_t>& myhop) {
  MY_INFO_STREAM << "Node " << myport << " Routing Table" << std::endl;
  for (auto it : myvec) {
    if (it.first == myport) continue;
    std::cout << " - (" <<  it.second << ") -> Node " << it.first;
    if (myhop[it.first] == it.first) {
      std::cout << std::endl;
    } else {
      std::cout << "; Next hop -> Node " << myhop[it.first] << std::endl;
    }
  }
}

int main(int argc, char** argv) {
  // Per-process variables
  dv_t myvec;
  std::unordered_map<port_t, port_t> myhop;
  std::unordered_map<port_t, dv_t> memo;
  std::vector<struct sockaddr_in> neighbors;

  // Parse arguments and initialze variables
  port_t myport = cstr_to_port(argv[1]);
  bool last = false;
  myvec[myport] = 0;        // Dummy entry for Bellman-Ford update
  myhop[myport] = myport;

  for (int i = 2; i < argc; i+=2) {
    if (std::string(argv[i]) == "last"){
      last = true;
      break;
    }
    port_t p = cstr_to_port(argv[i]);
    float f = std::stof(argv[i+1]);
    myvec[p] = f;
    myhop[p] = p;

    struct sockaddr_in n;
    set_udp_addr(n, p);
    neighbors.push_back(n);
  }

  // Setup socket-related variables
  int fd;
  struct sockaddr_in myaddr;
  set_udp_addr(myaddr, myport);
  init_udp(fd, myaddr);

  std::string outmsg = std::to_string(myport) + entryDelim + dv_out(myvec);
  if (last) {
    std::thread kickoff(__broadcase, fd, neighbors, outmsg);
    kickoff.detach();
  }

  // Listen and update DV table
  struct sockaddr_in rcvaddr;
  socklen_t addrlen = sizeof(rcvaddr);
  char buf[bufferSize];
  while (true) {
    // The alarm signal may wake up recvfrom with errno EINTR.
    // In this case, we just ignore the signal and keep listening.
    ssize_t ret;
    while ((ret = recvfrom(fd, buf, bufferSize, 0, (struct sockaddr*)&rcvaddr,
        &addrlen)) == -1 && errno == EINTR);
    buf[ret] = '\0';
    std::pair<port_t, dv_t> msg = dv_in(std::string(buf, ret));
    memo[msg.first] = msg.second;
    
    if (bellman_ford_update(myvec, myhop, memo) || !last) {
      last = true;
      std::string outmsg = std::to_string(myport) + entryDelim + dv_out(myvec);
      //MY_INFO_STREAM << "Broadcast new vector " << outmsg << std::endl;
      __print_table(myport, myvec, myhop);
      std::thread kickoff(__broadcase, fd, neighbors, outmsg);
      kickoff.detach();
    }

  }
  return 0;
}