#pragma once
//used in inheritance to be able to access direct base class via super
template <class Base>
struct AddSuper : public Base {
    template <class... T>
    AddSuper(T... args)
        : Base(args...) {}

protected:
    typedef Base super;
};
