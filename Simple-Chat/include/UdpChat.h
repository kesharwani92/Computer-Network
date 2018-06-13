/*******************************************************************************
* UdpChat.h
* 
* Defines structs and classes for the simple chat application.
*
* Author: Yan-Song Chen
* Date  : Jun 6th, 2018
*******************************************************************************/
#include <unordered_map>
#include <string>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>


// Table struct that stores client information
typedef struct {
  std::string ip;
  uint16_t port;
  bool on;
} table_row_t;

typedef std::unordered_map<std::string,table_row_t> table_t;

std::ostream& operator<<(std::ostream& os, const table_row_t& r) {
  os << r.ip << ':' << r.port;
  if (r.on) os << " (on)";
  else os << " (off)";
  return os;
}

std::ostream& operator<<(std::ostream& os, const table_t& t) {
  for (const std::pair<std::string,table_row_t>& r : t)
    os << r.first << ' ' << r.second << std::endl;
  return os;
}

// Udp message type that will be exported by the socket wrapper. The ip and port
// number are converted to standard types when parsed to the main program.
typedef struct {
  std::string ip;
  uint16_t port;
  std::string msg;
} udpmsg_t;


// Udp socket wrapper
// Encapsulates C-style system calls and allows the main program to focus on
// higher-level tasks (e.g. protocal exchange).
class UdpSocket {
public:
  UdpSocket(uint16_t port) {
    if ((fd_ = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
      throw "error: cannot create socket";
    }
    memset(&myaddr_, 0, sizeof(myaddr_));
    myaddr_.sin_family = AF_INET;
    myaddr_.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr_.sin_port = htons(port);

    if (bind(fd_, (struct sockaddr*)&myaddr_, sizeof(myaddr_))<0) {
      throw "error: bind failed";
    }
  }

  ~UdpSocket() {
    close(fd_);
  }

  void SendTo(const char* ip, uint16_t port, const std::string& msg) {
    memset((char*)&clntaddr_, 0, sizeof(clntaddr_));
    clntaddr_.sin_family = AF_INET;
    clntaddr_.sin_port = htons(port);
    if (inet_aton(ip, &clntaddr_.sin_addr) < 0) {
      throw "error: invalid ip";
    };
    if (sendto(fd_, msg.c_str(), msg.size(), 0, (struct sockaddr *)&clntaddr_,
        sizeof(clntaddr_)) < 0) {
      throw "error: sendto failed";
    }
  }

  udpmsg_t Listen() {
      size_t recvlen = recvfrom(fd_, buf_, BUFSIZE, 0,
                                (struct sockaddr*)&clntaddr_, &addrlen_);
      buf_[recvlen] = 0;
      return udpmsg_t{std::string(inet_ntoa(clntaddr_.sin_addr)),
                      ntohs(clntaddr_.sin_port),
                      std::string((char*) buf_, recvlen)};
  }

private:
  static const size_t BUFSIZE = 2048;
  int fd_;
  struct sockaddr_in myaddr_;
  struct sockaddr_in clntaddr_; // Target addr (talker), sender addr (listener)
  socklen_t addrlen_ = sizeof(clntaddr_);
  unsigned char buf_[BUFSIZE];
};
