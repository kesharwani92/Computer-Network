/*******************************************************************************
* cnnode.cc
* 
* Go-Back-N protocol.
*
* Author: Yan-Song Chen
* Date  : Jul 4th, 2018
*******************************************************************************/
#include <common.h>

int main(int argc, char** argv) {
  timestamp_t t1 = TIMESTAMP_NOW;
  while (true) {
    std::cout << '.';
    if (checktimeout(t1,500)) {
      std::cout << "timeout!" << std::endl;
      break;
    }
  }
  return 0;
}