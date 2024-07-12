#pragma once

template <typename... T>
struct TypeList {
    static constexpr size_t size = sizeof...(T);
};

/// Subclass of \p Parent - the only thing it does is passing \p args to the parent's constructor
/// Useful in GUI
template <typename Parent, auto... args>
class WithConstructorArgs final : public Parent {
public:
    WithConstructorArgs()
        : Parent(args...) {}
};
