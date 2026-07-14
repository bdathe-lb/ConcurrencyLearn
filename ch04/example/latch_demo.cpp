#include <chrono>
#include <iostream>
#include <latch>
#include <thread>
#include <vector>

void LoadDataWorker(int id, std::latch& latch) {
  std::this_thread::sleep_for(std::chrono::milliseconds(100 * id));
  std::cout << "Thread " << id << " completed data loading.\n";

  latch.count_down();
}

int main (int argc, char *argv[]) {
  const int thread_count = 10;

  std::latch data_ready_latch(thread_count);
  std::vector<std::thread> threads;

  for (int i = 1; i <= thread_count; ++ i) {
    threads.emplace_back(LoadDataWorker, i, std::ref(data_ready_latch));
  }

  std::cout << "Main thread is waiting for all workers...\n";
    
  data_ready_latch.wait();

  std::cout << "All clear! Main thread starts processing consolidated data.\n";

  for (auto& t : threads) t.join();
  
  return 0;
}
