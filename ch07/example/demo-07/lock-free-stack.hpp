#ifndef DEMO07_LOCK_FREE_STACK_HPP
#define DEMO07_LOCK_FREE_STACK_HPP

#include <atomic>
#include <memory>

namespace demo07 {

template<typename T>
class LockFreeStack {
private:
  struct Node {
    std::shared_ptr<T> data;
    std::atomic<std::shared_ptr<T>> next;

    explicit Node(const T& value)
      : data(std::make_shared<T>(value))
    {}
  }; // struct Node

  std::atomic<std::shared_ptr<T>> head_;

public:
  LockFreeStack() = default;

  LockFreeStack(const LockFreeStack&) = delete;
  LockFreeStack& operator=(const LockFreeStack&) = delete;

  ~LockFreeStack();

  void Push(const T& data);

  [[nodiscard]] std::shared_ptr<T> Pop();
};

template<typename T>
void
LockFreeStack<T>::Push(const T& data) {
  const std::shared_ptr<Node> new_node = std::make_shared<Node>(data);
  new_node->next = head_.load();

  while (!head_.compare_exchange_weak(new_node->next, new_node))
    ;
}

template<typename T>
std::shared_ptr<T>
LockFreeStack<T>::Pop() {
  std::shared_ptr<Node> old_head = head_.load();
  while (old_head &&
         !head_.compare_exchange_weak(old_head, old_head->next.load()))
    ;

  if (old_head) {
    old_head->next = nullptr;
    return old_head->data;
  }

  return nullptr;
}

template<typename T>
LockFreeStack<T>::~LockFreeStack() {
  while (Pop())
    ;
}

} // namespace demo07

#endif // !DEMO07_LOCK_FREE_STACK_HPP
