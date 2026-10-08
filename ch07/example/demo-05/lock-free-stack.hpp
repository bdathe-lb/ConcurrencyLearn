#ifndef DEMO05_LOCK_FREE_STACK_HPP
#define DEMO05_LOCK_FREE_STACK_HPP

#include <atomic>
#include <memory>
#include <utility>

namespace demo05 {

// 返回本线程专属的风险指针，所有线程都能看到它
std::atomic<void*>& GetHazardPointerForCurrentThread();
// 是否有任何线程的风险指针指向 p
bool OutstandingHazardPointerFor(void* p);
// 暂时不能删除的对象放入待回收链表，稍后再删除
template<typename T>
void ReclaimLater(T* data);
// 扫描待回收链表，删除已经没有风险指针指向的对象
void DeleteNodesWithNoHazards();

template<typename T>
class LockFreeStack {
private:
  struct Node {
    std::shared_ptr<T> data;
    Node* next = nullptr;

    explicit Node(const T& value)
      : data(std::make_shared<T>(value))
    {}
  }; // struct Node

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
  Node* const new_node = new Node(data);
  new_node->next = head_.load();
  while (!head_.compare_exchange_weak(new_node->next, new_node))
    ;
}

template<typename T>
std::shared_ptr<T>
LockFreeStack<T>::Pop() {
  std::atomic<void*>& hp = GetHazardPointerForCurrentThread();
  Node* old_head = head_.load();

  // 外层：
  // CAS 把 old_head 摘离栈顶，失败时， old_head 变成新的 head_，
  // 此时需要回到内层把它重新登记为风险指针
  // 由于一次失败就需要重做一次内层循环，所以 weak 的伪失败是浪费
  do {
    // 内层：
    // 先登记风险指针，再确认 head_ 没变
    // 删除者总是先摘下节点、再扫描风险指针。登记后 head_ 仍是它，
    // 说明它还没被摘下，于是任何删除者的扫描都发生在登记之后，一定能看到
    Node* temp;
    do {
      temp = old_head;
      // hp.store(X) 表示本线程声明 X 可能正在被使用，别的线程不得 delete 它（但仍可以把它弹出）
      hp.store(old_head);
      old_head = head_.load();
    } while (old_head != temp);
  } while (old_head && !head_.compare_exchange_strong(old_head, old_head->next));

  // 节点已被本线程摘下，只有本线程会删它，不再需要风险指针保护
  hp.store(nullptr);

  std::shared_ptr<T> res;
  if (old_head) {
    res = std::move(old_head->data);

    // 别的线程可能在本线程 CAS 之前就对它登记了风险指针，还没来得及撤销
    if (OutstandingHazardPointerFor(old_head)) {
      ReclaimLater(old_head);
    } else {
      delete old_head;
    }

    // 顺便处理之前挂起的节点：已经没人登记的就删掉，其余留给下一次 Pop
    DeleteNodesWithNoHazards();
  }
  return res;
}

} // namespace demo05

#endif // !DEMO05_LOCK_FREE_STACK_HPP
