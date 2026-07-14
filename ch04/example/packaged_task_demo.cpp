#include <chrono>
#include <iostream>
#include <future>
#include <string>
#include <thread>

// A regular business calculation function
int ComputeStringLength(std::string str) {
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  return str.length();
}

int main (int argc, char *argv[]) {
  // Create a task packer
  std::packaged_task<int(std::string)> task(ComputeStringLength);

  // Get future from packer
  std::future<int> result_fut = task.get_future();

  // The packer itself is Move-only; it is passed to the physical thread via std::move.  
  // After thread startup, it asynchronously invokes the task's operator(), passing the argument "Hello Concurrency!"
  std::thread t(std::move(task), "Hello Concurrency!");

  std::cout << "Main thread is free to do other things...\n";

  // Wait for the asynchronous task to complete and retrieve the result
  int length = result_fut.get();
  std::cout << "Task completed! String length is: " << length << "\n";

  t.join();
  
  return 0;
}
