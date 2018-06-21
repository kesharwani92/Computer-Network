#include <common.h>
#include <thread>
#include <atomic>
#include <string>
#include <queue>
#define GBNNODE_DEBUG 1 // Debugging mode

// Shared memory accross threads
bool proc_running = true;

std::atomic_flag lock = ATOMIC_FLAG_INIT; // lock for "messages"
std::queue<std::string> messages;

int fd;
struct sockaddr_in src, dest;

inline void __grab_lock() {
  while (lock.test_and_set(std::memory_order_acquire));
}

inline void __release_lock() {
  lock.clear(std::memory_order_release);
}

// Multi-threaded functions

// Takes in command line input and process command type
void cin_proc() {
  while (proc_running) {
    std::string line;
    std::getline(std::cin, line);

    size_t delim = line.find(' ');
    if (line.substr(0, delim) != "send") {
      #if GBNNODE_DEBUG
      std::cerr << "warning: support only send operation" << std::endl;
      #endif
      continue;
    }

    std::string message = line.substr(delim+1, line.size()-delim);

    #if GBNNODE_DEBUG
    std::cout << "cin_proc: got " << message << std::endl;
    #endif
    __grab_lock();
    messages.push(message);
    __release_lock();
  }
}

// Actual implementation of Go-Back-N protocol
// Pops out the top message and send it over UDP
void gbn_proc(const int& window) {
  const char* buf = nullptr;
  size_t bufsize = 0;
  while (proc_running) {
    if (messages.empty()) continue;
    __grab_lock();
    bufsize = messages.size();
    buf = messages.front().c_str();

    #if GBNNODE_DEBUG
    printmytime();
    std::cout << "gbn_proc: send " << buf << std::endl;
    #endif

    // TODO: implement GBN protocol here

    messages.pop();
    __release_lock();
  }
}

int main(int argc, char** argv) {
  #if GBNNODE_DEBUG
  std::cout << "Arguments" << "Window size = " << std::stoi(argv[1]) << std::endl
      << "Source port: " << cstr_to_port(argv[2]) << std::endl
      << "Destination port: " << cstr_to_port(argv[3]) << std::endl;
  #endif
  const int window = std::stoi(argv[1]);
  set_udp_addr(src, cstr_to_port(argv[2]));
  set_udp_addr(dest, cstr_to_port(argv[3]));
  initialize(fd, src);
  std::thread user_cin(cin_proc);
  std::thread send_gbn(gbn_proc, window);
  user_cin.join();
  send_gbn.join();
  close(fd);
  return 0;
}