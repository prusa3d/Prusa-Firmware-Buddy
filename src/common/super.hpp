#pragma once
//used in inheritance to be able to access direct base class via super
template <class Base>
struct AddSuper : public Base {
protected:
    typedef Base super;
};
