#include <map>
#include <string>
#include <mutex>
#include <shared_mutex>

class DNSCache {
private:
  std::map<std::string, std::string> entries_;
  mutable std::shared_mutex entry_mutex_;

public:
  // Reader interface: concurrent domain lookup
  std::string FindEntry(const std::string& domain) const {
    // Use std::shared_lock to acquire a shared lock (read lock)
    std::shared_lock<std::shared_mutex> lk(entry_mutex_);

    auto it = entries_.find(domain);
    return (it == entries_.end()) ? "" : it->second;
  }

  // Author interface: Update or add a domain
  void UpdateOrAddEntry(const std::string& domain, const std::string& ip_details) {
    // Use the standard std::lock_guard to acquire an exclusive lock (write lock).
    std::lock_guard<std::shared_mutex> lk(entry_mutex_);

    entries_[domain] = ip_details;
  }
};
