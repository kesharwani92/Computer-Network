#include <dvnode.h>

int main(int argc, char** argv) {
  DVTable t(1111);
  t.Insert(1111, {{2222, 0.1},{3333, 0.5}});
  std::cout << t.Str() << std::endl;
  return 0;
}