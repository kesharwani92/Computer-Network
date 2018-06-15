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
#include <sstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <exception>


// Table struct that stores client information
typedef struct {
  std::string ip;
  uint16_t port;
  bool active;
} entry_t;

typedef std::unordered_map<std::string,entry_t> table_t;

std::ostream& operator<<(std::ostream& os, const entry_t& r) {
  os << r.ip << ':' << r.port;
  if (r.active) os << " (on)";
  else os << " (off)";
  return os;
}

std::ostream& operator<<(std::ostream& os, const table_t& t) {
  for (const std::pair<std::string,entry_t>& r : t)
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

// Type converting functions (sockets only accepts char*)
std::string __row_to_str(entry_t r) {
  std::stringstream ss;
  ss << r.ip << ':' << r.port << ':' << r.active;
  return ss.str();
}

entry_t __str_to_row(std::string s) {
  size_t pos;
  entry_t r;
  pos = s.find(':');
  r.ip = s.substr(0,pos);
  pos = s.find(':', pos+1);
  r.port = strtoul(s.substr(pos,pos+5).c_str(),nullptr,0);
  pos = s.find(':', pos+1);
  r.active = (s[pos+1] == '1');
  return r;
}

typedef std::pair<std::string, entry_t> table_pair_t;
table_pair_t __get_table_pair(std::string s) {
  size_t pos1 = s.find(':');
  size_t pos2 = s.find(':', pos1+1);
  size_t pos3 = s.find(':', pos2+1);
  table_pair_t res;
  res.first = s.substr(0, pos1);
  res.second.ip = s.substr(pos1+1, pos2-pos1-1);
  res.second.port = strtoul(s.substr(pos2+1,pos3-pos2-1).c_str(),nullptr,0);
  res.second.active = (s[pos3+1] == '1');
  return res;
}

// Udp socket wrapper
// Encapsulates C-style system calls and allows the main program to focus on
// higher-level tasks (e.g. protocal exchange).
class UdpSocket {
public:
  UdpSocket(uint16_t port) {
    if ((fd_ = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
      std::cerr << "error: cannot create socket" << std::endl;
      throw std::exception();
    }
    memset(&myaddr_, 0, sizeof(myaddr_));
    myaddr_.sin_family = AF_INET;
    myaddr_.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr_.sin_port = htons(port);
    int option = 1;
    if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) == -1) {
      std::cerr << "error: setsocketopt failed" << std::endl;
      exit(1);
    }
    if (bind(fd_, (struct sockaddr*)&myaddr_, sizeof(myaddr_))<0) {
      std::cerr << "error: bind failed" << std::endl;
      std::cerr << "error: " << strerror(errno) << std::endl;
      throw std::exception();
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
      std::cerr << "error: invalid ip" << std::endl;
      throw std::exception();
    };
    if (sendto(fd_, msg.c_str(), msg.size(), 0, (struct sockaddr *)&clntaddr_,
        sizeof(clntaddr_)) < 0) {
      std::cerr << "error: sendto failed" << std::endl;
      throw std::exception();
    }
  }

  bool Listen(udpmsg_t& msg, time_t sec = 0, suseconds_t usec = 0) {
    struct timeval tv;
    tv.tv_sec = sec;
    tv.tv_usec = usec;
    if (setsockopt(fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
      std::cerr << "error: setsockopt failed" << std::endl;
      std::cerr << "error: " << strerror(errno) << std::endl;
      throw std::exception();
    }

    ssize_t ret = recvfrom(fd_, buf_, BUFSIZE, 0,
                          (struct sockaddr*)&clntaddr_, &addrlen_);
    if (ret > 0) {
      buf_[ret] = 0;
      msg = {std::string(inet_ntoa(clntaddr_.sin_addr)),
             ntohs(clntaddr_.sin_port),
             std::string((char*) buf_, ret)};
      return true;
    } else if (errno == EAGAIN || errno == EWOULDBLOCK ) {// Timeout
      //std::cout << "warning: timeout!!" << std::endl;
      return false;
    } else {
      std::cerr << "error: recvfrom failed" << std::endl;
      std::cerr << "error: " <<  strerror(errno);
      return false;
    }
  }

private:
  static const size_t BUFSIZE = 2048;
  int fd_;
  struct sockaddr_in myaddr_;
  struct sockaddr_in clntaddr_; // Target addr (talker), sender addr (listener)
  socklen_t addrlen_ = sizeof(clntaddr_);
  unsigned char buf_[BUFSIZE];
};
