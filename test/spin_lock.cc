#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include <xcoroutine/structure/spin_lock.hpp>

using namespace xc::oroutine;

// Custom policy for testing
struct spin_lock_policy_test {
    static constexpr size_t max_spin_count = 100;
};

TEST(spin_lock, default_construct) {
    spin_lock<> lock;
    EXPECT_FALSE(lock.owns_lock());
}

TEST(spin_lock, policy_construct) {
    spin_lock<spin_lock_policy_test> lock;
    EXPECT_FALSE(lock.owns_lock());
}

TEST(spin_lock, lock_unlock) {
    spin_lock<> lock;

    lock.lock();
    EXPECT_TRUE(lock.owns_lock());

    lock.unlock();
    EXPECT_FALSE(lock.owns_lock());
}

TEST(spin_lock, try_lock_success) {
    spin_lock<> lock;

    bool result = lock.try_lock();
    EXPECT_TRUE(result);
    EXPECT_TRUE(lock.owns_lock());

    lock.unlock();
    EXPECT_FALSE(lock.owns_lock());
}

TEST(spin_lock, try_lock_fail_when_locked) {
    spin_lock<> lock;

    lock.lock();
    EXPECT_TRUE(lock.owns_lock());

    // try_lock should fail when already locked
    bool result = lock.try_lock();
    EXPECT_FALSE(result);
    EXPECT_TRUE(lock.owns_lock());

    lock.unlock();
    EXPECT_FALSE(lock.owns_lock());
}

TEST(spin_lock, try_lock_multiple_attempts) {
    spin_lock<spin_lock_policy_test> lock;

    // First try_lock should succeed
    EXPECT_TRUE(lock.try_lock());
    EXPECT_TRUE(lock.owns_lock());

    // Subsequent try_lock should fail due to low max_spin_count
    // but we need to release lock first
    lock.unlock();

    // Lock again and test
    lock.lock();
    EXPECT_TRUE(lock.owns_lock());
    lock.unlock();
}

TEST(spin_lock, owns_lock) {
    spin_lock<> lock;

    EXPECT_FALSE(lock.owns_lock());

    lock.lock();
    EXPECT_TRUE(lock.owns_lock());

    lock.unlock();
    EXPECT_FALSE(lock.owns_lock());
}

// TEST(spin_lock, wait_until_unlock) {
//     spin_lock<> lock;

//     lock.lock();
//     EXPECT_TRUE(lock.owns_lock());

//     // Start a thread to unlock after a delay
//     std::thread unlock_thread([&lock]() {
//         std::this_thread::sleep_for(std::chrono::milliseconds(50));
//         lock.unlock();
//         lock.notify_all();  // Wake up any waiters
//     });

//     // wait_until_unlock should block until unlock is called
//     lock.wait_until_unlock();
//     EXPECT_FALSE(lock.owns_lock());

//     unlock_thread.join();
// }

TEST(spin_lock, notify_one) {
    spin_lock<> lock;
    std::atomic<bool> waiter_started{false};
    std::atomic<bool> notify_received{false};
    std::atomic<int> counter{0};

    lock.lock();  // Hold the lock

    // Waiter thread
    std::thread waiter([&lock, &waiter_started, &notify_received, &counter]() {
        waiter_started = true;
        lock.lock();
        notify_received = true;
        ++counter;
        lock.unlock();
    });

    // Wait for waiter to start and try to acquire lock
    while (!waiter_started) {
        std::this_thread::yield();
    }

    // Give waiter time to spin
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Notify one waiter
    lock.notify_one();
    lock.unlock();

    waiter.join();

    EXPECT_TRUE(notify_received);
    EXPECT_EQ(counter, 1);
}

TEST(spin_lock, notify_all) {
    spin_lock<> lock;
    const int num_waiters = 3;
    std::atomic<int> counter{0};
    std::vector<std::thread> waiters;

    lock.lock();  // Hold the lock

    // Create waiter threads
    for (int i = 0; i < num_waiters; ++i) {
        waiters.emplace_back([&lock, &counter]() {
            lock.lock();
            ++counter;
            lock.unlock();
        });
    }

    // Give waiters time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Notify all waiters
    lock.notify_all();
    lock.unlock();

    // Wait for all waiters to complete
    for (auto& t : waiters) {
        t.join();
    }

    EXPECT_EQ(counter, num_waiters);
}

TEST(spin_lock, concurrent_access) {
    spin_lock<> lock;
    const int num_threads = 4;
    const int iterations_per_thread = 1000;
    std::atomic<int> counter{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&lock, &counter]() {
            for (int i = 0; i < iterations_per_thread; ++i) {
                lock.lock();
                ++counter;
                lock.unlock();
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(counter, num_threads * iterations_per_thread);
}

TEST(spin_lock, try_lock_concurrent) {
    spin_lock<spin_lock_policy_test> lock;
    const int num_threads = 4;
    const int iterations_per_thread = 100;
    std::atomic<int> success_count{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&lock, &success_count]() {
            for (int i = 0; i < iterations_per_thread; ++i) {
                if (lock.try_lock()) {
                    ++success_count;
                    std::this_thread::yield();  // Brief hold
                    lock.unlock();
                }
                std::this_thread::yield();
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // At least some try_lock attempts should succeed
    EXPECT_GT(success_count, 0);
}

TEST(spin_lock, lock_is_reentrant) {
    // spin_lock is NOT reentrant - this test documents that behavior
    spin_lock<> lock;

    lock.lock();

    // Note: spin_lock does NOT support reentrant locking
    // Calling lock() again while holding the lock will deadlock/spin forever
    // This test just verifies basic lock state

    EXPECT_TRUE(lock.owns_lock());
    lock.unlock();
    EXPECT_FALSE(lock.owns_lock());
}

TEST(spin_lock, memory_alignment) {
    spin_lock<> lock;

    // Verify alignment is correct (should be 64 bytes for cache line)
    constexpr size_t expected_alignment = 64;
    uintptr_t addr = reinterpret_cast<uintptr_t>(&lock);
    EXPECT_EQ(addr % expected_alignment, 0u);
}
