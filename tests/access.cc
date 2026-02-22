
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <queue>
#include <thread>
#include <xc/coroutine/access.hpp>

using namespace xc::coroutine;
TEST(compose, shared) {
    shared_mutex<int> a{1};
    shared_mutex<int> b{1};

    std::cout << "a: " << *a.lock() << ", b: " << *b.lock() << std::endl;
}
TEST(compose, compose) {
    mutex<int> a{1};
    mutex<int> b{1};
    compose{a, b}([](auto& a, auto& b) {
        a = 2;
        b = 2;
    });
    EXPECT_EQ(*a.lock(), 2);
    EXPECT_EQ(*b.lock(), 2);
    const auto& ca = a;
    const auto& cb = b;
    compose(ca, cb)([](auto& a, auto& b) {
        std::cout << "a: " << a << ", b: " << b << std::endl;
    });
}

TEST(read_write, queue) {
    mutex<std::queue<int64_t>> queue{std::queue<int64_t>()};

    std::thread producer([&]() {
        for (int64_t i = 0; i < 1000; ++i) {
            queue.lock()->push(i);
        }
    });
    std::thread consumer([&]() {
        size_t i = 0;
        while (i < 1000) {
            if (queue.lock()->empty()) continue;
            int64_t j = queue.lock().apply([](std::queue<int64_t>& q) {
                int64_t j = q.front();
                q.pop();
                return j;
            });
            EXPECT_EQ(j, i++);
        }
    });
    producer.join();
    consumer.join();
}