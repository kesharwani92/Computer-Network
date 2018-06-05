#include <UdpChat.h>

int main() {
  table_t table;
  table["John"]=table_row_t{1000,50,false};
  std::cout << table["John"] << std::endl;
  std::cout << table;
  return 0;
}
