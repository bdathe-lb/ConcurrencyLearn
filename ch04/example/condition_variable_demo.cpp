#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

std::mutex mtx;
std::condition_variable cv;
bool ready = false;

void ProviderThread() {
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  {
    std::lock_guard<std::mutex> lk(mtx);
    ready = true;
  }

  cv.notify_one();
}

void ConsumerThread() {
  std::unique_lock<std::mutex> lk(mtx);
  cv.wait(lk, [] { return ready; });
  std::cout << "Data is ready, processing..." << std::endl;
}

int main (int argc, char *argv[]) {
  std::thread t1(ConsumerThread);
  std::thread t2(ProviderThread);

  t1.join();
  t2.join();

  return 0;
}
