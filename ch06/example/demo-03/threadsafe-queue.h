#ifndef DEMO_03_THREADSAFE_QUEUE_H
#define DEMO_03_THREADSAFE_QUEUE_H

#include <algorithm>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <utility>

namespace demo03 {

template<typename T>
class ThreadSafeQueue {
private:
  struct Node {
    std::shared_ptr<T> data;
    std::unique_ptr<Node> next;
  };

  std::mutex head_mutex_;
  std::mutex tail_mutex_;
  std::unique_ptr<Node> head_;
  Node* tail_;
  std::condition_variable data_cond_;

private:
  Node* GetTail();
  std::unique_ptr<Node> PopHead();
  std::unique_lock<std::mutex> WaitForData();
  std::unique_ptr<Node> WaitPopHead();
  std::unique_ptr<Node> WaitPopHead(T& value);
  std::unique_ptr<Node> TryPopHead();
  std::unique_ptr<Node> TryPopHead(T& value);

public:
  ThreadSafeQueue();
  ~ThreadSafeQueue();

  ThreadSafeQueue(const ThreadSafeQueue&) = delete;
  ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

  std::shared_ptr<T> TryPop();
  bool TryPop(T& value);
  std::shared_ptr<T> WaitAndPop();
  void WaitAndPop(T& value);
  void Push(T new_value);
  bool Empty();
};

template<typename T>
typename ThreadSafeQueue<T>::Node* ThreadSafeQueue<T>::GetTail() {
  std::lock_guard<std::mutex> tail_lock(tail_mutex_);
  return tail_;
}

template<typename T>
std::unique_ptr<typename ThreadSafeQueue<T>::Node> ThreadSafeQueue<T>::PopHead() {
  std::unique_ptr<typename ThreadSafeQueue<T>::Node> old_head = std::move(head_);
  head_ = std::move(old_head->next);
  return old_head;
}

template<typename T>
std::unique_lock<std::mutex> ThreadSafeQueue<T>::WaitForData() {
  std::unique_lock<std::mutex> head_lock(head_mutex_);
  data_cond_.wait(head_lock, [&] { return head_.get() != GetTail();});
  return head_lock;
}

template<typename T>
std::unique_ptr<typename ThreadSafeQueue<T>::Node> ThreadSafeQueue<T>::WaitPopHead() {
  std::unique_lock<std::mutex> head_lock(WaitForData());
  return PopHead();
}

template<typename T>
std::unique_ptr<typename ThreadSafeQueue<T>::Node> ThreadSafeQueue<T>::WaitPopHead(T& value) {
  std::unique_lock<std::mutex> head_lock(WaitForData());
  value = std::move(*head_->data);
  return PopHead();
}

template<typename T>
std::unique_ptr<typename ThreadSafeQueue<T>::Node> ThreadSafeQueue<T>::TryPopHead() {
  std::lock_guard<std::mutex> head_lock(head_mutex_);
  if (head_.get() == GetTail()) {
    return nullptr;
  }

  return PopHead();
}

template<typename T>
std::unique_ptr<typename ThreadSafeQueue<T>::Node> ThreadSafeQueue<T>::TryPopHead(T& value) {
  std::lock_guard<std::mutex> head_lock(head_mutex_);
  if (head_.get() == GetTail()) {
    return nullptr;
  }

  value = std::move(*head_->data);
  return PopHead();
}

template<typename T>
ThreadSafeQueue<T>::ThreadSafeQueue() : head_(new Node)
  , tail_(head_.get())
{}

template<typename T>
ThreadSafeQueue<T>::~ThreadSafeQueue() {
  while (head_) {
    std::unique_ptr<Node> p = std::move(head_->next);
    head_ = std::move(p);
  }
}

template<typename T>
void ThreadSafeQueue<T>::Push(T new_value) {
  std::shared_ptr<T> new_data(
    std::make_shared<T>(std::move(new_value))
  );

  std::unique_ptr<Node> p = std::make_unique<Node>();
  {
    std::lock_guard<std::mutex> tail_lock(tail_mutex_);
    tail_->data = new_data;

    Node* const new_tail = p.get();

    tail_->next = std::move(p);
    tail_ = new_tail;
  }
  data_cond_.notify_one();
}

template<typename T>
std::shared_ptr<T> ThreadSafeQueue<T>::WaitAndPop() {
  std::unique_ptr<Node> const old_head = WaitPopHead();
  return old_head->data;
}

template<typename T>
void ThreadSafeQueue<T>::WaitAndPop(T& value) {
  std::unique_ptr<Node> const old_head = WaitPopHead(value);
}

template<typename T>
std::shared_ptr<T> ThreadSafeQueue<T>::TryPop() {
  std::unique_ptr<Node> old_head = TryPopHead();
  return old_head ? old_head->data : nullptr;
}

template<typename T>
bool ThreadSafeQueue<T>::TryPop(T& value) {
  std::unique_ptr<Node> old_head = TryPopHead(value);
  return old_head != nullptr;
}

template<typename T>
bool ThreadSafeQueue<T>::Empty() {
  std::lock_guard<std::mutex> head_lock(head_mutex_);
  return head_.get() == GetTail();
}

} // namespace demo03

#endif // !DEMO_03_THREADSAFE_QUEUE_H
