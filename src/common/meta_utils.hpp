#pragma once

template <typename... T>
struct TypeList {
    static constexpr size_t size = sizeof...(T);
};
