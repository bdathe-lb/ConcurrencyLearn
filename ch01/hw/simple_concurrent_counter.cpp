#include <iostream>
#include <thread>

void count_down(int id, int from) {
  for (int i = from; i > 0; -- i) {
    std::cout << "Thread [" << id << "]: [" << i << "]\n";
  }
}

int main (int argc, char *argv[]) {

  std::thread t1(count_down, 1, 5);
  std::thread t2(count_down, 2, 3);

  t1.join();
  t2.join();

  std::cout << "All threads finished. Exiting main.\n";

  return 0;
}
