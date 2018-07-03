/*******************************************************************************
* dvnnode.h
* 
* Distance Vector Algorithm.
*
* Author: Yan-Song Chen
* Date  : Jul 2nd, 2018
*******************************************************************************/
#include <common.h>
#include <myudp.h>
#include <unordered_map>
#include <sstream>

#ifndef DVNODE_H
#define DVNODE_H
typedef std::unordered_map<port_t, float> dv_t;

// The format of a distance vector UDP message is
// [sender port][entryDelim][dest. port][pairDelim][shortest dist.]
// For example, if the sender is port 1111, the distance to another node is
// 0.5, then the UDP message will be: 1111:1111,0.0:2222,0.5
const char pairDelim = ',';
const char entryDelim = ':';

inline std::string dv_out(dv_t v) {
  std::stringstream ss;
  for (auto it : v) {
    ss << it.first << pairDelim << it.second << entryDelim;
  }
  return ss.str();
}

std::pair<port_t, dv_t> dv_in(std::string msg) {
  //std::cout << "dv_in " << msg << std::endl;
  size_t pos0 = 0, pos1 = msg.find(entryDelim, pos0), pos2 = pos1;
  port_t port = cstr_to_port(msg.substr(pos0, pos1-pos0).c_str());
  dv_t vec;
  while (true) {
    pos0 = pos2 + 1;
    pos1 = msg.find(pairDelim, pos0);
    pos2 = msg.find(entryDelim, pos1+1);
    if (pos1 == std::string::npos || pos2 == std::string::npos) break;
    port_t p = cstr_to_port(msg.substr(pos0, pos1-pos0).c_str());
    float f = std::stof(msg.substr(pos1+1, pos2-pos1-1));
    vec[p] = f;
  }
  return std::make_pair(port, vec);
}

// Update the optimal distance to destination. If the distance vector is
// changed, return true; return false otherwise.
bool bellman_ford_update(dv_t& myvec, std::unordered_map<port_t, port_t>& myhop,
    const std::unordered_map<port_t, dv_t>& memo) {
  bool ret = false;
  for (const auto& edge : myvec) {
    auto src = memo.find(edge.first);
    if (src == memo.end()) continue;
    for (const auto& dest : src->second) {
      if (myvec.find(dest.first ) == myvec.end() ||
          edge.second + dest.second < myvec[dest.first]) {
        myhop[dest.first] = myhop[edge.first];
        myvec[dest.first] = edge.second + dest.second;
        ret = true;
      }
    }
  }
  return ret;
}

#endif