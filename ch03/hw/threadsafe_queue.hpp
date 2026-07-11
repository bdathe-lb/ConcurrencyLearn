#include <queue>
#include <mutex>
#include <memory>

// This is a thread-safe queue
template<typename T>
class threadsafe_queue {
private:
  std::queue<T> data_queue;
  mutable std::mutex m; // Q1: Why is mutable needed here?
                        // A1: empty() is a const function.  
                        // When internally calling std::lock_guard<std::mutex> lk(m),  
                        // the underlying implicitly invokes m.lock(), which modifies the mutex's internal lock state.  
                        // The C++ compiler would generate an error, so the mutex is marked as mutable  
                        // to inform the compiler that "this variable is allowed to be modified even in const contexts."

public:
  threadsafe_queue() {}

  void push(T new_value) {
    std::lock_guard<std::mutex> lk(m);
    data_queue.push(std::move(new_value)); // Q2: Why is `std::move` needed here?
                                           // A2: Performance optimization, avoid copying
  }

  // Solution A: Pass by reference for safe copying
  bool try_pop(T& value) {
    std::lock_guard<std::mutex> lk(m);
    if (data_queue.empty()) {
      return false;
    }

    value = std::move(data_queue.front());
    data_queue.pop();
    return true;
  }

  // Solution B: Return a smart pointer
  std::shared_ptr<T> try_pop() {
    std::lock_guard<std::mutex> lk(m);
    if (data_queue.empty()) {
      return nullptr; 
    }

    std::shared_ptr<T> ptr = std::make_shared<T>(std::move(data_queue.front()));
    data_queue.pop();
    return ptr;
  }

  bool empty() const {
    std::lock_guard<std::mutex> lk(m);
    return data_queue.empty();
  }
};
