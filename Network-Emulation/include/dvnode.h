#include <common.h>
#include <myudp.h>
#include <unordered_map>
#include <set>
#include <sstream>

#ifndef DVNODE_H
#define DVNODE_H

typedef std::unordered_map<port_t, float> dv_t;

class DVTable {
public:
  DVTable(port_t x):id_(x){};
  DVTable(std::string s, char d1, char d2);
  std::string DVStr();                    // TODO: Overload std::to_string()
  bool Insert(port_t x, dv_t v); // TODO: Overload [] operator
  bool Update();                        // Implement Bellman-Ford algorithm
  //friend std::ostream& operator << (std::ostream&, const DVTable&);
  friend std::ostream& operator << (std::ostream& os, const DVTable& t);  
private:
  const port_t id_;
  dv_t adj_;    // Neighbors and edge costs
  std::unordered_map<port_t, port_t> hops_;
  std::unordered_map<port_t, dv_t> memo_;

};

std::string DVTable::DVStr() {
  std::stringstream ss;
  ss << id_ << '\n';
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

std::ostream& operator << (std::ostream& os, const DVTable& t) {
  for (auto it : t.memo_) {
    os << "port " << it.first;
    for (auto it2 : it.second) {
      os << " | " << it2.first << "(" << it2.second << ")";
    }
    os << std::endl;
  }
  return os;
}
#endif