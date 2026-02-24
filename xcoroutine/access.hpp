#pragma once
#include <cassert>
#include <cstddef>
#include <iostream>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <tuple>
#include <type_traits>
#include <utility>
#include <xcmixin/xcmixin.hpp>

namespace xc {
namespace coroutine {
XCMIXIN_DEF_BEGIN(const_mutex_ref_impl)
const auto& operator*() const { return xcmixin_const_self.ref_; }
const auto* operator->() const { return &xcmixin_const_self.ref_; }
template <typename Fn>
auto apply(Fn&& fn) const {
    return fn(
        const_cast<const std::decay_t<decltype(xcmixin_const_self.ref_)>&>(
            xcmixin_const_self.ref_));
}
XCMIXIN_DEF_END()

XCMIXIN_DEF_BEGIN(mutex_ref_impl)
auto& operator*() const { return xcmixin_const_self.ref_; }
auto* operator->() const { return &xcmixin_const_self.ref_; }
template <typename Fn>
auto apply(Fn&& fn) const {
    return fn(xcmixin_const_self.ref_);
}
XCMIXIN_DEF_END()

template <typename T, typename mutex_ = std::mutex>
class const_mutex_ref : public xcmixin::impl_mixin<const_mutex_ref<T, mutex_>,
                                                   const_mutex_ref_impl> {
   public:
    using mutex_type = mutex_;
    const_mutex_ref(const T& ref, mutex_type& mutex)
        : ref_(ref),
          lock_(std::make_unique<std::lock_guard<mutex_type>>(mutex)) {}
    const_mutex_ref(T& ref, mutex_type& mutex, std::adopt_lock_t)
        : ref_(ref), lock_(mutex, std::adopt_lock_t{}) {}

   public:
    ~const_mutex_ref() = default;
    const_mutex_ref(const_mutex_ref&&) = default;
    const_mutex_ref(const const_mutex_ref&) = delete;
    const_mutex_ref& operator=(const_mutex_ref&&) = default;
    const_mutex_ref& operator=(const const_mutex_ref&) = delete;

   protected:
    const T& ref_;
    std::unique_ptr<std::lock_guard<mutex_type>> lock_;
    xcmixin_init_template(const_mutex_ref);
    xcmixin_friend(const_mutex_ref_impl);
};

template <typename T, typename mutex_ = std::mutex>
class mutex_ref
    : public xcmixin::impl_mixin<mutex_ref<T, mutex_>, mutex_ref_impl> {
   public:
    using mutex_type = mutex_;
    mutex_ref(T& ref, mutex_type& mutex)
        : ref_(ref),
          lock_(std::make_unique<std::lock_guard<mutex_type>>(mutex)) {
        std::cout << "mutex_ref construct " << (void*)&ref << std::endl;
    }
    mutex_ref(T& ref, mutex_type& mutex, std::adopt_lock_t)
        : ref_(ref), lock_(mutex, std::adopt_lock_t{}) {}

   public:
    ~mutex_ref() { std::cout << "mutex_ref destory " << (void*)&ref_ << std::endl; };
    mutex_ref(mutex_ref&&) = default;
    mutex_ref(const mutex_ref&) = delete;
    mutex_ref& operator=(mutex_ref&&) = default;
    mutex_ref& operator=(const mutex_ref&) = delete;

   public:
   protected:
    T& ref_;
    std::unique_ptr<std::lock_guard<mutex_type>> lock_;
    xcmixin_init_template(mutex_ref);
    xcmixin_friend(mutex_ref_impl);
};

template <typename T>
class const_mutex_ref<T, std::shared_mutex>
    : public xcmixin::impl_mixin<const_mutex_ref<T, std::shared_mutex>,
                                 const_mutex_ref_impl> {
   public:
    using mutex_type = std::shared_mutex;
    const_mutex_ref(const T& ref, mutex_type& mutex)
        : ref_(ref), lock_(&mutex) {
        lock_->lock_shared();
    }
    ~const_mutex_ref() {
        if (lock_) lock_->unlock_shared();
    };

   private:
    const T& ref_;
    mutex_type* lock_{nullptr};
    xcmixin_init_template(const_mutex_ref);
    xcmixin_friend(const_mutex_ref_impl);
};

template <typename T, typename Derive>
class lock_impl {
   public:
    template <typename... Args>
        requires(std::is_constructible_v<T, Args...>)
    lock_impl(Args&&... args) : data_(args...) {}
    auto lock() const {
        return const_mutex_ref<T, typename Derive::mutex_type>{
            std::ref(data_),
            std::ref(static_cast<const Derive*>(this)->mutex_)};
    }
    auto lock() {
        return mutex_ref<T, typename Derive::mutex_type>{
            std::ref(data_), std::ref(static_cast<Derive*>(this)->mutex_)};
    }

   protected:
    T data_;
};
template <typename T>
class mutex : public lock_impl<T, mutex<T>> {
    friend class lock_impl<T, mutex<T>>;

   public:
    using mutex_type = std::mutex;
    using lock_impl<T, mutex<T>>::lock_impl;

   private:
    mutable mutex_type mutex_{};
};
template <typename T>
class shared_mutex;
template <typename T>
using shared_mutex_impl = lock_impl<T, shared_mutex<T>>;
template <typename T>
class shared_mutex : public shared_mutex_impl<T> {
    friend shared_mutex_impl<T>;

   public:
    using mutex_type = std::shared_mutex;
    using shared_mutex_impl<T>::shared_mutex_impl;

    auto read() const { return shared_mutex_impl<T>::lock(); }
    auto write() { return shared_mutex_impl<T>::lock(); }

   private:
    mutable std::shared_mutex mutex_{};
};

template <typename T>
struct compose_deref {
    static decltype(auto) deref(T&& v) { return *v; }
};
template <typename T>
struct compose_construct_transform {
    static const T& transform(const T& v) { return v; }
};
template <typename T>
struct compose_construct_transform<mutex<T>> {
    static decltype(auto) transform(mutex<T>& v) {
        std::cout << " mutex<T>& transform()" << std::endl;
        return v.lock();
    }
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
            std::declval<T>()))>...>;

   public:
    compose(T&&... refs)
        : refs_(std::forward_as_tuple(compose_construct_transform<T>::transform(
              std::forward<T>(refs))...)) {
        [&]<size_t... I>(std::index_sequence<I...>) -> decltype(auto) {
            ((std::cout << "init " << &*std::get<I>(refs_) << std::endl), ...);
        }(std::make_index_sequence<sizeof...(T)>());
    }
    ~compose() {
        [&]<size_t... I>(std::index_sequence<I...>) -> decltype(auto) {
            ((std::cout << "destroy " << &*std::get<I>(refs_) << std::endl),
             ...);
        }(std::make_index_sequence<sizeof...(T)>());
    }

    template <typename Fn>
    decltype(auto) operator()(Fn&& fn) {
        return [&]<size_t... I>(std::index_sequence<I...>) -> decltype(auto) {
            ((std::cout << "pre call " << &*std::get<I>(refs_) << std::endl),
             ...);
            fn(*std::get<I>(refs_)...);
        }(std::make_index_sequence<sizeof...(T)>());
    }

    //    private:
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