/*******************************************************************************
* UdpChat.h
* 
* Defines structs and classes for the simple chat application.
*
* Author: Yan-Song Chen
* Date  : Jun 4th, 2018
*******************************************************************************/
#include <unordered_map>
#include <string>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

typedef struct {
  unsigned long ip;
  unsigned short port;
  bool on;
} table_row_t;

std::ostream& operator<<(std::ostream& os, const table_row_t& r) {
  os << r.ip << ':' << r.port;
  if (r.on) os << " (on)";
  else os << " (off)";
  return os;
}

typedef std::unordered_map<std::string,table_row_t> table_t;

std::ostream& operator<<(std::ostream& os, const table_t& t) {
  for (const std::pair<std::string,table_row_t>& r : t)
    os << r.first << ' ' << r.second << std::endl;
  return os;
}

class UdpSocket {
public:
  UdpSocket(unsigned short port) {
    memset(&myaddr_, 0, sizeof(myaddr_));
    myaddr_.sin_family = AF_INET;
    myaddr_.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr_.sin_port = htons(port);

    if (bind(fd_, (struct sockaddr*)&myaddr_, sizeof(myaddr_))<0) {
      throw "error: bind failed";
    }
  }

  void SetServer(char* servip, unsigned short servport,
                 unsigned short clntport) {
    memset((char*)&servaddr_, 0, sizeof(servaddr_));
    servaddr_.sin_family = AF_INET;
    servaddr_.sin_port = htons(servport);
    if (inet_aton(servip,&servaddr_.sin_addr) < 0) {
      throw "error: invalid server ip";
    };
  }

  std::string Listen() {
      size_t recvlen = recvfrom(fd_, buf_, BUFSIZE, 0,
                                (struct sockaddr*)&clntaddr_, &addrlen_);
      return std::string((char*) buf_, recvlen);
  }
private:
  static const size_t BUFSIZE = 2048;
  int fd_;
  struct sockaddr_in myaddr_;
  struct sockaddr_in servaddr_; // server address, optional
  struct sockaddr_in clntaddr_; // stores sender's address
  socklen_t addrlen_ = sizeof(clntaddr_);
  unsigned char buf_[BUFSIZE];
  table_t table_;
};
