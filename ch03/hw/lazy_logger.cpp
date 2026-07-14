#include <fstream>
#include <ios>
#include <iostream>
#include <mutex>
#include <string>

class LazyLogger {
private:
  std::string file_path_;
  std::ofstream log_file_;
  std::mutex write_mutex_;
  std::once_flag lazy_flag_;

  void InitFileSafe() {
    log_file_.open(file_path_, std::ios::app);
    if (log_file_.is_open()) {
      log_file_ << "--- Log System Initialize ---\n";
    }
  }

public:
  explicit LazyLogger(std::string file_path)
    : file_path_(std::move(file_path)) {}

  ~LazyLogger() {
    if (log_file_.is_open()) {
      log_file_.close();
    }
  }

  LazyLogger(const LazyLogger&) = delete;
  LazyLogger& operator=(const LazyLogger&) = delete;

  // Task: Implement a log writing interface 
  //       (ensure safe delayed loading and no concurrency conflicts during concurrent writes)
  void Log(const std::string& message) {
    std::call_once(lazy_flag_, &LazyLogger::InitFileSafe, this);

    std::lock_guard<std::mutex> lk(write_mutex_);
    if (log_file_.is_open()) {
      log_file_ << "[LOG]: " << message << "\n";
    }
  }
};
