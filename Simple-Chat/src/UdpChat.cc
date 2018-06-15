/*******************************************************************************
* UdpChat.cc
* 
* Simple chat application.
*
* Author: Yan-Song Chen
* Date  : Jun 6th, 2018
*******************************************************************************/
#include <UdpChat.h>
#include <thread>

// Shared memory
bool proc_running = true;
table_t usertable;

void client_listen(uint16_t port) {
  UdpSocket udp(port);
  udpmsg_t msg;
  while (proc_running) {
    if (!udp.Listen(msg) || msg.msg.empty()) continue;
    //std::cout << "receive msg: " << msg.msg << std::endl;
    size_t pos0 = msg.msg.find(':');
    if (msg.msg.substr(0, pos0) == "TABLE") {
      table_pair_t p = __get_table_pair(msg.msg.substr(pos0+1,
                                                       msg.msg.size()));
      usertable[p.first] = p.second;
      std::cout << "Update table" << std::endl;
      std::cout << usertable;
    }
  }
  std::cout << "client_listen finished cleanly" << std::endl;
}

void client_talk(uint16_t port, entry_t servaddr) {
  UdpSocket udp(port);
  while (proc_running) {
    std::string msg;
    std::cout << ">>> " << std::flush;
    getline(std::cin, msg);
    if (msg.size() > 0)
	    udp.SendTo(servaddr.ip.c_str(), servaddr.port, msg);
  }
  std::cout << "client_talk finished cleanly" << std::endl;
}

int main(int argc, char** argv) {
  /****************************************************************************
  ** Server side
  *****************************************************************************/
  if (std::string(argv[1]) == "-s") {
    if (argc < 2) {
      throw "error: too few arguments";
    }

    // Initialize udp socket
    uint16_t port = strtoul(argv[2], nullptr, 0);
    UdpSocket udp(port);
    std::cout << "Server mode" << std::endl;

    while (true) {
      udpmsg_t msg;
      udp.Listen(msg);
      std::cout << "Receive msg from " << msg.ip << ':';
      std::cout << msg.port << std::endl << msg.msg << std::endl;

      size_t delim = msg.msg.find(':');
      if (msg.msg.substr(0, delim) == "NEWUSER") {
        std::string newname(msg.msg.substr(delim+1, msg.msg.size()));

        if (usertable.find(newname) != usertable.end()) {
          std::cout << "warning: duplicate user name " << newname << std::endl;
          continue;
        }

        // ACK the new user
        std::cout << "ACK new user " << msg.msg << std::endl;
        entry_t newrow{msg.ip, msg.port, true};
        usertable.insert({newname, newrow});
	      udp.SendTo(msg.ip.c_str(), msg.port, "ACK");
        
        // 1. Broadcast the new user to all users
        // 2. Send a copy of table to the new user
        for (auto it = usertable.begin(); it != usertable.end(); it++) {
          std::cout << it->first << ":" << it->second << std::endl;
          udp.SendTo(it->second.ip.c_str(), it->second.port,
                     "TABLE:" + newname + ":" + __row_to_str(newrow));
	        udp.SendTo(msg.ip.c_str(), msg.port,
                     "TABLE:" + it->first + ":" + __row_to_str(it->second));
        }
      }
    }

  /****************************************************************************
  ** Client side
  *****************************************************************************/
  } else if (std::string(argv[1]) == "-c") {
    if (argc < 5) {
      throw "error: too few arguments";
    }
    // Initialize udp socket
    uint16_t myport = strtoul(argv[5], nullptr, 0);
    UdpSocket udp(myport);

    // Server address
    uint16_t servport = strtoul(argv[4], nullptr, 0);
    entry_t server{std::string(argv[3]), servport, true};

    std::string nickname(argv[2]);
    udp.SendTo(argv[3], servport, "NEWUSER:"+nickname);
    udpmsg_t msg;
    while (!udp.Listen(msg, 1, 0) || msg.msg != "ACK") {
      std::cout << "error: register failed. Retrying..." << std::endl;
    }
    usertable[nickname] = {std::string(argv[3]), myport, true};

    std::thread talker(client_talk, myport, server);
    std::thread listener(client_listen, myport);
    talker.join();
    listener.join();
  } else {
    std::cout << "warning: unknown argument " << argv[1] << std::endl;
    return 1;
  }
  return 0;
}
