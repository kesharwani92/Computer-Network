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
#include <sys/time.h>
#include <fstream>

// Shared memory
bool proc_running = true;
table_t usertable;
std::string usercmd;
bool userevent = false;

void intepreter() {
  std::string msg;
  while (proc_running) {
    std::cout << ">>> " << std::flush;
    getline(std::cin, msg);
    //std::cout << "echo " << usercmd << std::endl;
    if (msg.empty()) continue;
    usercmd = msg;
    userevent = true;
  }
  std::cout << "client_talk finished cleanly" << std::endl;
}

void write_to_file(std::string filename, std::string msg) {
  std::cout << "write to " << filename << std::endl;
  std::cout << "content: " << msg << std::endl;
  std::ofstream file;
  file.open(filename,std::ios::out | std::ios::app);
  file << msg << std::endl;
  file.close();
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
      } else if (msg.msg.substr(0, delim) == "DEREG") {
        std::string target = msg.msg.substr(delim+1, msg.msg.size());
        if (usertable.find(target) == usertable.end()) {
          std::cout << "warning: " << target << " doesn't exist" << std::endl;
          continue;
        }
        usertable[target].active = false;
        for (auto it = usertable.begin(); it != usertable.end(); it++) {
          std::cout << it->first << ":" << it->second << std::endl;
          udp.SendTo(it->second.ip.c_str(), it->second.port,
                     "TABLE:" + target + ":" + __row_to_str(usertable[target]));
        }
        udp.SendTo(msg.ip.c_str(), msg.port, "ACK");
      } else if (msg.msg.substr(0, delim) == "REG") {
        std::string target = msg.msg.substr(delim+1, msg.msg.size());
        if (usertable.find(target) == usertable.end()) {
          std::cout << "warning: " << target << " doesn't exist" << std::endl;
          continue;
        }
        usertable[target].active = true;
        usertable[target].ip = msg.ip;
        usertable[target].port = msg.port;
        std::ifstream f((target+".log").c_str());
        if (f.good() && f.is_open()) {
          std::string line;
          udp.SendTo(usertable[target].ip.c_str(), usertable[target].port,
                     "ACK");
          while (std::getline(f, line)) {
            udp.SendTo(usertable[target].ip.c_str(), usertable[target].port,
                     "MSG:"+line);
          }
          f.close();
          if (remove((target+".log").c_str())) {
            std::cerr << "error: remove operation failed" << std::endl;
            std::cerr << strerror(errno) << std::endl;
          }
        } else {
          udp.SendTo(usertable[target].ip.c_str(), usertable[target].port,
                     "NAK");
        }
      } else if (msg.msg.substr(0, delim) == "OFFMSG") {
        size_t pos1 = msg.msg.find(':', delim+1);
        std::string target = msg.msg.substr(delim+1,pos1-delim-1);
        if (usertable.find(target) == usertable.end()) {
          std::cout << "warning: " << target << " doesn't exist" << std::endl;
          continue;
        }
        if (usertable[target].active) {
          udp.SendTo(msg.ip.c_str(), msg.port, "NAK");
          for (auto it = usertable.begin(); it != usertable.end(); it++) {
            std::cout << it->first << ":" << it->second << std::endl;
	          udp.SendTo(msg.ip.c_str(), msg.port,
                       "TABLE:" + it->first + ":" + __row_to_str(it->second));
          }
        } else {
          //TODO: save to text file
          std::thread twrite(write_to_file, target+".log",
                            msg.msg.substr(pos1+1, msg.msg.size()));
          twrite.detach();
          //write_to_file(target+".log", msg.msg.substr(pos1+1, msg.msg.size()));
          udp.SendTo(msg.ip.c_str(), msg.port, "ACK");
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
      std::cout << "error: register failed." << std::endl;
      exit(1);
    }
    bool offline = false;
    std::thread tin(intepreter);
    while (proc_running) {
      msg.msg.clear(); // If not clear, will print out at next iteration

      // Process terminal input
      if (userevent) {
        size_t pos0 = usercmd.find(' ');
        if (usercmd.substr(0,pos0) == "reg") {
          std::string target = usercmd.substr(pos0+1, usercmd.size());
          if (!offline) {
            std::cerr << "warning: you're already online " << target
                      << std::endl << ">>> " << std::flush;
            goto clearevent;
          }// else if (target != nickname) {
          //  std::cerr << "warning: invalid nickname " << target << std::endl
          //            << ">>> " << std::flush;
          //  goto clearevent;
          //}
          udp.SendTo(argv[3], servport, "REG:"+target);
          if (udp.Listen(msg,1,0)) {
            if (msg.msg == "ACK") {
              std::cout << "[You have messages]\n>>> " << std::flush;
            }
            nickname = target;
          } else {
            std::cerr << "error: reg failed\n>>> " << std::flush;
          }
          offline = false;
        }
        if (offline) goto clearevent;
        if (usercmd.substr(0,pos0) == "send") {
          size_t pos1 = usercmd.find(' ', pos0+1);
          std::string target = usercmd.substr(pos0+1, pos1-pos0-1);
          if (usertable.find(target) == usertable.end() ) {
            std::cerr << "warning: " << target << " doesn't exist" << std::endl;
            goto clearevent;
          }
          std::string outmsg = "MSG:" + nickname + ":  " +
                               usercmd.substr(pos1+1, usercmd.size());
          if (!usertable[target].active) {
            std::cout << "[" << target << " is offline, message sent to server.]"
                      << std::endl << ">>> " << std::flush;
            goto offline_msg;
          }
          udp.SendTo(usertable[target].ip.c_str(), usertable[target].port,
                     outmsg);
          if (udp.Listen(msg,0,500000) && msg.msg == "ACK") {
            std::cout << "[Message received by "+ target + ".]" << std::endl
                      << ">>> " << std::flush;
            goto clearevent;
          } else {
            std::cout << "[No ACK from " << target
                      << ", message sent to server.]" << std::endl
                      << ">>> " << std::flush;
          }
offline_msg:
          struct timeval timestamp;
          if (gettimeofday(&timestamp, nullptr) == -1) {
            std::cerr << "error: gettimeofday() failed\n" << strerror(errno)
                      << std::endl;
            exit(1);
          }
          outmsg = "OFFMSG:"+ target + ":" + nickname + ":  <" +
                   std::to_string(timestamp.tv_sec) + "> " +
                   usercmd.substr(pos1+1, usercmd.size());
          udp.SendTo(argv[3], servport, outmsg);
          for (int i = 0; i < 6; i++) {
            if (udp.Listen(msg,0,500000)) {
              if (msg.msg == "ACK") {
                std::cout << "[Messages received by the server and saved]\n>>> "
                          << std::flush;
                goto clearevent;
              } else if (msg.msg == "NAK") {
                std::cerr << "[Client "<< target << " exists!!]\n>>> "
                          << std::flush;
                goto clearevent;
              }
            }
          }
          std::cout << "[Server not responding]\n>>> [Exiting]" << std::endl;
          exit(1);
        } else if (usercmd.substr(0,pos0) == "dereg") {
          std::string target = usercmd.substr(pos0+1, usercmd.size());
          if (target != nickname) {
            std::cerr << "warning: invalid nickname " << target << std::endl
                      << ">>> " << std::flush;
            goto clearevent;
          }
          // Repeat deregistratino up to 6 times
          for (int i = 0; i < 6; i++) {
            udp.SendTo(argv[3], servport, "DEREG:"+target);
            if (udp.Listen(msg,0,500000) && msg.msg == "ACK") {
              std::cout << "[You are Offline. Bye.]" << std::endl
                        << ">>> " << std::flush;
              offline = true;
              goto clearevent;
            }
          }
          std::cout << "[Server not responding]\n>>> [Exiting]" << std::endl;
          exit(1);
        }
clearevent:
        userevent = false;
      }

      // Process UDP message if not offline
      if (offline || !udp.Listen(msg,0,5000) || msg.msg.empty()) {
        continue;
      }
      //std::cout << "receive msg: " << msg.msg << std::endl
      //          << ">>> " << std::flush;
      size_t pos0 = msg.msg.find(':');
      if (msg.msg.substr(0, pos0) == "TABLE") {
        table_pair_t p = __get_table_pair(msg.msg.substr(pos0+1,
                                                         msg.msg.size()));
        usertable[p.first] = p.second;
        std::cout << "[Client table updated.]\n>>> " << std::flush;
      } else if (msg.msg.substr(0, pos0) == "MSG") {
        udp.SendTo(msg.ip.c_str(), msg.port, "ACK");
        std::cout << msg.msg.substr(pos0+1, msg.msg.size()) << std::endl
                  << ">>> " << std::flush;
      }
    }
    proc_running = false;
    tin.join();
  } else {
    std::cout << "warning: unknown argument " << argv[1] << std::endl;
    return 1;
  }
  return 0;
}
