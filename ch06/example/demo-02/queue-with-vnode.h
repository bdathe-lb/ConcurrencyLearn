#ifndef QUEUE_WITH_VNODE_H
#define QUEUE_WITH_VNODE_H

#include <memory>
#include <utility>

template<typename T>
class QueueWithVNode {
private:
  struct Node {
    std::shared_ptr<T> data;
    std::unique_ptr<Node> next;
  };

  std::unique_ptr<Node> head_;
  Node* tail_;

public:
  QueueWithVNode()
    : head_(new Node)
    , tail_(head_.get())
  {}

  QueueWithVNode(const QueueWithVNode&) = delete;
  QueueWithVNode& operator=(const QueueWithVNode&) = delete;

  std::shared_ptr<T> TryPop() {
    if (head_.get() == tail_) {
      return std::shared_ptr<T>();
    }

    std::shared_ptr<T> const res(std::move(head_->data));
    std::unique_ptr<Node> old_head = std::move(head_);
    head_ = std::move(old_head->next);

    return res;
  }

  void Push(T new_value) {
    std::shared_ptr<T> new_data(
      std::make_shared<T>(std::move(new_value))
    );

    std::unique_ptr<Node> p(new Node);
    tail_->data = std::move(new_data);
    Node* const new_tail = p.get();
    tail_->next = std::move(p);
    tail_ = new_tail;
  }
};

#endif // !QUEUE_WITH_VNODE_H
