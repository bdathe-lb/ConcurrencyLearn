#include <thread>
#include <iostream>

// This is a executable unit for thread
void independent_task() {
  std::cout << "Concurrent task is running seamlessly!\n";
}

int main() {
  std::thread t(independent_task);

  std::cout << "Hello World from main thread!\n";

  t.join();

  return 0;
}
