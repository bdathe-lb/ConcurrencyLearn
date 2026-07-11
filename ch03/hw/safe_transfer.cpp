#include <iostream>
#include <mutex>

// A bank account class
class BankAccount {
private:
  int id_;
  double balance_;
  mutable std::mutex m_;

public:
  BankAccount(int id, double balance)
    : id_(id)
    , balance_(balance) {}

  friend void Transfer(BankAccount& from, BankAccount& to, double amount) {
    // 'from' and 'to' refer to the same account
    // Note: In C++, the safest way to check if two objects are the same instance is 
    //       to compare their memory addresses.
    if (&from == &to) return;

    // For C++ 11
    // std::lock(from.m_, to.m_);
    // std::lock_guard<std::mutex> lock_from(from.m_, std::adopt_lock);
    // std::lock_guard<std::mutex> lock_to(to.m_, std::adopt_lock);

    // For C++ 17
    std::scoped_lock guard(from.m_, to.m_);

    if (from.balance_ < amount) {
      throw std::runtime_error("Insufficient balance for transfer.");
    }

    from.balance_ -= amount;
    to.balance_ += amount;
  }

  void PrintBalance() const {
    std::lock_guard<std::mutex> lk(m_);
    std::cout << "Account " << id_ << " Balance: " << balance_ << "\n";
  }
};
