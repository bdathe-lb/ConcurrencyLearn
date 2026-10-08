#ifndef QUEUE_H
#define QUEUE_H

#include <memory>
#include <utility>

template<typename T>
class Queue {
private:
  struct Node {
    T data;
    std::unique_ptr<Node> next;

    Node(T value)
      : data(std::move(value))
    {}
  };

  std::unique_ptr<Node> head_;
  Node* tail_ = nullptr;

public:
  Queue() = default;
  Queue(const Queue&) = delete;
  Queue& operator=(const Queue&) = delete;

  std::shared_ptr<T> TryPop() {
    if (!head_) {
      return std::shared_ptr<T>();
    }

    std::shared_ptr<T> const res(
      std::make_shared<T>(std::move(head_->data))
    );

    std::unique_ptr<Node> const old_head = std::move(head_);
    head_ = std::move(old_head->next);
    if (!head_) tail_ = nullptr;

    return res;
  }

  void Push(T new_value) {
    std::unique_ptr<Node> p(new Node(std::move(new_value)));
    Node* const new_tail = p.get();

    if (tail_) {
      tail_->next = std::move(p);
    } else {
      head_ = std::move(p);
    }

    tail_ = new_tail;
  }
};

#endif // !QUEUE_H
