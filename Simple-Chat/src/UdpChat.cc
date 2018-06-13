/*******************************************************************************
* UdpChat.cc
* 
* Simple chat application.
*
* Author: Yan-Song Chen
* Date  : Jun 6th, 2018
*******************************************************************************/
#include <UdpChat.h>
#include <sys/mman.h>
#include <sstream>
std::string __row_to_str(table_row_t r) {
  std::stringstream ss;
  ss << r.ip << ':' << r.port << ':' << r.on;
  return ss.str();
}

table_row_t __str_to_row(std::string s) {
  size_t pos;
  table_row_t r;
  pos = s.find(':');
  r.ip = s.substr(0,pos);
  pos = s.find(':', pos+1);
  r.port = strtoul(s.substr(pos,pos+5).c_str(),nullptr,0);
  pos = s.find(':', pos+1);
  r.on = (s[pos+1] == '1');
  return r;
}

typedef std::pair<std::string, table_row_t> table_pair_t;

table_pair_t __get_table_pair(std::string s) {
  size_t pos1 = s.find(':');
  size_t pos2 = s.find(':', pos1+1);
  size_t pos3 = s.find(':', pos2+1);
  table_pair_t res;
  res.first = s.substr(0, pos1);
  res.second.ip = s.substr(pos1+1, pos2-pos1-1);
  res.second.port = strtoul(s.substr(pos2+1,pos3-pos2-1).c_str(),nullptr,0);
  res.second.on = (s[pos3+1] == '1');
  return res;
}

const size_t BUFSIZE = 2048;
int main(int argc, char** argv) {
  /*****************************************************************************
  *  Server code starts from here
  *****************************************************************************/
  if (std::string(argv[1]) == "-s") {
    if (argc < 2) {
      throw "error: too few arguments";
    }
    uint16_t port = strtoul(argv[2], nullptr, 0);
    UdpSocket udp(port);
    std::cout << "Server mode" << std::endl;

    table_t usertable;

    while (true) {
      udpmsg_t msg = udp.Listen();
      std::cout << "Receive msg from " << msg.ip << ':';
      std::cout << msg.port << std::endl << msg.msg << std::endl;

      size_t delim = msg.msg.find(':');
      if (msg.msg.substr(0, delim) == "NEWUSER") {
        std::cout << "Register new user " << msg.msg << std::endl;
        std::string newname(msg.msg.substr(delim+1, msg.msg.size()));
	table_row_t newrow{msg.ip, msg.port, true};
        usertable.insert({newname, newrow});
        std::cout << "Update usertable:" << std::endl << usertable << std::endl;
	      udp.SendTo(msg.ip.c_str(), msg.port, "ACK");
        // TODO: maybe request ACK from client?
        
        for (auto it = usertable.begin(); it != usertable.end(); it++) {
          udp.SendTo(it->second.ip.c_str(), it->second.port,
                     "TABLE:" + newname + ":" + __row_to_str(newrow));
	        udp.SendTo(msg.ip.c_str(), msg.port,
                     "TABLE:" + it->first + ":" + __row_to_str(it->second));
        }
      }
    }

  /*****************************************************************************
  *  Client code starts from here
  *****************************************************************************/
  } else if (std::string(argv[1]) == "-c") {
    if (argc < 5) {
      throw "error: too few arguments";
    }
    uint16_t myport = strtoul(argv[5], nullptr, 0);
    UdpSocket udp(myport);
    std::cout << "Client mode" << std::endl;

    // Create a new nickname on the server
    std::string nickname(argv[2]);
    uint16_t servport = strtoul(argv[4], nullptr, 0);

    udp.SendTo(argv[2], servport, "NEWUSER:"+nickname);
    if (udp.Listen().msg != "ACK") {
      std::cout << "error: register failed" << std::endl;
      return 1;
    }

    table_t* usertable = (table_t*) mmap(NULL, sizeof(table_t),
                                         PROT_READ|PROT_WRITE,
                                         MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    (*usertable) = table_t();
    pid_t pid = fork();

    // Listener: response to a udp message
    if (pid > 0) {
      while (true) {
        udpmsg_t msg = udp.Listen();
        size_t pos0 = msg.msg.find(':');
        if (msg.msg.substr(0, pos0) == "TABLE") {
          table_pair_t p = __get_table_pair(msg.msg.substr(pos0+1,
                                                           msg.msg.size()));
          (*usertable)[p.first] = p.second;
          std::cout << "New user: " << p.first << " " << p.second << std::endl;
        }
      }
    // Talker: take input from the user
    } else {
      while (true) {
        std::string msg;
        std::cout << ">>> " << std::flush;
        getline(std::cin, msg);
	      udp.SendTo(argv[3], servport, msg);
      }
    }
  } else {
    std::cout << "warning: unknown argument " << argv[1] << std::endl;
  }
  return 0;
}
