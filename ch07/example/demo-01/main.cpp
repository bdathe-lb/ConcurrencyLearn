#include "spinlock-mutex.h"
#include <iostream>
#include <thread>
#include <chrono>

using demo01::SpinlockMutex;

unsigned long balance = 100;
SpinlockMutex spinlock_mutex;

void Pay(int amount) {
  if (balance >= amount) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    balance -= amount;
  }
}

void SafePay(int amount) {
  spinlock_mutex.Lock();

  if (balance >= amount) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    balance -= amount;
  }

  spinlock_mutex.Unlock();
}

int main (int argc, char *argv[]) {

  std::thread t1(SafePay, 100);
  std::thread t2(SafePay, 100);

  t1.join();
  t2.join();

  std::cout << "balance is " << balance << "\n";
   
  return 0;
}
