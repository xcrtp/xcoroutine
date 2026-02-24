#include <print>  // IWYU pragma: export
#define DEBUG(...) std::println(__VA_ARGS__)
// #define WORKER_DEBUG
// #define SCHEDULER_DEBUG  

#ifdef WORKER_DEBUG
#define _WORKER_DEBUG DEBUG
#else
#define _WORKER_DEBUG(...)
#endif
#ifdef SCHEDULER_DEBUG
#define _SCHEDULER_DEBUG DEBUG
#else
#define _SCHEDULER_DEBUG(...)
#endif
#ifdef _DEBUG
#include <cassert>
#include <iostream> // IWYU pragma: export
#define ASSERT(cond)                                               \
    do {                                                           \
        if (!(cond))                                               \
            std::cerr << __FILE__ << ":" << __LINE__               \
                      << " ASSERT failed: " << #cond << std::endl; \
        assert(cond);                                              \
    } while (1);
#else
#define ASSERT(cond)
#endif