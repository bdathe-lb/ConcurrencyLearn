#ifndef THREADSAFE_QUEUE_FINE_GRAINED_H
#define THREADSAFE_QUEUE_FINE_GRAINED_H

#include <memory>
#include <mutex>
#include <utility>

template<typename T>
class ThreadSafeQueueFineGrained {
private:
  struct Node {
    std::shared_ptr<T> data;
    std::unique_ptr<Node> next;
  };

  std::mutex head_mutex_;
  std::mutex tail_mutex_;
  std::unique_ptr<Node> head_;
  Node* tail_;

private:
  Node* GetTail() {
    std::lock_guard<std::mutex> tail_lock(tail_mutex_);
    return tail_;
  }

  std::unique_ptr<Node> PopHead() {
    std::lock_guard<std::mutex> head_lock(head_mutex_);
    if (head_.get() == GetTail()) {
      return nullptr;
    }

    std::unique_ptr<Node> old_head = std::move(head_);
    head_ = std::move(old_head->next);
    return old_head;
  }

public:
  ThreadSafeQueueFineGrained()
    : head_(new Node)
    , tail_(head_.get())
  {}

  ~ThreadSafeQueueFineGrained() {
    while (head_) {
      std::unique_ptr<Node> p = std::move(head_->next);
      head_ = std::move(p);
    }
  }

  ThreadSafeQueueFineGrained(const ThreadSafeQueueFineGrained&) = delete;
  ThreadSafeQueueFineGrained& operator=(const ThreadSafeQueueFineGrained&) = delete;

  std::shared_ptr<T> TryPop() {
    std::unique_ptr<Node> old_head = PopHead();
    return old_head ? std::move(old_head->data) : std::shared_ptr<T>();
  }

  void Push(T new_value) {
    std::shared_ptr<T> new_data(
      std::make_shared<T>(std::move(new_value))
    );

    std::unique_ptr<Node> p(new Node);
    Node* const new_tail = p.get();
    std::lock_guard<std::mutex> tail_lock(tail_mutex_);
    tail_->data = std::move(new_data);
    tail_->next = std::move(p);
    tail_ = new_tail;
  }
};

#endif // !THREADSAFE_QUEUE_FINE_GRAINED_H
