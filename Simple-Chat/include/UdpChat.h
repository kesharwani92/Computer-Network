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
