#include <common.h>
#include <myudp.h>
#include <unordered_map>
#include <sstream>

#ifndef DVNODE_H
#define DVNODE_H

typedef std::unordered_map<port_t, float> dv_t;

class DVTable {
public:
  DVTable(port_t x):id_(x){};
  std::string Str();                    // TODO: Overload std::to_string()
  bool Insert(port_t x, dv_t v); // TODO: Overload [] operator
  bool Update();                        // Implement Bellman-Ford algorithm
private:
  const port_t id_;
  dv_t adj_;    // Neighbors and edge costs
  std::unordered_map<port_t, port_t> hops_;
  std::unordered_map<port_t, dv_t> memo_;
};

std::string DVTable::Str() {
  std::stringstream ss;
  for (auto it : memo_[id_]) {
    ss << it.first << ':' << it.second<<'\n';
  }
  return ss.str();
}

bool DVTable::Insert(port_t x, dv_t v) {
  bool ret = (memo_.find(x) == memo_.end());
  memo_[x] = v;
  return ret;
}

bool DVTable::Update() {
  // TODO: implement Bellman-Ford Algorithm
  return true;
}

#endif