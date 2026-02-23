Under construction.......
# xcoroutine

A C++20 coroutine library providing lightweight, safe, efficient, and fluent concurrent infrastructure.

## Features

- **Thread Pool**: Work-stealing based thread pool for optimal load balancing
- **Concurrent Data Structures**: Thread-safe wrappers with RAII locking semantics
- **Lock Composition**: Atomically lock multiple resources with `compose<T...>`
- **Zero-Copy Design**: Minimal memory allocation overhead

## Requirements

- C++20 compatible compiler (GCC 10+, Clang 14+, MSVC 2022+)
- CMake 3.10+

## Quick Start

```cpp
#include <xc/coroutine/worker.hpp>
#include <iostream>

using namespace xc::xcoroutine;

int main() {
    // Create a worker thread
    auto worker = Worker::make();
    worker->start(0);

    // Submit tasks
    worker->try_enqueue_task([]() {
        std::cout << "Hello from worker!" << std::endl;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    worker->stop();
}
```

## Thread-Safe Data Structures

```cpp
#include <xc/coroutine/access.hpp>

using namespace xc::coroutine;

// Thread-safe counter
mutex<int> counter{0};

// Reader-writer protected data
shared_mutex<std::vector<int>> data{};

// Lock multiple resources atomically
mutex<int> a{1}, b{2};
compose(a, b)([](auto& x, auto& y) {
    // Both locks held simultaneously
    std::cout << "a=" << x << ", b=" << y << std::endl;
});
```

## Build

```bash
mkdir build && cd build
cmake .. -G "Ninja Multi-Config" -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Debug

# Run tests
ctest -C Debug --output-on-failure
```

## Architecture

- `xc/coroutine/worker.hpp` - Worker thread with task queue and work stealing
- `xc/coroutine/scheduler.hpp` - Multi-worker scheduler with load balancing
- `xc/coroutine/access.hpp` - Concurrent data structures (mutex, shared_mutex, compose)
- `xc/coroutine/types.hpp` - Type definitions
- `xc/coroutine/config.hpp` - Debug configuration

## License

[MIT](LICENSE)
