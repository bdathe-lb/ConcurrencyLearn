#ifndef THREADSAFE_QUEUE_SHARED_PTR_H
#define THREADSAFE_QUEUE_SHARED_PTR_H

/**
 * 《C++ 并发编程实战》清单 6.3：单锁 + 条件变量的线程安全队列，队列中存储 shared_ptr<T>。
 *
 * 相对清单 6.2（threadsafe-queue.h）的改动：shared_ptr<T> 的构造从 WaitAndPop() 移到 Push()。
 *
 * 效果：
 *   - 可能抛出异常的操作都发生在 notify_one 之前：make_shared 在加锁前执行，
 *     data_queue_.push 在锁内也可能因分配内存失败而抛出异常。无论哪一步失败，
 *     元素都未入队、也未唤醒任何消费者，异常直接传给生产者。
 *   - WaitAndPop() 被唤醒后只需移动队首的 shared_ptr，该操作不抛出异常，
 *     因此不会浪费 notify_one 发出的唤醒。
 *   - 内存分配移到锁外，缩短了持锁时间。
 *
 * 适用范围：上述"唤醒不被浪费"的保证只覆盖返回 shared_ptr 的 WaitAndPop()。
 * WaitAndPop(T&) 仍需执行 value = std::move(*front())，调用 T 的移动赋值；
 * 只有在 T 的移动赋值不抛出异常的前提下，它才不会浪费唤醒。
 */

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <utility>

template<typename T>
class ThreadSafeQueueSharedPtr {
private:
  std::queue<std::shared_ptr<T>> data_queue_;
  std::condition_variable data_cond_;
  mutable std::mutex mut_;

public:
  ThreadSafeQueueSharedPtr() = default;

  void Push(T new_value) {
    std::shared_ptr<T> data(std::make_shared<T>(std::move(new_value)));
    std::lock_guard<std::mutex> lk(mut_);
    data_queue_.push(data);
    data_cond_.notify_one();
  }

  void WaitAndPop(T& value) {
    std::unique_lock<std::mutex> lk(mut_);
    data_cond_.wait(lk, [this] { return !data_queue_.empty(); });
    value = std::move(*data_queue_.front());
    data_queue_.pop();
  }

  std::shared_ptr<T> WaitAndPop() {
    std::unique_lock<std::mutex> lk(mut_);
    data_cond_.wait(lk, [this] { return !data_queue_.empty(); });
    std::shared_ptr<T> res = std::move(data_queue_.front());
    data_queue_.pop();
    return res;
  }

  bool TryPop(T& value) {
    std::lock_guard<std::mutex> lk(mut_);
    if (data_queue_.empty()) 
      return false;

    value = std::move(*data_queue_.front());
    data_queue_.pop();
    return true;
  }

  std::shared_ptr<T> TryPop() {
    std::lock_guard<std::mutex> lk(mut_);
    if (data_queue_.empty())
      return std::shared_ptr<T>();

    std::shared_ptr<T> res = std::move(data_queue_.front());
    data_queue_.pop();
    return res;
  }

  bool Empty() const {
    std::lock_guard<std::mutex> lk(mut_);
    return data_queue_.empty();
  }

};

#endif // !THREADSAFE_QUEUE_SHARED_PTR_H
