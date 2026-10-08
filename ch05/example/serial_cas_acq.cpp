// 中继站「自己要用上游的货」—— CAS 的 acquire 半是必需的
//
// thread_2 在接力之后自己读了 data。释放序列那条直达线路只对 thread_3 有效，
// thread_2 是链子中间的一节，蹭不上，必须亲自 acquire 才能读 thread_1 写的东西。
//
// 把成功序改成 memory_order_relaxed，再用
//   g++ -O1 -g -std=c++17 -pthread -fsanitize=thread serial_cas_acq.cpp
// 编译运行，TSAN 会报 data race in thread_2。

#include <iostream>
#include <atomic>
#include <thread>

std::atomic<int> sync(0);

int data(0);

void thread_1() {
  data = 42;
  sync.store(1, std::memory_order_release);
}

void thread_2() {
  int expected = 1;
  while (!sync.compare_exchange_strong(
         expected,
         2,
         std::memory_order_acquire,   // ← 换成 relaxed 就会出竞争
         std::memory_order_relaxed
  )) {
    expected = 1;
  }

  std::cout << "thread_2 data = " << data << "\n";
}

void thread_3() {
  while (sync.load(std::memory_order_acquire) < 2);

  std::cout << "thread_3 data = " << data << "\n";
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
