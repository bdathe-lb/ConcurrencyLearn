#include <exception>
#include <future>
#include <iostream>
#include <string>
#include <thread>

// Work thread function
void StringToIntWorker(std::string str, std::promise<int> ans_promise) {
  try {
    // Use `std::stoi` to parse string
    int res = std::stoi(str, nullptr, 10);
    ans_promise.set_value(res);
  } catch (...) {
    ans_promise.set_exception(std::current_exception());
  }
}

int main (int argc, char *argv[]) {
  {
    std::promise<int> p;
    std::future<int> f = p.get_future();

    // Start a thread
    std::thread t(StringToIntWorker, "1234567", std::move(p));

    t.join();

    try {
      std::cout << "Result: " << f.get() << "\n";
    } catch (const std::exception& e) {
      std::cout << e.what() << "\n";
    }
  }

  std::cout << "---------------------------\n";

  {
    std::promise<int> p;
    std::future<int> f = p.get_future();

    // Start a thread
    std::thread t(StringToIntWorker, "not_a_number", std::move(p));

    t.join();

    try {
      std::cout << "Result: " << f.get() << "\n";
    } catch (const std::exception& e) {
      std::cout << e.what() << "\n";
    }
  }
  
  return 0;
}
