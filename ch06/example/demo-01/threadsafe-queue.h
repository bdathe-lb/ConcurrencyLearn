#ifndef THREADSAFE_QUEUE_H
#define THREADSAFE_QUEUE_H

/**
 * 《C++ 并发编程实战》清单 6.2：单锁 + 条件变量的线程安全队列，队列中直接存储 T。
 *
 * 缺陷：被 notify_one 唤醒的消费者若在取数据时抛出异常，这次唤醒即被浪费。
 *
 * WaitAndPop() 持锁构造 shared_ptr<T>，有两处可能抛出异常：
 *   - make_shared 分配内存失败，抛出 std::bad_alloc；
 *   - T 的移动构造函数抛出异常。
 *
 * 场景：消费者 A、B、C 均在 wait 中睡眠。生产者 Push 一个元素，notify_one 只唤醒 A。
 * A 构造 shared_ptr<T> 时抛出异常，pop() 未执行，元素仍留在队列中；异常传出时
 * unique_lock 析构并解锁，A 离开 WaitAndPop。B、C 仍在等待通知，而唯一的唤醒已被 A 耗尽。
 * 若此后不再有 Push，B、C 将永久睡眠，队列中的元素无人取走。
 *
 * WaitAndPop(T&) 存在同样的问题：value = std::move(front()) 调用 T 的移动赋值，
 * 抛出异常时 pop() 同样未执行，唤醒同样被浪费；且队首元素可能已被部分移走，
 * 其状态取决于 T 的移动赋值提供的异常保证。
 *
 * TryPop 不受影响：它不调用 wait，不消耗任何唤醒，抛出异常只会传给调用者。
 *
 * 三种修复方案：
 *   1. Push 改用 notify_all：所有消费者由"等待通知"转为"等待锁"，A 解锁后 B、C 依次
 *      获得锁并重新检查队列。代价是每次 Push 唤醒全部消费者，未取到数据的线程重新睡眠，
 *      浪费 CPU。
 *   2. WaitAndPop 捕获异常后调用 notify_one 再重新抛出，把唤醒转交给下一个消费者。
 *      代价是每个等待函数都要编写这段逻辑。
 *   3. 队列改为存储 shared_ptr<T>，在 Push 中构造，使可能抛出异常的操作发生在
 *      notify_one 之前。见 threadsafe-queue-shared-ptr.h（清单 6.3）。
 */

#include <memory>
#include <mutex>
#include <utility>
#include <queue>
#include <condition_variable>

template<typename T>
class ThreadSafeQueue {
private:
  std::queue<T> data_queue_;
  std::condition_variable data_cond_;
  mutable std::mutex mut_;

public:
  ThreadSafeQueue() = default;

  void Push(T new_value) {
    std::lock_guard<std::mutex> lk(mut_);
    data_queue_.push(std::move(new_value));
    data_cond_.notify_one();
  }

  void WaitAndPop(T& value) {
    std::unique_lock<std::mutex> lk(mut_);
    data_cond_.wait(lk, [this] { return !data_queue_.empty(); });

    value = std::move(data_queue_.front());
    data_queue_.pop();
  }

  std::shared_ptr<T> WaitAndPop() {
    std::unique_lock<std::mutex> lk(mut_);
    data_cond_.wait(lk, [this] { return !data_queue_.empty(); });
    std::shared_ptr<T> res(
      std::make_shared<T>(std::move(data_queue_.front()))
    );
    data_queue_.pop();
    return res;
  }

  bool TryPop(T& value) {
    std::lock_guard<std::mutex> lk(mut_);
    if (data_queue_.empty()) 
      return false;

    value = std::move(data_queue_.front());
    data_queue_.pop();
    return true;
  }

  std::shared_ptr<T> TryPop() {
    std::lock_guard<std::mutex> lk(mut_);
    if (data_queue_.empty())
      return std::shared_ptr<T>();

    std::shared_ptr<T> res(
      std::make_shared<T>(std::move(data_queue_.front()))
    );
    data_queue_.pop();
    return res;
  }

  bool Empty() const {
    std::lock_guard<std::mutex> lk(mut_);
    return data_queue_.empty();
  }
};

#endif // !THREADSAFE_QUEUE_H
