#include <functional>
#include <thread>
#include <iostream>
#include <chrono>

void background_worker(int& local_ref) {
  // simulate a time-consuming task
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  // At this point, the physical thread is still running, 
  // but the external local_state has already been destroyed when the outer function exited
  std::cout << "Value: " << local_ref << "\n";
}

void oops() {
  int local_state = 42;
  std::thread t(background_worker, std::ref(local_state));
  t.detach();
}

int main (int argc, char *argv[]) {
  oops();
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));
  return 0;
}
