#pragma once

#include <stdint.h>
#include "IWinMenuContainer.hpp"
#include <tuple>

#pragma pack(push, 1)

template <class... T>
struct WinMenuContainer : public IWinMenuContainer {
    std::tuple<T...> menu_items;

    void Init(const char *label, T... args) {
        menu_items = std::tuple<T...>(args...);
    }

    template <std::size_t I>
    decltype(auto) Item() {
        return std::get<I>(menu_items);
    }
};

#pragma pack(pop)
