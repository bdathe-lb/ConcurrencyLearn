#ifndef DEMO06_LOCK_FREE_STACK_HPP
#define DEMO06_LOCK_FREE_STACK_HPP

#include <atomic>
#include <memory>

namespace demo06 {

template<typename T>
class LockFreeStack {
private:
  struct Node {
    std::shared_ptr<T> data;
    std::shared_ptr<Node> next;

    explicit Node(const T& value)
      : data(std::make_shared<T>(value))
    {}
  };

  std::shared_ptr<Node> head_;

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
  new_node->next = std::atomic_load(&head_);

  while (!std::atomic_compare_exchange_weak( &head_, &new_node->next, new_node))
    ;
}

template<typename T>
std::shared_ptr<T>
LockFreeStack<T>::Pop() {
  std::shared_ptr<Node> old_head = std::atomic_load(&head_);
  while (old_head && !std::atomic_compare_exchange_weak(
    &head_, &old_head, old_head->next
  ))
    ;

  if (old_head) {
    std::atomic_store(&old_head->next, std::shared_ptr<Node>());
    return old_head->data;
  }

  return nullptr;
}

template<typename T>
LockFreeStack<T>::~LockFreeStack() {
  while (Pop())
    ;
}

} // namespace demo06

#endif // !DEMO06_LOCK_FREE_STACK_HPP
