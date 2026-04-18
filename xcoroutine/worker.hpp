#pragma once
#ifdef _MSVC_VAR
#include <vcruntime_new.h>
#endif
#include <atomic>
#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <new>
#include <queue>
#include <thread>

#include "./config.hpp"
#include "./types.hpp"

namespace xc {
namespace xcoroutine {
template <typename Policy>
class PolicyThreadWorker {
   protected:
    PolicyThreadWorker() = default;

   public:
    using task_t = typename Policy::task_t;
    struct WorkerDeleter {
        void operator()(PolicyThreadWorker* worker) const {
            ::operator delete(worker,
                              std::align_val_t(alignof(PolicyThreadWorker)));
        }
    };
    static std::unique_ptr<PolicyThreadWorker, WorkerDeleter> make() {
#if defined(_MSC_VER) && !defined(__clang__)
        PolicyThreadWorker<Policy>* worker = new (::operator new(
            sizeof(PolicyThreadWorker<Policy>),
            std::align_val_t(alignof(PolicyThreadWorker<Policy>))))
            PolicyThreadWorker<Policy>();
        return std::unique_ptr<PolicyThreadWorker<Policy>, WorkerDeleter>(
            worker);
#else
        return std::unique_ptr<PolicyThreadWorker, WorkerDeleter>(
            new (std::align_val_t(alignof(PolicyThreadWorker)))
                PolicyThreadWorker());
#endif
    }

   public:
    bool try_enqueue_task(const task_t& task) {
        assert(task && "task is invalid");
        if (queue_flag_.test_and_set(std::memory_order_acq_rel)) return false;
        task_queue_.push(task);
        ++task_count_;
        queue_flag_.clear(std::memory_order_release);
        if (wait_flag_.test_and_set()) {
            wait_flag_.clear();
            wait_flag_.notify_all();
        }
        wait_flag_.notify_all();
        wait_flag_.clear();
        return true;
    }
    inline bool try_dequeue_task(task_t& task) noexcept {
        if (queue_flag_.test_and_set(std::memory_order_acq_rel)) return false;
        if (task_queue_.empty()) {
            queue_flag_.clear(std::memory_order_release);
            return false;
        }
        task = std::move(task_queue_.front());
        task_queue_.pop();
        --task_count_;
        queue_flag_.clear(std::memory_order_release);
        return true;
    }
    void stop() {
        run_flag_.clear(std::memory_order_release);
        if (wait_flag_.test_and_set()) {
            wait_flag_.clear();
            wait_flag_.notify_all();
        }
        wait_flag_.clear();
    }
    void join() { thread_.join(); }
    bool joinable() const { return thread_.joinable(); }
    void start(uint32_t worker_id) {
        worker_id_ = worker_id;
        assert(!thread_.joinable());
        if (run_flag_.test_and_set(std::memory_order_acq_rel)) assert(false);
        thread_ = std::jthread{&PolicyThreadWorker::work, this};
    }
    inline uint32_t worker_id() const noexcept { return worker_id_; }
    inline std::thread::id thread_id() const noexcept { return thread_id_; }
    inline uint32_t task_count() const noexcept { return task_count_.load(); }
    inline void set_steal_callback(
        std::function<void(std::queue<task_t>&)> wait_notify) noexcept {
        steal_callback_ = wait_notify;
    }
    inline bool waiting() const noexcept { return wait_flag_.test(); }
    inline uint32_t wait_count() const noexcept { return wait_count_; }
    inline uint32_t max_wait_count() const noexcept { return max_wait_count_; }
    inline void set_max_wait_count(uint32_t max_wait_count) noexcept {
        max_wait_count_ = max_wait_count;
    }
    inline auto current_task_duration_us() const noexcept {
        return wait_count_
                   ? 0
                   : std::chrono::duration_cast<std::chrono::microseconds>(
                         std::chrono::steady_clock::now() -
                         current_task_start_time_)
                         .count();
    }
    inline void enable_steal() noexcept { steal_flag_ = true; }
    inline void disable_steal() noexcept { steal_flag_ = false; }
    inline bool steal_enabled() const noexcept { return steal_flag_; }

   protected:
    void work() {
        // auto tread_id_ = std::this_thread::get_id();
        _WORKER_DEBUG("Worker {} start @ {}", worker_id_, tread_id_);
        while (!wait_flag_.test_and_set()) {
            wait_flag_.notify_all();
        }
        wait_flag_.clear();
        while (run_flag_.test(std::memory_order_acquire)) {
            if (wait_count_ > max_wait_count_) {
                if (wait_flag_.test_and_set()) {
                    wait_count_ = 0;
                    continue;
                }
                wait_flag_.notify_all();
                if (queue_flag_.test_and_set()) {
                    --wait_count_;
                    continue;
                }
                if (steal_callback_ && steal_flag_) {
                    _WORKER_DEBUG("Worker {} start steal tasks", worker_id_);
                    steal_callback_(task_queue_);
                }
                auto empty = task_queue_.empty();
                queue_flag_.clear();
                wait_count_ = 1;
                if (empty) {
                    _WORKER_DEBUG("Worker {} wait ", worker_id_);
                    wait_flag_.wait(true);
                    _WORKER_DEBUG("Worker {} wake up", worker_id_);
                } else {
                    wait_flag_.clear();
                }
            }
            task_t task;
            if (queue_flag_.test_and_set(std::memory_order_acq_rel)) {
                continue;
            }
            if (!task_queue_.empty()) {
                task = std::move(task_queue_.front());
                task_queue_.pop();
                wait_count_ = 0;
            } else {
                ++wait_count_;
            }
            queue_flag_.clear(std::memory_order_release);
            if (wait_count_ == 0) {
                _WORKER_DEBUG("worker {} doing tasks , reamin {}", worker_id_,
                              task_count_.load() - 1);
                current_task_start_time_ = std::chrono::steady_clock::now();
                Policy::run_task(task);
                --task_count_;
            }
        }
        _WORKER_DEBUG("Worker {} exit @ {}", worker_id_, tread_id_);
    }

   private:
    uint32_t wait_count_{1};
    uint32_t max_wait_count_{1000};
    std::thread::id thread_id_{};
    std::queue<task_t> task_queue_{};
    std::atomic_uint32_t task_count_{0};
    bool steal_flag_{};
    uint32_t worker_id_{0};
    std::function<void(std::queue<task_t>&)> steal_callback_{};
    std::chrono::steady_clock::time_point current_task_start_time_{};
    alignas(64) std::atomic_flag run_flag_{};
    alignas(64) std::atomic_flag wait_flag_{};
    alignas(64) std::atomic_flag queue_flag_{};
    std::jthread thread_{};
};
using functional_worker_ptr =
    std::unique_ptr<FunctionalThreadWorker,
                    FunctionalThreadWorker::WorkerDeleter>;
using point_worker_ptr =
    std::unique_ptr<PointerThreadWorker, PointerThreadWorker::WorkerDeleter>;

}  // namespace xcoroutine
}  // namespace xc
