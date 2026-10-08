#include "threadsafe-stack.h"
#include <chrono>
#include <thread>

void StartWork() {
  ThreadSafeStack<int> stack;

  std::thread t([&]() {
    for (int i = 0; i < 100; ++ i) 
      stack.Push(i);
  });

  t.detach();
}

int main (int argc, char *argv[]) {

  StartWork();

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  return 0;
}
