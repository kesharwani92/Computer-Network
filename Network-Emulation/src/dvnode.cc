/*******************************************************************************
* dvnnode.cc
* 
* Distance Vector Algorithm.
*
* Author: Yan-Song Chen
* Date  : Jul 2nd, 2018
*******************************************************************************/
#include <dvnode.h>
#include <thread>
#include <vector>

// Sends out message to all neighbors
void __broadcast(int fd, const std::vector<struct sockaddr_in>& neighbors,
    std::string msg) {
  for (auto neighbor : neighbors) {
    udpsend(fd, neighbor, msg);
  }
}

// Print out the routing table according to specification of PA2
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
  // Parse arguments and initialize per-process variables
  dv_t myvec;
  std::unordered_map<port_t, port_t> myhop;
  std::unordered_map<port_t, dv_t> memo;
  std::vector<struct sockaddr_in> neighbors;

  port_t myport = cstr_to_port(argv[1]);
  bool activated = false;   // A node publishes its vector on activation
  myvec[myport] = 0;        // Dummy entry for Bellman-Ford update
  myhop[myport] = myport;   // Another dummy entry

  for (int i = 2; i < argc; i+=2) {
    if (std::string(argv[i]) == "last"){
      activated = true;
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

  // The "last" node will shoot the first distance vector and activate the
  // network. The broadcast process is threaded so that the listening socket
  // won't miss incoming messages.
  if (activated) {
    std::string outmsg = std::to_string(myport) + entryDelim + dv_out(myvec);
    std::thread kickoff(__broadcast, fd, neighbors, outmsg);
    kickoff.detach();
  }

  // Use the socket to receive distance vectors
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
    
    // If the node has never broadcast its distance vector, it will do broadcast
    // right here. Otherwise if the Bellman-Ford equation updates the distance
    // vector, broadcast the new vector too.
    if (!activated || bellman_ford_update(myvec, myhop, memo)) {
      last = true;
      std::string outmsg = std::to_string(myport) + entryDelim + dv_out(myvec);
      __print_table(myport, myvec, myhop);
      std::thread kickoff(__broadcast, fd, neighbors, outmsg);
      kickoff.detach();
    }

  }
  return 0;
}