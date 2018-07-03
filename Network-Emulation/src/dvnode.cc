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

std::pair<port_t, dv_t> __translate_msg(std::string s) {
  dv_t vec;
  size_t pos0 = 0;
  size_t pos1 = s.find('\n');
  port_t port = cstr_to_port(s.substr(pos0, pos1-pos0).c_str());
  pos0 = pos1+1;
  pos1 = s.find(':');
  size_t pos2 = s.find('\n', pos0);
  while (pos0 < s.size() && pos0 != std::string::npos &&
      pos1 != std::string::npos) {
    port_t p =  cstr_to_port(s.substr(pos0, pos1-pos0).c_str());
    float f = std::stof(s.substr(pos1+1, pos2));
    //std::cout << p << "----" << f << std::endl;
    pos0 = pos2 + 1;
    pos1 = s.find(':', pos0);
    pos2 = s.find('\n', pos0);
    vec[p] = f;
  }
  return {port, vec};
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

  if (last) {
    std::thread kickoff(__broadcase, fd, neighbors, t.DVStr());
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
    //MY_INFO_STREAM << "receive vector" << std::endl << buf << std::flush;

    std::pair<port_t, dv_t> msg = __translate_msg(std::string(buf, ret));
    t.Insert(msg.first, msg.second);
    MY_INFO_STREAM << "update table: " << std::endl << t;
  }
  return 0;
}