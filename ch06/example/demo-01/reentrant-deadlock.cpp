#include "threadsafe-stack.h"

#include <iostream>
#include <utility>

struct Reentrant {
  int data;
  
  Reentrant() = default;
  Reentrant(Reentrant&&);
};

ThreadSafeStack<Reentrant> g_stack;

Reentrant::Reentrant(Reentrant&& other) {
  std::cerr << "Move constructor\n";

  g_stack.Empty();
  data = std::move(other.data);
}

int main (int argc, char *argv[]) {

  Reentrant r{};
  g_stack.Push(std::move(r));

  return 0;
}
