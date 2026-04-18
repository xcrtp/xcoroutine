#pragma once
#include <atomic>
#include <cstddef>
namespace xc {
namespace oroutine {
struct spin_lock_policy_default {
    static constexpr size_t max_spin_count = 1000;
};

template <typename Policy = spin_lock_policy_default>
class spin_lock {
   public:
    spin_lock() = default;
    ~spin_lock() = default;

    bool try_lock() {
        for (size_t i = 0; i < Policy::max_spin_count; ++i) {
            if (!flag_.test_and_set(std::memory_order_acq_rel)) {
                return true;
            }
        }
        return false;
    }
    bool owns_lock() const { return flag_.test(std::memory_order_acquire); }
    void unlock() {
        flag_.clear(std::memory_order_release);
        notify_one();
    }
    void notify_one() { flag_.notify_one(); }
    void notify_all() { flag_.notify_all(); }
    void lock() {
        while (flag_.test_and_set(std::memory_order_acq_rel)) {
            flag_.wait(true, std::memory_order_acquire);
        }
    }

   private:
    alignas(64) std::atomic_flag flag_{};
};

}  // namespace oroutine
}  // namespace xc