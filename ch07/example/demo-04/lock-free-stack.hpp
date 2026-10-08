#ifndef DEMO04_LOCK_FREE_STACK_HPP
#define DEMO04_LOCK_FREE_STACK_HPP

#include <atomic>
#include <memory>
#include <utility>

namespace demo04 {

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
  std::atomic<unsigned> threads_in_pop_{0};
  std::atomic<Node*> to_be_deleted_{nullptr}; // 存放暂时不能删除的节点

private:
  void TryReclaim(Node* old_head);
  static void DeleteNodes(Node* nodes);
  void ChainPendingNodes(Node* nodes);
  void ChainPendingNodes(Node* first, Node* last);
  void ChainPendingNode(Node* node);

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
  // 必须在读取 head_ 之前登记：
  // 只要计数 != 0，别的线程就不会 delete 本线程可能正在拿着的节点
  ++ threads_in_pop_;

  Node* old_head = head_.load();
  while (old_head && !head_.compare_exchange_weak(old_head, old_head->next))
    ;

  // 节点不会立刻删除，所以把数据移出来
  // 用 move 而不是拷贝，让数据的释放不必等节点被回收。
  //
  // 拷贝：res = old_head->data
  //
  //   res ────────────┐
  //                   ├──► [ T ]   引用计数 = 2
  //   old_head->data ─┘
  //
  //   调用者丢掉 res → 计数 = 1，T 还活着，
  //   要一直等到节点从待删除链表里被 delete 才释放
  //
  // move：res = std::move(old_head->data)
  //
  //   res ──────────────► [ T ]   引用计数 = 1
  //   old_head->data = nullptr
  //
  //   调用者丢掉 res → 计数 = 0，T 立刻释放
  std::shared_ptr<T> res;
  if (old_head) {
    res = std::move(old_head->data);
  }

  TryReclaim(old_head);
  return res;
}

template<typename T>
void
LockFreeStack<T>::TryReclaim(Node* old_head) {
  if (threads_in_pop_.load() == 1) {
    // 此刻只有本线程在 Pop
    // 把整条待删除链表原子地认领到手
    Node* nodes_to_delete = to_be_deleted_.exchange(nullptr);

    if (!-- threads_in_pop_) {
      // 认领后计数归零：
      // 没有其他线程在 Pop，
      // 之后进来的线程也拿不到已认领的节点，可以全部删除
      DeleteNodes(nodes_to_delete);
    } else if (nodes_to_delete) {
      // 在“检查计数”和“认领”之间有别的线程进了 Pop，
      // 它可能正拿着链表里的某个节点，不能删，挂回去
      ChainPendingNodes(nodes_to_delete);
    }

    // old_head 总是可删：
    // - 先来的线程（在 CAS 之前读过 head_、可能拿着 old_head）一定计入了
    //   threads_in_pop_，计数为 1 说明它们都已离开 Pop
    // - 后来的线程读 head_ 时，old_head 早已摘离栈顶，根本拿不到
    // 待删除链表里的节点不满足这一点：它们可能是在计数检查之后才被
    // 别的线程摘下来的，所以要靠上面的二次检查（计数归零）才能删
    delete old_head;
  } else {
    // 还有别的线程在 Pop，它们可能拿着 old_head，只能先挂起来
    ChainPendingNode(old_head);
    -- threads_in_pop_;
  }
}

template<typename T>
void 
LockFreeStack<T>::DeleteNodes(Node* nodes) {
  while (nodes) {
    Node* next = nodes->next; 
    delete nodes;
    nodes = next;
  }
}

template<typename T>
void
LockFreeStack<T>::ChainPendingNodes(Node* nodes) {
  // 找到这段链表的尾节点
  Node* last = nodes;
  while (Node* const next = last->next) {
    last = next; 
  }

  ChainPendingNodes(nodes, last);
}

template<typename T>
void
LockFreeStack<T>::ChainPendingNodes(Node* first, Node* last) {
  // 和 Push 同一个思路：把 [first, last] 整段挂到 to_be_deleted_ 前面
  last->next = to_be_deleted_.load();
  while (!to_be_deleted_.compare_exchange_weak(last->next, first))
    ;
}

template<typename T>
void
LockFreeStack<T>::ChainPendingNode(Node* node) {
  ChainPendingNode(node, node);
}

} // namespace demo04

#endif // !DEMO04_LOCK_FREE_STACK_HPP
