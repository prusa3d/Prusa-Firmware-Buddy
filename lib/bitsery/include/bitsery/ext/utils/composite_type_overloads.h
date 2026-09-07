// MIT License
//
// Copyright (c) 2019 Mindaugas Vinkelis
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef BITSERY_EXT_COMPOSITE_TYPE_OVERLOADS_H
#define BITSERY_EXT_COMPOSITE_TYPE_OVERLOADS_H

#include "../../details/serialization_common.h"
#include <functional>

#if __cplusplus < 201703L
#error these utils requires c++17
// in theory, it could be implemented using C++11
// but without class template argument deduction guides that would be very
// inconvenient to use these are very helpul for sum types (e.g. std::variant),
// but for product types (e.g. std::tuple) you can you can easily do it your
// self with lambda, without extension
#endif

namespace bitsery {

namespace ext {
// might be usable, when you want to have one overload set for different
// composite types, e.g. variant, tuple and pair
template<class... Ts>
struct CompositeTypeOverloads : Ts...
{
  using Ts::operator()...;
};

template<typename... Overloads>
CompositeTypeOverloads(Overloads...) -> CompositeTypeOverloads<Overloads...>;

// convenient way to invoke s.value<N>, shorter than specifying a lambda
template<typename T, size_t N>
struct OverloadValue
{
  template<typename S>
  void operator()(S& s, T& v) const
  {
    s.template value<N>(v);
  }
};

// convenient way to invoke other extension using value or object overloads
// there is no reason to write OverloadExtLambda,
// because you'll need to specify lambda type, which is very inconvenient and it
// will be much easier to simple write a lambda with extension inside it, in
// order to implement it in a convenient way, i need a way to deduce only last
// template parameter (lambda type) but this is not possible with deduction
// guides at the moment

template<typename T, size_t N, typename Ext>
struct OverloadExtValue : public Ext
{
  template<typename S>
  void operator()(S& s, T& v) const
  {
    s.template ext<N>(v, static_cast<const Ext&>(*this));
  }
};

template<typename T, typename Ext>
struct OverloadExtObject : public Ext
{
  template<typename S>
  void operator()(S& s, T& v) const
  {
    s.ext(v, static_cast<const Ext&>(*this));
  }
};
}

namespace details {

template<template<typename...> typename CompositeType, typename... Overloads>
class CompositeTypeOverloadsUtils
  : public ext::CompositeTypeOverloads<Overloads...>
{
protected:
  // converts run-time index to compile-time index,
  // by calling lambda with std::integral_constant<size_t, INDEX>
  template<typename Fnc, typename... Ts>
  void execIndex(size_t index, CompositeType<Ts...>& obj, Fnc&& fnc) const
  {
    execIndexImpl(
      index, obj, std::forward<Fnc>(fnc), std::index_sequence_for<Ts...>{});
  }

  // call lambda for all indexes in composite type
  template<typename Fnc, typename... Ts>
  void execAll(CompositeType<Ts...>& obj, Fnc&& fnc) const
  {
    execAllImpl(obj, std::forward<Fnc>(fnc), std::index_sequence_for<Ts...>{});
  }

  // serialize a type, by using overload first
  template<typename S, typename T>
  void serializeType(S& s, T& v) const
  {
    // first check if overload exists, otherwise try to call serialize method
    if constexpr (hasOverload<S, T>()) {
      std::invoke(*this, s, v);
    } else {
      static_assert(
        details::SerializeFunction<S, T>::isDefined(),
        "Please define overload or 'serialize' function for your type.");
      s.object(v);
    }
  }

private:
  template<typename S, typename T>
  static constexpr bool hasOverload()
  {
    return std::is_invocable<ext::CompositeTypeOverloads<Overloads...>,
                             std::add_lvalue_reference_t<S>,
                             std::add_lvalue_reference_t<T>>::value;
  }

  template<typename Variant, typename Fnc, size_t... Is>
  void execIndexImpl(size_t index,
                     Variant& obj,
                     Fnc&& fnc,
                     std::index_sequence<Is...>) const
  {
    ((index == Is ? fnc(obj, std::integral_constant<size_t, Is>{}), 0 : 0),
     ...);
  }

  template<typename Variant, typename Fnc, size_t... Is>
  void execAllImpl(Variant& obj, Fnc&& fnc, std::index_sequence<Is...>) const
  {
    (fnc(obj, std::integral_constant<size_t, Is>{}), ...);
  }
};

}
}

#endif // BITSERY_EXT_COMPOSITE_TYPE_OVERLOADS_H
