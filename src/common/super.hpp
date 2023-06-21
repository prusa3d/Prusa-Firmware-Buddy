/**
 * @file super.hpp
 * @author Radek Vana
 * @brief add ability to call parrent method via super::
 *
 * example:
 *
 * struct parent {
 * virtual void do_something();
 * };
 *
 * struct child : AddSuper<parent> {
 * virtual void do_something() override {
 *     super::do_something();
 *     // now do something extra
 * }
 * };
 *
 * @date 2021-09-24
 */

#pragma once
// used in inheritance to be able to access direct base class via super

#include <utility>

template <class Base>
struct AddSuper : public Base {
    template <class... Args>
    AddSuper(Args &&...args)
        : Base(std::forward<Args>(args)...) {}

protected:
    typedef Base super;
};
