#ifndef DEMO05_HAZARD_POINTER_HPP
#define DEMO05_HAZARD_POINTER_HPP

#include "lock-free-stack.hpp"
#include <atomic>
#include <functional>
#include <stdexcept>
#include <thread>

namespace demo05 {

namespace {

const int kMaxHazardPointers = 100;

struct HazardPointer {
  std::atomic<std::thread::id> id;
  std::atomic<void*> pointer;
};

HazardPointer g_hazard_pointers[kMaxHazardPointers];

class HpOwner {
private:
  HazardPointer* hp_ = nullptr;

public:
  HpOwner(const HpOwner&) = delete;
  HpOwner& operator=(const HpOwner&) = delete;

  HpOwner() {
    for (int i = 0; i < kMaxHazardPointers; ++ i) {
      std::thread::id old_id;
      if (g_hazard_pointers[i].id.compare_exchange_strong(
        old_id, std::this_thread::get_id())) {
        hp_ = &g_hazard_pointers[i];
        break;
      }
    }

    if (hp_ == nullptr) {
      throw std::runtime_error("No hazard pointers available");
    }
  }

  std::atomic<void*>& GetPointer() const {
    return hp_->pointer;
  }

  ~HpOwner() {
    hp_->pointer.store(nullptr);
    hp_->id.store(std::thread::id());
  }

};

template<typename T>
void DoDelete(void* p) {
  delete static_cast<T*>(p);
}

struct DataToReclaim {
  void* data;
  std::function<void(void*)> deleter;
  DataToReclaim* next = nullptr;

  template<typename T>
  explicit DataToReclaim(T* p)
    : data(p)
    , deleter(DoDelete<T>)
    {}

  ~DataToReclaim() {
    deleter(data);
  }

  DataToReclaim(const DataToReclaim&) = delete;
  DataToReclaim& operator=(const DataToReclaim&) = delete;
};

inline std::atomic<DataToReclaim*> g_nodes_to_reclaim{nullptr};

inline void AddToReclaimList(DataToReclaim* node) {
  node->next = g_nodes_to_reclaim.load();
  while (!g_nodes_to_reclaim.compare_exchange_weak(node->next, node))
    ;
}

} // namespace

inline std::atomic<void*>& GetHazardPointerForCurrentThread() {
  thread_local HpOwner hazard;
  return hazard.GetPointer();
}

inline bool OutstandingHazardPointerFor(void *p) {
  for (int i = 0; i < kMaxHazardPointers; ++ i) {
    if (g_hazard_pointers[i].pointer.load() == p) {
      return true;
    }
  }

  return false;
}

template<typename T>
void ReclaimLater(T *data) {
  AddToReclaimList(new DataToReclaim(data));
}

inline void DeleteNodesWithNoHazards() {
  DataToReclaim* current = g_nodes_to_reclaim.exchange(nullptr);
  while (current != nullptr) {
    DataToReclaim* const next = current->next;
    if (!OutstandingHazardPointerFor(current->data)) {
      delete current;
    } else {
      AddToReclaimList(current);
    }
    current = next;
  }
}

} // namespace demo05

#endif // !DEMO05_HAZARD_POINTER_HPP

