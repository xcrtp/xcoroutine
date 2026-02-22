#pragma once
#include <cassert>
#include <mutex>
#include <shared_mutex>
#include <tuple>
#include <type_traits>
#include <utility>

namespace xc {
namespace coroutine {
template <typename T, typename Derived>
class const_mutex_ref_impl {
   public:
    const T& operator*() const {
        return static_cast<const Derived*>(this)->ref_;
    }
    const T* operator->() const {
        return &(static_cast<const Derived*>(this)->ref_);
    }
    template <typename Fn>
        requires std::is_invocable_v<Fn, const T&>
    auto apply(Fn&& fn) const {
        return fn(
            const_cast<const T&>(static_cast<const Derived*>(this)->ref_));
    }
};

template <typename T, typename Derived>
class mutex_ref_impl {
   public:
    T& operator*() const { return static_cast<const Derived*>(this)->ref_; }
    T* operator->() const { return &(static_cast<const Derived*>(this)->ref_); }
    template <typename Fn>
        requires std::is_invocable_v<Fn, T&>
    auto apply(Fn&& fn) const {
        return fn(static_cast<const Derived*>(this)->ref_);
    }
};
template <typename T, typename mutex_ = std::mutex>
class const_mutex_ref
    : public const_mutex_ref_impl<T, const_mutex_ref<T, mutex_>> {
    friend class const_mutex_ref_impl<T, const_mutex_ref<T, mutex_>>;
    using Mutex = mutex_;

   public:
    const_mutex_ref(const T& ref, Mutex& mutex) : ref_(ref), lock_(mutex) {}

   public:
    ~const_mutex_ref() = default;
    const_mutex_ref(const_mutex_ref&&) = default;
    const_mutex_ref(const const_mutex_ref&) = delete;
    const_mutex_ref& operator=(const_mutex_ref&&) = default;
    const_mutex_ref& operator=(const const_mutex_ref&) = delete;

   protected:
    const T& ref_;
    std::lock_guard<Mutex> lock_;
};
template <typename T, typename mutex_ = std::mutex>
class mutex_ref : public mutex_ref_impl<T, mutex_ref<T, mutex_>> {
    friend class mutex_ref_impl<T, mutex_ref<T, mutex_>>;
    using Mutex = mutex_;

   public:
    mutex_ref(T& ref, Mutex& mutex) : ref_(ref), lock_(mutex) {}

   public:
    ~mutex_ref() = default;
    mutex_ref(mutex_ref&&) = default;
    mutex_ref(const mutex_ref&) = delete;
    mutex_ref& operator=(mutex_ref&&) = default;
    mutex_ref& operator=(const mutex_ref&) = delete;

   public:
   protected:
    T& ref_;
    std::lock_guard<Mutex> lock_;
};
template <typename T>
class mutex_ref<T, std::shared_mutex>
    : public mutex_ref_impl<T, mutex_ref<T, std::shared_mutex>> {
    friend class mutex_ref_impl<T, mutex_ref<T, std::shared_mutex>>;
    using Mutex = std::shared_mutex;

   public:
    mutex_ref(T& ref, Mutex& mutex) : ref_(ref), lock_(mutex) { lock_.lock(); }
    ~mutex_ref() { lock_.unlock(); }

   private:
    T& ref_;
    std::shared_mutex& lock_;
};
template <typename T>
class const_mutex_ref<T, std::shared_mutex>
    : public const_mutex_ref_impl<T, const_mutex_ref<T, std::shared_mutex>> {
    friend class const_mutex_ref_impl<T, const_mutex_ref<T, std::shared_mutex>>;
    using Mutex = std::shared_mutex;

   public:
    const_mutex_ref(const T& ref, Mutex& mutex) : ref_(ref), lock_(mutex) {
        lock_.lock_shared();
    }
    ~const_mutex_ref() { lock_.unlock_shared(); };

   private:
    const T& ref_;
    std::shared_mutex& lock_;
};

template <typename T, typename Derive, typename Mutex = std::mutex>
class lock_impl {
   public:
    template <typename... Args>
        requires(std::is_constructible_v<T, Args...>)
    lock_impl(Args&&... args) : data_(args...) {}
    auto lock() const {
        return const_mutex_ref<T, Mutex>{
            static_cast<const Derive*>(this)->data_,
            static_cast<const Derive*>(this)->mutex_};
    }
    auto lock() {
        return mutex_ref<T, Mutex>{static_cast<Derive*>(this)->data_,
                                   static_cast<Derive*>(this)->mutex_};
    }
    T data_;
};
template <typename T>
class mutex : public lock_impl<T, mutex<T>> {
    friend class lock_impl<T, mutex<T>>;

   public:
    using lock_impl<T, mutex<T>>::lock_impl;

   private:
    mutable std::mutex mutex_{};
};
template <typename T>
class shared_mutex;
template <typename T>
using shared_mutex_impl = lock_impl<T, shared_mutex<T>, std::shared_mutex>;
template <typename T>
class shared_mutex : public shared_mutex_impl<T> {
    friend shared_mutex_impl<T>;

   public:
    using shared_mutex_impl<T>::shared_mutex_impl;

    auto read() const { return lock(); }
    auto write() { return lock(); }

   private:
    mutable std::shared_mutex mutex_{};
};

template <typename Fn, typename... Fns, typename... T>
auto transform(std::tuple<T...>& refs, Fn&& fn, Fns&&... fns) {
    auto res = std::apply(
        [&](auto&&... args) {
            return std::make_tuple<std::invoke_result_t<Fn, decltype(args)>...>(
                std::forward<Fn>(fn)(std::forward<decltype(args)>(args))...);
        },
        refs);
    if constexpr (sizeof...(Fns) == 0) {
        return res;
    } else {
        return transform(res, std::forward<Fns>(fns)...);
    }
}

template <typename T>
struct compose_deref {
    static auto deref(T&& v) { return std::ref(*v); }
};
template <typename T>
struct compose_construct_transform {
    static decltype(auto) transform(T&& v) { return std::forward<T>(v); }
};
template <typename T>
struct compose_construct_transform<mutex<T>> {
    static decltype(auto) transform(mutex<T>& v) { return v.lock(); }
};
template <typename T>
struct compose_construct_transform<mutex<T>&>
    : compose_construct_transform<mutex<T>> {};
template <typename T>
struct compose_construct_transform<const mutex<T>> {
    static decltype(auto) transform(const mutex<T>& v) { return v.lock(); }
};
template <typename T>
struct compose_construct_transform<const mutex<T>&>
    : compose_construct_transform<const mutex<T>> {};

template <typename... T>
class compose {
    using inner_type = std::tuple<
        const std::decay_t<decltype(compose_construct_transform<T>::transform(
            std::declval<T>()))>&...>;

   public:
    compose(T&&... refs)
        : refs_(std::forward_as_tuple(compose_construct_transform<T>::transform(
              std::forward<T>(refs))...)) {}
    ~compose() = default;

    template <typename Fn>
    void operator()(Fn&& fn) {
        std::apply(fn, transform(refs_, [](auto& ref) -> decltype(auto) {
                       return compose_deref<decltype(ref)>::deref(ref);
                   }));
    }

   private:
    inner_type refs_;
};
template <typename... T>
compose(T&&... refs) -> compose<T...>;

template <typename Fn, typename... T>
void compose_apply(Fn&& fn, T&&... refs) {
    compose{std::forward<T>(refs)...}(std::forward<Fn>(fn));
}
}  // namespace coroutine
}  // namespace xc