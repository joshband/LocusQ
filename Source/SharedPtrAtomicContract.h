#pragma once

#include <atomic>
#include <memory>

// Shared_ptr publication wrapper with explicit acquire/release semantics.
// Uses std::atomic<std::shared_ptr<T>> when supported by the toolchain; falls
// back to localized atomic free-function calls otherwise so call sites remain
// contract-based and do not use deprecated APIs directly.
template <typename T>
class SharedPtrAtomicContract
{
public:
    SharedPtrAtomicContract() noexcept = default;
    explicit SharedPtrAtomicContract (std::shared_ptr<T> initialValue) noexcept
        : value (std::move (initialValue))
    {
    }

    void store (std::shared_ptr<T> nextValue) noexcept
    {
       #if defined(__cpp_lib_atomic_shared_ptr) && (__cpp_lib_atomic_shared_ptr >= 201711L)
        value.store (std::move (nextValue), std::memory_order_release);
       #else
       #if defined(__clang__)
       #pragma clang diagnostic push
       #pragma clang diagnostic ignored "-Wdeprecated-declarations"
       #endif
        std::atomic_store_explicit (&value, std::move (nextValue), std::memory_order_release);
       #if defined(__clang__)
       #pragma clang diagnostic pop
       #endif
       #endif
    }

    std::shared_ptr<T> load() const noexcept
    {
       #if defined(__cpp_lib_atomic_shared_ptr) && (__cpp_lib_atomic_shared_ptr >= 201711L)
        return value.load (std::memory_order_acquire);
       #else
       #if defined(__clang__)
       #pragma clang diagnostic push
       #pragma clang diagnostic ignored "-Wdeprecated-declarations"
       #endif
        return std::atomic_load_explicit (&value, std::memory_order_acquire);
       #if defined(__clang__)
       #pragma clang diagnostic pop
       #endif
       #endif
    }

private:
   #if defined(__cpp_lib_atomic_shared_ptr) && (__cpp_lib_atomic_shared_ptr >= 201711L)
    std::atomic<std::shared_ptr<T>> value;
   #else
    std::shared_ptr<T> value;
   #endif
};
