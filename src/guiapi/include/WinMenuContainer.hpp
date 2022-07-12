#pragma once

#include <stdint.h>
#include "IWinMenuContainer.hpp"
#include <tuple>

//helper functions to get Nth element at runtime
//todo make it member
template <std::size_t I = 0, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), IWindowMenuItem *>::type
get_ptr_for_index(int, std::tuple<Tp...> &) { return NULL; }

template <std::size_t I = 0, typename... Tp>
    inline typename std::enable_if < I<sizeof...(Tp), IWindowMenuItem *>::type
    get_ptr_for_index(int index, std::tuple<Tp...> &t) {
    if (index == 0)
        return &(std::get<I>(t));
    return get_ptr_for_index<I + 1, Tp...>(index - 1, t);
}

template <class... T>
class WinMenuContainer : public IWinMenuContainer {
public:
    std::tuple<T...> menu_items;

    void Init(const char *label, T... args) {
        menu_items = std::tuple<T...>(args...);
    }

    //compiletime access by index
    template <std::size_t I>
    decltype(auto) Item() {
        return std::get<I>(menu_items);
    }
    //compiletime access by type
    template <class TYPE>
    decltype(auto) Item() {
        return std::get<TYPE>(menu_items);
    }

    virtual size_t GetCount() override {
        return std::tuple_size<std::tuple<T...>>::value;
    }

    virtual IWindowMenuItem *GetItem(size_t pos) override {
        if (pos > GetCount())
            return NULL;
        else
            return get_ptr_for_index((int)pos, menu_items);
    }

    virtual size_t GetIndex(IWindowMenuItem &item) override {
        size_t pos = 0;
        for (; pos < GetCount(); ++pos) {
            if (GetItem(pos) == &item)
                break;
        }
        return pos;
    }
};
