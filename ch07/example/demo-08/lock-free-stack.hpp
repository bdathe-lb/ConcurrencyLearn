#ifndef DEMO08_LOCK_FREE_STACK_HPP
#define DEMO08_LOCK_FREE_STACK_HPP

#include <atomic>
#include <memory>

namespace demo08 {

template<typename T>
class LockFreeStack {
private:
  struct Node;

  struct CountedNodePtr {
    int external_count;
    Node* ptr;
  }; // struct CountedNodePtr

  struct Node {
    std::shared_ptr<T> data;
    std::atomic<int> internal_count;
    CountedNodePtr next;

    explicit Node(const T& value)
      : data(std::make_shared<T>(value))
      , internal_count(0)
    {}
  };

  std::atomic<CountedNodePtr> head_{};

public:
  LockFreeStack() = default;

  LockFreeStack(const LockFreeStack&) = delete;
  LockFreeStack& operator=(const LockFreeStack&) = delete;

  ~LockFreeStack();

private:
  void IncreaseHeadCount(CountedNodePtr& old_counter);

public:
  void Push(const T& data);

  [[nodiscard]] std::shared_ptr<T> Pop();
}; // class LockFreeStack

template<typename T>
void
LockFreeStack<T>::Push(const T& data) {
  CountedNodePtr new_node{};
  new_node.ptr = new Node(data);
  new_node.external_count = 1;
  new_node.ptr->next = head_.load();

  while (!head_.compare_exchange_weak(new_node.ptr->next, new_node))
    ;
}

template<typename T>
std::shared_ptr<T>
LockFreeStack<T>::Pop() {
  CountedNodePtr old_head = head_.load();
  for (;;) {
    IncreaseHeadCount(old_head);

    Node* const ptr = old_head.ptr;
    if (ptr == nullptr) {
      return nullptr;
    }

    if (head_.compare_exchange_strong(old_head, ptr->next)) {
      std::shared_ptr<T> res;
      res.swap(ptr->data);

      const int count_increase = old_head.external_count - 2;
      if (ptr->internal_count.fetch_add(count_increase) + count_increase == 0) {
        delete ptr;
      }

      return res;
    }

    if (ptr->internal_count.fetch_sub(1) == 1) {
      delete ptr;
    }
  }
}

template<typename T>
void
LockFreeStack<T>::IncreaseHeadCount(CountedNodePtr& old_counter) {
  CountedNodePtr new_counter;
  do {
    new_counter = old_counter;
    ++ new_counter.external_count;
  } while (!head_.compare_exchange_strong(old_counter, new_counter));

  old_counter.external_count = new_counter.external_count;
}

template<typename T>
LockFreeStack<T>::~LockFreeStack() {
  while (Pop())
    ;
}

} // namespace demo08

#endif // !DEMO08_LOCK_FREE_STACK_HPP
