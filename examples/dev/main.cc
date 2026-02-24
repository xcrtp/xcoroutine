#include <chrono>
#include <iostream>
#include <thread>
#include <xcoroutine/worker.hpp>

using namespace std::chrono_literals;
using namespace xc::xcoroutine;
int main() {
    auto worker = Worker::make();
    worker->start(1);
    worker->try_enqueue_task([]() { std::cout << "hello world" << std::endl; });
    std::this_thread::sleep_for(100ms);
    worker->stop();
}