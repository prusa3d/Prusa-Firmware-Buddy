#pragma once

template <typename... T>
struct TypeList {
    static constexpr size_t size = sizeof...(T);
};

/// Subclass of \p Parent - the only thing it does is passing \p args to the parent's constructor
/// If runtime arguments are provided, they are passed first, before the templated ones
/// Useful in GUI
template <typename Parent, auto... args>
class WithConstructorArgs final : public Parent {
public:
    template <typename... Args2>
    WithConstructorArgs(Args2... args2)
        : Parent(args2..., args...) {}
};
