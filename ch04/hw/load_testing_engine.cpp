#include <barrier>
#include <chrono>
#include <functional>
#include <iostream>
#include <latch>
#include <thread>
#include <vector>
#include <syncstream>

// Global configuration and state
const int kNumberWorkers = 3;
const int kTotalRounds = 3;

// Define the completion callback lambda for barrier: 
// used to automatically print statistical reports at the end of each round
auto ReportCompletion = []() noexcept {
  static int current_round = 1;
  std::cout << "\n============================================\n";
  std::cout << "[METRICS] 第 " << current_round << " 轮压测结束！\n";
  std::cout << " [METRICS] 聚合吞吐量: " << (10000 * kNumberWorkers) << " ops/sec\n";
  std::cout << "============================================\n\n";

  ++ current_round;
};

// Core synchronization component definition
std::latch init_latch(kNumberWorkers);
std::barrier<decltype(ReportCompletion)> round_barrier(kNumberWorkers, ReportCompletion);

// Work thread function
void WorkerThread(int id, std::latch& init_latch, std::barrier<decltype(ReportCompletion)>& round_barrier) {
  // Stage 1
  {
    // Use std::osyncstream to wrap std::cout, ensuring atomic line printing and eliminating character ordering issues in multithreaded environments.
    std::osyncstream(std::cout) << "[Worker " << id << "] 正在建立数据库长连接，预热缓存...\n";
  }

  // Simulate the random time taken to establish a connection
  std::this_thread::sleep_for(std::chrono::milliseconds(200 * (id + 1)));
  {
    std::osyncstream(std::cout) << "[Worker " << id << "] 连接建立成功，节点就绪！\n";
  }

  init_latch.count_down();

  for (int round = 1; round <= kTotalRounds; ++ round) {
    std::this_thread::sleep_for(std::chrono::milliseconds(150 * (3 - id)));
    {
      std::osyncstream(std::cout) << "[Worker " << id << "] 完成了第 " << round << " 轮的压测任务。\n";
    }

    round_barrier.arrive_and_wait();
  }

  {
    std::osyncstream(std::cout) << "[Worker " << id << "] 安全退出线程\n";
  }
}

int main (int argc, char *argv[]) {
  std::cout << "[Controller] 压测控制中心启动，正在引导集群节点...\n";

  std::vector<std::thread> workers;

  for (int i = 0; i < kNumberWorkers; ++ i) {
    workers.emplace_back(WorkerThread, i, std::ref(init_latch), std::ref(round_barrier));
  }

  std::cout << "[Controller] 控制中心进入等待状态，死等所有节点就绪...\n";

  init_latch.wait();

  std::cout << "[Controller] 💥 集群全员就绪！下发指令：正式开启压测演练！💥\n\n";

  for (auto& t : workers) t.join();

  std::cout << "[Controller] 整个分布式压测任务圆满结束，关闭引擎。\n";
  
  return 0;
}
