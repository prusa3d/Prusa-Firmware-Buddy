#pragma once
//used in inheritance to be able to access direct base class via super
template <class Base>
struct AddSuper : public Base {
    template <class... Args>
    AddSuper(Args &&... args)
        : Base(std::forward<Args>(args)...) {}

protected:
    typedef Base super;
};
