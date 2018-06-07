/*******************************************************************************
* UdpChat.cc
* 
* Simple chat application.
*
* Author: Yan-Song Chen
* Date  : Jun 6th, 2018
*******************************************************************************/
#include <UdpChat.h>

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

    while (true) {
      udpmsg_t msg = udp.Listen();
      std::cout << "Receive msg from " << msg.ip << ':';
      std::cout << msg.port << std::endl;
      std::cout << "Content " << msg.msg << std::endl;
    }

  /*****************************************************************************
  *  Client code starts from here
  *****************************************************************************/
  } else if (std::string(argv[1]) == "-c") {
    if (argc < 5) {
      throw "error: too few arguments";
    }
    uint16_t myport = strtoul(argv[4], nullptr, 0);
    UdpSocket udp(myport);
    std::cout << "Client mode" << std::endl;

    pid_t pid = fork();

    // Listener: response to a udp message
    if (pid > 0) {
      while (true) {
        udpmsg_t msg = udp.Listen();
        std::cout << "echo: " << msg.msg << std::endl << ">>> " << std::flush;
      }
    // Talker: take input from the user
    } else {
      uint16_t servport = strtoul(argv[3], nullptr, 0);
      while (true) {
        std::string msg;
        std::cout << ">>> " << std::flush;
        getline(std::cin, msg);
	udp.SendTo(argv[2], servport, msg);
      }
    }
  } else {
    std::cout << "warning: unknown argument " << argv[1] << std::endl;
  }
  return 0;
}
