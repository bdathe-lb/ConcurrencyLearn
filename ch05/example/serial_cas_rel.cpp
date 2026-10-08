// 中继站「自己有货要发下游」—— CAS 的 release 半是必需的
//
// thread_2 在接力之前写了自己的 extra。data 走释放序列的直达线路，thread_3 总能读到；
// 但 extra 是 thread_2 自己的东西，没有 release 就封不进包裹，发不出去。
//
// 把成功序改成 memory_order_relaxed，再用
//   g++ -O1 -g -std=c++17 -pthread -fsanitize=thread serial_cas_rel.cpp
// 编译运行，TSAN 会报 data race in thread_3（注意报错位置和 _acq 那份不同）。

#include <iostream>
#include <atomic>
#include <thread>

std::atomic<int> sync(0);

int data(0), extra(0);

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
         std::memory_order_release,   // ← 换成 relaxed 就会出竞争
         std::memory_order_relaxed
  )) {
    expected = 1;
  }
}

void thread_3() {
  while (sync.load(std::memory_order_acquire) < 2);

  std::cout << "data = " << data << ", extra = " << extra << "\n";
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
