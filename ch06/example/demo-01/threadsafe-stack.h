#ifndef THREADSAFE_STACK_H
#define THREADSAFE_STACK_H

#include <exception>
#include <utility>
#include <memory>
#include <mutex>
#include <stack>

struct EmptyStack : std::exception {
  const char* what() const noexcept override {
    return "empty stack";
  }
};

template<typename T>
class ThreadSafeStack {
private:
  std::stack<T> data_;
  mutable std::mutex m_;

private:
  ThreadSafeStack(const ThreadSafeStack& other, std::lock_guard<std::mutex>&&) 
    : data_(other.data_) 
  {}

public:
  ThreadSafeStack() = default;
  ThreadSafeStack(const ThreadSafeStack& other)
    : ThreadSafeStack(other, std::lock_guard<std::mutex>(other.m_))
  {}

  ThreadSafeStack& operator=(const ThreadSafeStack&) = delete;

  void Push(T new_value) {
    std::lock_guard<std::mutex> lock(m_);
    data_.push(std::move(new_value));
  }

  std::shared_ptr<T> Pop() {
    std::lock_guard<std::mutex> lock(m_);

    if (data_.empty()) throw EmptyStack();
    
    std::shared_ptr<T> const res(
      std::make_shared<T>(std::move(data_.top()))
    );
    data_.pop();
    
    return res;
  }

  void Pop(T& value) {
    std::lock_guard<std::mutex> lock(m_);

    if (data_.empty()) throw EmptyStack();

    value = std::move(data_.top());

    data_.pop();
  }

  bool Empty() const {
    std::lock_guard<std::mutex> lock(m_);
    return data_.empty();
  }
};

#endif // !THREADSAFE_STACK_H
