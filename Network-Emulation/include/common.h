#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <cstring>
#include <iostream>
#include <exception>
#include <arpa/inet.h>


// Print specified time format
inline void printmytime() {
	struct timeval timestamp;
	if (gettimeofday(&timestamp, nullptr) == -1) {
		std::cerr << "error: gettimeofday() failed\n" << strerror(errno)
        << std::endl;
		exit(1);
	}
  timestamp.tv_usec = timestamp.tv_usec  % 1000; // us to ms
	std::cout << '[' << timestamp.tv_sec << '.' << timestamp.tv_usec << "] ";
}

typedef uint16_t port_t;
inline port_t cstr_to_port(char* cstr) { return strtoul(cstr, nullptr, 0);}

// Set up UDP configuration
void set_udp_addr(struct sockaddr_in& addr, port_t port) {
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);
}

// Standard procedure for UDP
// 1. Register file decriptors
// 2. Bind port
// 3. Thorough error handling
void initialize(int& fd, struct sockaddr_in& myaddr) {
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