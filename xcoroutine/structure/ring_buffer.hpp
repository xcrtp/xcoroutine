#pragma once
#include <atomic>
#include <cstddef>
#include <memory>
namespace xc {
namespace coroutine {
template <typename T, size_t Capacity>
class ring_buffer {
    static_assert(Capacity > 0, "Capacity must be positive");
    static constexpr size_t capacity_mask = Capacity - 1;
    static_assert((Capacity & capacity_mask) == 0,
                  "Capacity must be power of 2");

   public:
    ring_buffer() : buffer_(std::make_unique<slot[]>(Capacity)) {}

   public:
    ring_buffer(const ring_buffer&) = delete;
    ring_buffer(ring_buffer&&) = delete;
    ring_buffer& operator=(const ring_buffer&) = delete;
    ring_buffer& operator=(ring_buffer&&) = delete;

   public:
    template <typename... Args>
        requires(std::is_constructible_v<T, Args && ...>)
    bool enqueue(Args&&... args) {
        size_t current_tail = tail_.load(std::memory_order_relaxed);
        size_t next_tail;
        slot* current_slot;

        do {
            current_slot = &buffer_[current_tail & capacity_mask];
            // 检查槽是否已被占用
            if (current_slot->ready.load(std::memory_order_acquire)) {
                return false;  // 队列已满
            }
            next_tail = current_tail + 1;
        } while (!tail_.compare_exchange_weak(current_tail, next_tail,
                                              std::memory_order_acq_rel,
                                              std::memory_order_relaxed));
        // 标记槽为已占用
        current_slot->ready.store(true, std::memory_order_release);
        // 构造元素
        new (&current_slot->data) T(std::forward<Args>(args)...);
        return true;
    }

    bool dequeue(T& value) {
        size_t current_head = head_.load(std::memory_order_relaxed);
        size_t next_head;
        slot* current_slot;

        do {
            current_slot = &buffer_[current_head & (Capacity - 1)];

            // 检查槽是否就绪
            if (!current_slot->ready.load(std::memory_order_acquire)) {
                return false;  // 队列为空
            }

            next_head = current_head + 1;
        } while (!head_.compare_exchange_weak(current_head, next_head,
                                              std::memory_order_acq_rel,
                                              std::memory_order_relaxed));

        // 提取数据
        value = std::move(current_slot->data);

        // 析构并标记为空
        current_slot->data.~T();
        current_slot->ready.store(false, std::memory_order_release);
        return true;
    }
    size_t size() const noexcept {
        return tail_.load(std::memory_order_acquire) -
               head_.load(std::memory_order_acquire);
    }
    bool empty() const noexcept { return size() == 0; }

    bool full() const noexcept { return size() >= Capacity; }

   private:
    struct alignas(64) slot {
        std::atomic<bool> ready{false};
        T data;
    };
    std::unique_ptr<slot[]> buffer_;
    std::atomic<size_t> head_{0};
    std::atomic<size_t> tail_{0};
};

}  // namespace coroutine
}  // namespace xc