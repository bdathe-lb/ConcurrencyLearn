#ifndef DEMO01_SPINLOCK_MUTEX_H
#define DEMO01_SPINLOCK_MUTEX_H

#include <atomic>

namespace demo01 {

// Lock()/Unlock() 的两个内存序构成一对单向屏障，把临界区夹在中间：
//
//        ┌─────────────────────────────────┐
//        │  锁外面的代码 X                  │
//        └─────────────────────────────────┘
//   ─────────── Lock()  [acquire] ───────────
//        ╎          ↓ 能穿过         ↑ 不能穿过
//        ╎  ┌─────────────────────────────┐
//        ╎  │   临 界 区                   │
//        ╎  └─────────────────────────────┘
//        ╎          ↓ 不能穿过       ↑ 能穿过
//   ─────────── Unlock() [release] ──────────
//        ┌─────────────────────────────────┐
//        │  锁外面的代码 Y                  │
//        └─────────────────────────────────┘
//
// acquire：临界区的读写不能被重排到 test_and_set 之前，
//          否则会出现「锁还没拿到就先访问共享数据」。
// release：临界区的读写不能被重排到 clear 之后，
//          否则会出现「锁已经放了，数据却还没写完」。
//
// 两者都是单向的：锁外的代码落进临界区无害，只是多受一层保护；
// 要防的是临界区的内容漏出去。单向也比双向（seq_cst）便宜。
// 二者配对后建立 synchronizes-with：持锁线程在临界区里的写，
// 对随后取得这把锁的线程可见。

class SpinlockMutex {
private:
  std::atomic_flag flag_ = ATOMIC_FLAG_INIT;

public:
  /**
   * test_and_set: 设置 flag_ 为 true，并返回原来的 flag_
   * - 原来的 flag_ 为 false：一个线程调用 Lock() 会返回 false，于是跳出循环
   *                          并持有锁，执行临界区的代码
   * - 原来的 flag_ 为 true ：说明已经有线程持有锁，其他线程调用 Lock() 会返回
   *                          true，不断的循环（自旋）
   */
  void Lock() {
    while (flag_.test_and_set(std::memory_order_acquire))
      ;
  }

  /**
   * clear: 设置 flag_ 为 false
   * 已经持有锁的线程调用 Unlock() 会设置 flag_ 为 false，于是其他正在调用
   * Lock() 的线程可以获得锁去执行临界区代码
   */
  void Unlock() {
    flag_.clear(std::memory_order_release);
  }
};

} // namespace demo01

#endif // DEMO01_SPINLOCK_MUTEX_H
