#include <iostream>
#include <condition_variable>
#include <mutex>
#include <thread>

template<typename T>
class ThreadSafeBox {
private:
  T item_;
  std::mutex mtx_;
  std::condition_variable cv_producer_;
  std::condition_variable cv_consumer_;
  bool has_item_ = false;

public:
  // Explicitly prohibit copying and assignment, complying with modern C++ guidelines for thread-safe components.
  ThreadSafeBox() = default;
  ThreadSafeBox(const ThreadSafeBox&) = delete;
  ThreadSafeBox& operator=(const ThreadSafeBox&) = delete;

  void Put(T value) {
    // Locking
    std::unique_lock<std::mutex> lk(mtx_);
    // Wait for the box to become empty
    cv_producer_.wait(lk, [this] { return !has_item_;} );
    // Put data in, update state
    item_ = std::move(value);
    has_item_ = true;
    // Notify consumers
    cv_consumer_.notify_one();
  }

  T Take() {
    // Locking
    std::unique_lock<std::mutex> lk(mtx_);
    // Wait for the box to become full
    cv_consumer_.wait(lk, [this] { return has_item_; });
    // Retrieve data, update state
    T value = std::move(item_);
    has_item_ = false;
    // Notify producer
    cv_producer_.notify_one();
    
    return value;
  }
};

std::mutex count_mtx;
int main (int argc, char *argv[]) {
  ThreadSafeBox<int> box;

  std::thread consumer([&]() {
    for (int i = 0; i < 5; ++ i) {
      int val = box.Take();
      {
        std::lock_guard<std::mutex> lock(count_mtx);
        std::cout << "Consumed: " << val << "\n";
      }
    }
  });

  std::thread producer([&](){
    for(int i = 1; i <= 5; ++i) {
      box.Put(i);

      {
        std::lock_guard<std::mutex> lock(count_mtx);
        std::cout << "Produced: " << i << "\n";
      }
    }
  });

  producer.join();
  consumer.join();
  
  return 0;
}
