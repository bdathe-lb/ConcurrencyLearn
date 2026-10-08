#include <iostream>
#include <atomic>
#include <thread>

std::atomic<int> sync(0);

int data(0);
int extra(0);

void thread_1() {
  data = 42;
  sync.store(1, std::memory_order_release);
}

void thread_2() {
  extra = 7;

  int expected = 1;
  while (!sync.compare_exchange_strong(
         expected, 
         2, 
         std::memory_order_acq_rel
  )) {
    expected = 1;
  }
}

void thread_3() {
  while (sync.load(std::memory_order_acquire) < 2);

  std::cout << "data = " << data << ", ";
  std::cout << "extra = " << extra << "\n";
}

int main (int argc, char *argv[]) {

  std::thread thread1(thread_1);
  std::thread thread2(thread_2);
  std::thread thread3(thread_3);

  thread1.join();
  thread2.join();
  thread3.join();
  
  return 0;
}
