#include <exception>
#include <iostream>
#include <string>
#include <thread>
#include <stdexcept>
#include <utility>

// A thread model conforming to RAII
class SafeThreadGuard {
private:
  std::thread t_;

public:
  explicit SafeThreadGuard(std::thread&& other) : t_(std::move(other)){
    if (!t_.joinable()) {
      throw std::logic_error("Thread is not joined!\n");
    }
  }

  ~SafeThreadGuard() {
    if (t_.joinable()) {
      t_.join();
    }
  }

  // Note:
  // The parameter must be in reference form; otherwise, it will lead to infinite recursion:
  // SafeThreadGuard g2(g1) -> The compiler calls the copy constructor to construct g1 
  //                        -> In order to call the copy constructor, the compiler finds that this is again a pass-by-value function,
  //                           so it has to call the copy constructor again...
  SafeThreadGuard(const SafeThreadGuard&) = delete;
  SafeThreadGuard& operator=(const SafeThreadGuard&) = delete;

  SafeThreadGuard(SafeThreadGuard&&) noexcept = default;
  SafeThreadGuard& operator=(SafeThreadGuard&&) noexcept = default;
};

void PrintMessage(const std::string& msg) {
  std::cout << "Worker thread: " << msg << "\n";
}

int main (int argc, char *argv[]) {
  try {
    std::string target = "Deep Dive C++ Concurrency!";
    // Copy the string into the independent memory space of the new thread
    std::thread t(PrintMessage, target);
    SafeThreadGuard safe_thread(std::move(t));

    throw std::runtime_error("Something went wrong in main thread!");
  } catch (const std::exception& e) {
    std::cout << "Caught exception: " << e.what() << "\n";
  }

  return 0;
}
