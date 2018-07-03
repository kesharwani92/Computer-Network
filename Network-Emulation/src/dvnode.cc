#include <dvnode.h>
#include <thread>
#include <vector>

void __broadcase(int fd, const std::vector<struct sockaddr_in>& neighbors,
    std::string msg) {
  MY_INFO_STREAM << "Network kickoff" << std::endl;
  for (auto neighbor : neighbors) {
    udpsend(fd, neighbor, msg);
  }
}

int main(int argc, char** argv) {
  //MY_INFO_STREAM << "dvnode program" << std::endl;
  // Parse arguments
  port_t myport = cstr_to_port(argv[1]);
  DVTable t(myport);
  dv_t v;
  bool last = false;
  std::vector<struct sockaddr_in> neighbors;
  for (int i = 2; i < argc; i+=2) {
    if (std::string(argv[i]) == "last"){
      last = true;
      break;
    }
    port_t p = cstr_to_port(argv[i]);
    v[p] = std::stof(argv[i+1]);

    struct sockaddr_in n;
    set_udp_addr(n, p);
    neighbors.push_back(n);
  }

  int fd;
  struct sockaddr_in myaddr;
  set_udp_addr(myaddr, myport);
  init_udp(fd, myaddr);

  t.Insert(myport, v);
  MY_INFO_STREAM << "My table\n" << t.Str() << std::flush;

  if (last) {
    std::thread kickoff(__broadcase, fd, neighbors, t.Str());
    kickoff.detach();
  }

  // Listen and update DV table
  MY_INFO_STREAM << "listen and update DV table" << std::endl;
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
    MY_INFO_STREAM << "receive vector" << std::endl << buf << std::flush;
  }
  return 0;
}