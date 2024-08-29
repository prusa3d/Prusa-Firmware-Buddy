#pragma once

#include <stdint.h>
#include "i_window_menu_container.hpp"
#include <tuple>

template <class... T>
class WinMenuContainer : public IWinMenuContainer {
public:
    mutable std::tuple<T...> menu_items; // mutable to be able to make const methods for getting index etc

    // compiletime access by index
    template <std::size_t I>
    decltype(auto) Item() {
        return std::get<I>(menu_items);
    }
    // compiletime access by type
    template <class TYPE>
    decltype(auto) Item() {
        return std::get<TYPE>(menu_items);
    }

    virtual int GetRawCount() const override {
        return std::tuple_size<std::tuple<T...>>::value;
    }

    virtual IWindowMenuItem *GetItemByRawIndex(int pos) const override {
        IWindowMenuItem *result = nullptr;
        [&]<size_t... i>(std::index_sequence<i...>) {
            ((pos == i ? (result = &std::get<i>(menu_items)), true : false) || ...);
        }(std::make_index_sequence<sizeof...(T)>());
        return result;
    }

    virtual int GetRawIndex(IWindowMenuItem &item) const override {
        int pos = 0;
        for (; pos < GetRawCount(); ++pos) {
            if (GetItemByRawIndex(pos) == &item) {
                break;
            }
        }
        return pos;
    }
};
