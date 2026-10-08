#ifndef DEMO02_LOCK_FREE_STACK_HPP
#define DEMO02_LOCK_FREE_STACK_HPP

#include <atomic>
namespace demo02 {

template<typename T>
class LockFreeStack {
private:
  struct Node {
    T data;
    Node* next = nullptr;

    explicit Node(const T& value)
      : data(value)
    {}
  }; // class Node

  std::atomic<Node*> head_{nullptr};

public:
  LockFreeStack() = default;

  LockFreeStack(const LockFreeStack&) = delete;
  LockFreeStack& operator=(const LockFreeStack&) = delete;

  void Push(const T& data);
  void Pop(T& result);
}; // class LockFreeStack

template<typename T>
void
LockFreeStack<T>::Push(const T& data) {
  // 节点要保证在接入链表之前就完全构造好
  // 一旦 head_ 指向它，其他线程立马可以看到
  Node* const new_node = new Node(data);

  // 先读取一份当前的 head_，作为新节点的 next
  new_node->next = head_.load();

  // CAS 循环
  // bool compare_exchange_*(T& expected, T desired);
  // - 当前值 == expected，把 desired 存入当前值，返回 true
  // - 当前值 != expected，把 expected 更新为当前值，返回 false
  // 保证了只有一个人能改 head_（最初的时候 head_ == expected）
  // 
  // NOTE:
  // 这里的 CAS 比较是做位模式比较，不碰目标内存
  while (!head_.compare_exchange_weak(new_node->next, new_node))
    ;
}

template<typename T>
void
LockFreeStack<T>::Pop(T& result) {
  // 先保存 head_ 的指向（待 Pop 的节点）
  Node* old_head = head_.load();

  // 问题1：
  // 空栈时，old_head->next 是未定义行为
  while (!head_.compare_exchange_weak(old_head, old_head->next))
    ;

  // 问题2：
  // 如果抛出异常，元素凭空消失
  result = old_head->data;

  // 问题3：
  // 节点泄漏，CAS 成功后从不 delete old_head，弹出的节点永远不回收
  // 这是刻意保留的，一旦 delete，会出现两个新问题：
  // - 其他线程手里的 old_head 变为悬空（use-after-free）
  // - ABA：被释放的地址会被 new 复用，而 CAS 只比较指针值，
  //        于是“A 走了又回来”被误判成“A 从没动过”，
  //        CAS 成功，head_ 被更新成一个早已 delete 的 next
  //   ABA 例子（初始 head -> A -> B -> C）：
  //     T1  old_head = A，读出 old_head->next = B，此时被抢占
  //     T2  弹出 A 并 delete A，再弹出 B 并 delete B     head -> C
  //     T3  Push 新节点，new 恰好复用了 A 的地址（记作 A'）head -> A'
  //     T1  恢复，CAS(expected = A, desired = B)
  //         head_ 的值确实还是 A 这个地址 -> CAS 成功
  //         head_ 被改成 B，而 B 早已 delete，栈就此损坏
  // 不 delete 就没有地址复用，A 不可能再出现在 head_ 上，ABA 无从发生
}

} // namespace demo02

#endif // !DEMO02_LOCK_FREE_STACK_HPP
