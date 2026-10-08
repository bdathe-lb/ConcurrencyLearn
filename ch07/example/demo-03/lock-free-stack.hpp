#ifndef DEMO03_LOCK_FREE_STACK_HPP
#define DEMO03_LOCK_FREE_STACK_HPP

#include <atomic>
#include <memory>

namespace demo03 {

template<typename T>
class LockFreeStack {
private:
  struct Node {
    std::shared_ptr<T> data;
    Node* next = nullptr;

    explicit Node(const T& value)
      : data(std::make_shared<T>(value))
    {}
  }; // class Node

  std::atomic<Node*> head_{nullptr};

public:
  LockFreeStack() = default;

  LockFreeStack(const LockFreeStack&) = delete;
  LockFreeStack& operator=(const LockFreeStack&) = delete;

  void Push(const T& data);
  [[nodiscard]] std::shared_ptr<T> Pop();
}; // class LockFreeStack

template<typename T>
void
LockFreeStack<T>::Push(const T& data) {
  // 数据在 Push 阶段就分配好
  // Pop 只拷贝 shared_ptr（不抛异常）
  // 
  // 会抛异常的操作（make_shared / new）全部发生在 Push 里、节点接入
  // 链表之前——那时 head_ 还没动过，抛了也什么都没丢，
  // Pop 中 CAS 成功、节点已摘离栈顶之后，剩下的只有拷贝 shared_ptr，
  // 标准保证 noexcept，所以元素不可能丢失
  //
  // 原则：会抛的操作一律放在改变共享状态之前；
  //       CAS 成功之后的每一步都必须是 noexcept 的。
  Node* const new_node = new Node(data);
  new_node->next = head_.load();
  while (!head_.compare_exchange_weak(new_node->next, new_node))
    ;
}

template<typename T>
std::shared_ptr<T>
LockFreeStack<T>::Pop() {
  Node* old_head = head_.load();

  // 空指针检查需要在循环条件里：
  // CAS 失败会把 old_head 刷新成当前的 head_，
  // 然而这个值可能是 nullptr（其他线程可能 Pop 了最后一个元素）
  // 短路求值保证了 old_head 为 nullptr 时不会去求值 old_head->next
  while (old_head && !head_.compare_exchange_weak(old_head, old_head->next))
    ;

  // 走到这里 old_head 要么为空（栈空），要么归本线程独占
  return old_head ? old_head->data : nullptr;
}

} // namespace demo03

#endif // !DEMO03_LOCK_FREE_STACK_HPP
