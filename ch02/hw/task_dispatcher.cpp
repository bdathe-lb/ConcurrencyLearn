#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <thread>

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

struct BigPayload {
  std::string data;
  explicit BigPayload(std::string msg) : data(std::move(msg)) {}
};

void WorkerProcess(std::unique_ptr<BigPayload> payload, int& sucess_count) {
  if (payload && !payload->data.empty()) {
    std::cout << "[Worker] Processing unique payload: " << payload->data << "\n";
    ++ sucess_count;
  }
}

int main (int argc, char *argv[]) {
  int total_success = 0;
  
  {
    char network_buffer[256];
    std::strcpy(network_buffer, "Encrypted Quantum Packet");

    std::unique_ptr<BigPayload> payload (new BigPayload(std::string(network_buffer)));
    std::thread t(WorkerProcess, std::move(payload), std::ref(total_success));
    SafeThreadGuard safe_thread(std::move(t));
  } 

  std::cout << "[Main] Total tasks successfully processed: " << total_success << "\n";

  return 0;
}
