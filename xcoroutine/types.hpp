#include <functional>

namespace xc {
namespace xcoroutine {
template <typename Policy>
class PolicyThreadWorker;
struct functional_task_policy {
    using task_t = std::function<void(void)>;
    static inline void run_task(task_t& task) { task(); }
};
struct pointer_task_policy {
    using task_t = void (*)(void);
    static inline void run_task(const task_t& task) { task(); }
};
using FunctionalThreadWorker = PolicyThreadWorker<functional_task_policy>;
using PointerThreadWorker = PolicyThreadWorker<pointer_task_policy>;

}  // namespace xcoroutine
}  // namespace xc
