#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

class SimpleTaskExecutor {
private:
  std::queue<std::packaged_task<void()>> tasks_;
  std::mutex mtx_;

public:
  SimpleTaskExecutor() = default;
  SimpleTaskExecutor(const SimpleTaskExecutor&) = delete;
  SimpleTaskExecutor& operator=(const SimpleTaskExecutor&) = delete;

  template<typename Func>
  std::future<void> PostTask(Func f) {
    std::packaged_task<void()> task(f);
    std::future<void> fut = task.get_future();

    {
      std::lock_guard<std::mutex> lk(mtx_);
      tasks_.push(std::move(task));
    }

    return fut;
  }

  void ExecuteNextTask() {
    std::packaged_task<void()> task;

    {
      std::lock_guard<std::mutex> lk(mtx_);
      if (tasks_.empty()) {
        return;
      }

      task = std::move(tasks_.front());
      tasks_.pop();
    }

    task();
  }
};

int main (int argc, char *argv[]) {
  SimpleTaskExecutor executor;

  auto f1 = executor.PostTask([]() {
      std::cout << "Task 1 executing in thread: " << std::this_thread::get_id() << "\n";
  });

  auto f2 = executor.PostTask([]() {
      std::cout << "Task 2 executing in thread: " << std::this_thread::get_id() << "\n";
  });

  std::cout << "Executor satrts processinf tasks...\n";
  executor.ExecuteNextTask();
  executor.ExecuteNextTask();

  f1.wait();
  f2.wait();
  std::cout << "All tasks verified complete!\n";
  
  return 0;
}
