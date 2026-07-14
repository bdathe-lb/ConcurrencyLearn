#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>

class OnlineConfigServer {
private:
  std::unordered_map<std::string, std::string> config_map_;
  mutable std::shared_mutex share_mutex_;

public:
  OnlineConfigServer() {
    // Pre-fill some default configurations
    config_map_["dp_port"] = "3306";
    config_map_["timeout"] = "5000";
  }

  // Task 1: Impliment `GetConfig` interface
  std::string GetConfig(const std::string& key) const {
    std::shared_lock<std::shared_mutex> lk(share_mutex_);

    auto it = config_map_.find(key);
    return (it == config_map_.end()) ? "" : it->second;
  }

  // Task 2: Impliment `UpdateConfig` interface
  void UpdateConfig(const std::string& key, const std::string& value) {
    std::lock_guard<std::shared_mutex> lk(share_mutex_);

    config_map_[key] = value;
  }
};
