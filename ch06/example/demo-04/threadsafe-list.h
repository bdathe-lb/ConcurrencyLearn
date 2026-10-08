#ifndef DEMO_04_THREADSAFE_LIST_H
#define DEMO_04_THREADSAFE_LIST_H

#include <mutex>
#include <memory>
#include <utility>
#include <algorithm>

namespace demo04 {

template<typename T>
class ThreadSafeList {
private:
  struct Node {
    std::shared_ptr<T> data;
    std::unique_ptr<Node> next;
    std::mutex mutex;

    Node() = default;
    explicit Node(const T& value);
  }; // class Node

  Node head_;

public:
  ThreadSafeList() = default;
  ~ThreadSafeList();

  ThreadSafeList(const ThreadSafeList&) = delete;
  ThreadSafeList& operator=(const ThreadSafeList&) = delete;

  void PushFront(const T& value);

  template<typename Function>
  void ForEach(Function f);

  template<typename Predicate>
  std::shared_ptr<T> FindFirstIf(Predicate p);


  template<typename Predicate>
  void RemoveIf(Predicate p);
}; // class ThreadSafeList

template<typename T>
ThreadSafeList<T>::Node::Node(const T& value)
  : data(std::make_shared<T>(value))
{}

template<typename T>
ThreadSafeList<T>::~ThreadSafeList() {
  RemoveIf([](const T&) { return true; });
}

template<typename T>
void
ThreadSafeList<T>::PushFront(const T& value) {
  std::unique_ptr<Node> new_node = std::make_unique<Node>(value);
  std::lock_guard<std::mutex> lock(head_.mutex);
  new_node->next = std::move(head_.next);
  head_.next = std::move(new_node);
}

template<typename T>
template<typename Function>
void
ThreadSafeList<T>::ForEach(Function f) {
  Node* current = &head_;
  std::unique_lock<std::mutex> lock(head_.mutex);
  while (Node* const next = current->next.get()) {
    std::unique_lock<std::mutex> next_lock(next->mutex);
    lock.unlock();
    f(*next->data);
    current = next;
    lock = std::move(next_lock);
  }
}

template<typename T>
template<typename Predicate>
std::shared_ptr<T> 
ThreadSafeList<T>::FindFirstIf(Predicate p) {
  Node* current = &head_;
  std::unique_lock<std::mutex> lock(head_.mutex);
  while (Node* const next = current->next.get()) {
    std::unique_lock<std::mutex> next_lock(next->mutex);
    lock.unlock();
    if (p(*next->data)) {
      return next->data;
    }
    current = next;
    lock = std::move(next_lock);
  }
  return std::shared_ptr<T>();
}

template<typename T>
template<typename Predicate>
void
ThreadSafeList<T>::RemoveIf(Predicate p) {
  Node* current = &head_;
  std::unique_lock<std::mutex> lock(head_.mutex);
  while (Node* const next = current->next.get()) {
    std::unique_lock<std::mutex> next_lock(next->mutex);
    if (p(*next->data)) {
      std::unique_ptr<Node> old_next = std::move(current->next);
      current->next = std::move(next->next);
      next_lock.unlock();

      // old_next 在此会被析构
    } else {
      lock.unlock();
      current = next;
      lock = std::move(next_lock);
    }
  }
}

} // namespace demo04

#endif // !DEMO_04_THREADSAFE_LIST_H
