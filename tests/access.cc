#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <iostream>
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
        EXPECT_EQ(a, 1);
        EXPECT_EQ(b, 1);
        a = 2;
        b = 2;
    });
    EXPECT_EQ(*a.lock(), 2);
    EXPECT_EQ(*b.lock(), 2);
    *a.lock() = 11;
    *b.lock() = 12;
    std::cout << "a: " << *a.lock() << ", b: " << *b.lock() << std::endl;
    const auto& ca = a;
    const auto& cb = b;
    compose(ca, cb)([](auto& a, auto& b) {
        std::cout << "a: " << a << ", b: " << b << std::endl;
        EXPECT_EQ(a, 11);
        EXPECT_EQ(b, 12);
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
struct MyClass {
    MyClass(int a) : a_(a) {}
    MyClass(const MyClass&) = delete;
    MyClass(MyClass&&) = delete;
    MyClass& operator=(const MyClass&) = delete;
    MyClass& operator=(MyClass&&) = delete;
    MyClass& operator=(int o) {
        a_ = o;
        return *this;
    }
    int a_{};
};

TEST(tuple, forword) {
    mutex<MyClass> a{0}, b{1};
    // EXPECT_EQ(a.lock()->a_, 0);
    // EXPECT_EQ(b.lock()->a_, 1);

    // int* a, *b;
    std::cout <<"real " << &a << std::endl;
    std::cout <<"real " << &b << std::endl;
    compose{a, b}([](auto& a, auto& b) {
        std::cout <<"call "<< (void*)&a << std::endl;
        std::cout <<"call "<< (void*)&b << std::endl;
        EXPECT_EQ(a.a_, 0);
        EXPECT_EQ(b.a_, 1);
        a = 2;
        b = 2;
    });
    // compose c{a, b};

    // auto s = transform(c.refs_, [](auto&& ref) {
    //     std::cout << " compose_deref<decltype(ref)>::deref(ref) "
    //               << (void*)&(*ref) << std::endl;
    //     return std::ref(*ref);
    // });
}