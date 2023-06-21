#pragma once

template <class T>
struct extract_class_type;
template <class C, class T>
struct extract_class_type<T C::*> {
    using type = C;
};
template <typename T>
using extract_class_type_t = typename extract_class_type<T>::type;

template <class T>
struct remove_member_pointer;
template <class C, class T>
struct remove_member_pointer<T C::*> {
    using type = T;
};
template <typename T>
using remove_member_pointer_t = typename remove_member_pointer<T>::type;
