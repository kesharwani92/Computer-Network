/*******************************************************************************
* UdpChat.cc
* 
* Simple chat application.
*
* Author: Yan-Song Chen
* Date  : Jun 5th, 2018
*******************************************************************************/
#include <UdpChat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

const size_t BUFSIZE = 2048;
int main(int argc, char** argv) {
  if (std::string(argv[1]) == "-s") {
    if (argc < 2) {
      throw "error: too few arguments";
    }
    /**************************************************************************
    *  Server code starts from here
    ***************************************************************************/
    unsigned short port = strtoul(argv[2], nullptr, 0);
    std::cout << "Server mode at port " << port << std::endl;

    struct sockaddr_in servaddr;
    struct sockaddr_in clntaddr;
    socklen_t addrlen = sizeof(clntaddr);
    int recvlen;
    int fd;
    unsigned char buf[BUFSIZE];

    // Create a Udp socket
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
      throw "error: cannot create socket";
    }
    // Bind the socket
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr*)&servaddr, sizeof(servaddr))<0) {
      throw "error: bind failed";
    }

    while (true) {
      recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr*)&clntaddr,
                         &addrlen);
      if (recvlen > 0) {
        buf[recvlen] = 0;
	std::cout << buf << std::endl;
      }
    }

    close(fd);

  } else if (std::string(argv[1]) == "-c") {
    if (argc < 5) {
      throw "error: too few arguments";
    }
    /**************************************************************************
    *  Client code starts from here
    ***************************************************************************/
    std::string cname(argv[1]);
    int fd;
    // Create server Udp socket
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
      throw "error: cannot create socket";
    }
    struct sockaddr_in servaddr;
    memset((char*)&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    unsigned short servport = strtoul(argv[3], nullptr, 0);
    servaddr.sin_port = htons(servport);
    if (inet_aton(argv[2],&servaddr.sin_addr) < 0) {
      throw "error: invalid server ip";
    };

    while (true) {
      std::cout << ">>> ";
      std::string msg;
      getline(std::cin, msg);
      if (sendto(fd, msg.c_str(), msg.size(), 0, (struct sockaddr *)&servaddr,
          sizeof(servaddr)) < 0) {
        throw "error: sendto failed";
      }
    }
    close(fd);
  } else {
    std::cout << "warning: unknown argument " << argv[1] << std::endl;
  }
  return 0;
}
