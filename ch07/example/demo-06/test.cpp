#include <atomic>
#include <iostream>
#include <memory>

int main (int argc, char *argv[]) {
  std::shared_ptr<int> p = std::make_shared<int>(0);
  std::cout << std::boolalpha 
            << std::atomic_is_lock_free(&p) << "\n";

  return 0;
}
