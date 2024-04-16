#pragma once

#include <concepts>

namespace concepts {

template <typename L>
concept BasicLockable = requires(L m) {
    { m.lock() };
    { m.unlock() };
};

template <typename L>
concept Lockable = BasicLockable<L> && requires(L m) {
    { m.try_lock() } -> std::same_as<bool>;
};

template <typename L>
concept SharedLockable = requires(L m) {
    { m.lock_shared() };
    { m.try_lock_shared() } -> std::same_as<bool>;
    { m.unlock_shared() };
};

} // namespace concepts
