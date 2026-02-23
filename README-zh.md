# xcoroutine

一个 C++20 协程库，提供轻量、安全、高效、流畅的并发基础设施。

## 特性

- **线程池**：基于工作窃取的线程池，实现最优负载均衡
- **并发数据结构**：支持 RAII 锁语义的线程安全封装
- **锁组合**：使用 `compose<T...>` 原子性锁定多个资源
- **零拷贝设计**：极低的内存分配开销

## 环境要求

- C++20 兼容编译器 (GCC 10+, Clang 14+, MSVC 2022+)
- CMake 3.10+

## 快速开始

```cpp
#include <xc/coroutine/worker.hpp>
#include <iostream>

using namespace xc::xcoroutine;

int main() {
    // 创建工作线程
    auto worker = Worker::make();
    worker->start(0);

    // 提交任务
    worker->try_enqueue_task([]() {
        std::cout << "你好，来自工作线程！" << std::endl;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    worker->stop();
}
```

## 线程安全数据结构

```cpp
#include <xc/coroutine/access.hpp>

using namespace xc::coroutine;

// 线程安全的计数器
mutex<int> counter{0};

// 读写保护的数据
shared_mutex<std::vector<int>> data{};

// 原子性锁定多个资源
mutex<int> a{1}, b{2};
compose(a, b)([](auto& x, auto& y) {
    // 同时持有两个锁
    std::cout << "a=" << x << ", b=" << y << std::endl;
});
```

## 构建

```bash
mkdir build && cd build
cmake .. -G "Ninja Multi-Config" -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Debug

# 运行测试
ctest -C Debug --output-on-failure
```

## 架构

- `xc/coroutine/worker.hpp` - 带任务队列和工作窃取的工作线程
- `xc/coroutine/scheduler.hpp` - 多工作线程调度器，支持负载均衡
- `xc/coroutine/access.hpp` - 并发数据结构（mutex、shared_mutex、compose）
- `xc/coroutine/types.hpp` - 类型定义
- `xc/coroutine/config.hpp` - 调试配置

## 许可证

[MIT](LICENSE)
