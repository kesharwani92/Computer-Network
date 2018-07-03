#include <sys/socket.h>
#include <arpa/inet.h>
#include <common.h>
#include <exception>


#ifndef HW6_MYUDP_H
#define HW6_MYUDP_H

typedef uint16_t port_t;

// Convert argument string to port_t
inline port_t cstr_to_port(const char* cstr) {
	return strtoul(cstr, nullptr, 0);
}

// Set up UDP configuration
void set_udp_addr(struct sockaddr_in& addr, port_t port) {
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);
}

// Standard procedure for UDP
// 1. Register file decriptors
// 2. Bind port (note that a port can only bind a process)
// 3. Thorough error handling
void init_udp(int& fd, struct sockaddr_in& myaddr) {
  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    std::cerr << "error: cannot create socket" << std::endl;
    throw std::exception();
  }
  
  int option = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) == -1) {
    std::cerr << "error: setsocketopt failed" << std::endl;
    exit(1);
  }
  if (bind(fd, (struct sockaddr*)&myaddr, sizeof(myaddr)) < 0) {
    std::cerr << "error: bind failed" << std::endl;
    std::cerr << "error: " << strerror(errno) << std::endl;
    throw std::exception();
  }
}

inline void udpsend(int fd, struct sockaddr_in& dest, const std::string& msg) {
  if (sendto(fd, msg.c_str(), msg.size(), 0, (struct sockaddr *)&dest,
      sizeof(dest)) < 0) {
    MY_ERROR_STREAM << "error: failed while sending " << msg << std::endl;
    MY_ERROR_STREAM << "error: " << strerror(errno) << std::endl;
    throw std::exception();
  }
}

#endif