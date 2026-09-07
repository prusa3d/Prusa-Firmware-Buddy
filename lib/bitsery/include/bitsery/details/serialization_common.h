// MIT License
//
// Copyright (c) 2017 Mindaugas Vinkelis
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

#ifndef BITSERY_DETAILS_SERIALIZATION_COMMON_H
#define BITSERY_DETAILS_SERIALIZATION_COMMON_H

#include "../traits/core/traits.h"
#include "adapter_common.h"
#include <tuple>

namespace bitsery {

// this allows to call private serialize method, and construct instance (if no
// default constructor is provided) for your type just make friend it in your
// class
class Access
{
public:
  template<typename S, typename T>
  static auto serialize(S& s, T& obj) -> decltype(obj.serialize(s))
  {
    obj.serialize(s);
  }

  template<typename T>
  static T create()
  {
    // if you get an error here, please create default constructor
    return T{};
  }
  template<typename T>
  static T* create(void* ptr)
  {
    return new (ptr) T{};
  }
};

// convenient functors that can be passed as lambda to serializer/deserializer
// instead of writing lambda e.g. instead of writing this: s.container(c, 100,
// [](S& s, float& v) { s.ext4b(v, CompactValue{});}); you can write like this
// s.container(c, 100, FtorExtValue2b<CompactValue>{});
template<size_t N, typename Ext>
struct FtorExtValue : public Ext
{
  template<typename S, typename T>
  void operator()(S& s, T& v) const
  {
    s.template ext<N>(v, static_cast<const Ext&>(*this));
  }
};

template<typename Ext>
struct FtorExtValue1b : FtorExtValue<1, Ext>
{};
template<typename Ext>
struct FtorExtValue2b : FtorExtValue<2, Ext>
{};
template<typename Ext>
struct FtorExtValue4b : FtorExtValue<4, Ext>
{};
template<typename Ext>
struct FtorExtValue8b : FtorExtValue<8, Ext>
{};
template<typename Ext>
struct FtorExtValue16b : FtorExtValue<16, Ext>
{};

template<typename Ext>
struct FtorExtObject : public Ext
{
  template<typename S, typename T>
  void operator()(S& s, T& v) const
  {
    s.ext(v, static_cast<const Ext&>(*this));
  }
};

// when call to serialize function is ambiguous (member and non-member serialize
// function exists for a type) specialize this class by inheriting from either
// UseNonMemberFnc or UseMemberFnc e.g. template <> struct
// SelectSerializeFnc<MyDerivedClass>:UseMemberFnc {};
template<typename T>
struct SelectSerializeFnc : std::integral_constant<int, 0>
{};

// types you need to inherit from when specializing SelectSerializeFnc class
struct UseNonMemberFnc : std::integral_constant<int, 1>
{};
struct UseMemberFnc : std::integral_constant<int, 2>
{};

namespace details {

// helper types for error handling
template<typename T>
struct IsContainerTraitsDefined
  : public IsDefined<typename traits::ContainerTraits<T>::TValue>
{};

template<typename T>
struct IsTextTraitsDefined
  : public IsDefined<typename traits::TextTraits<T>::TValue>
{};

template<typename Ext, typename T>
struct IsExtensionTraitsDefined
  : public IsDefined<typename traits::ExtensionTraits<Ext, T>::TValue>
{};

#ifdef _MSC_VER
// helper types for HasSerializeFunction
template<typename S, typename T>
using TrySerializeFunction =
  decltype(serialize(std::declval<S&>(), std::declval<T&>()));

template<typename S, typename T>
struct HasSerializeFunctionHelper
{
  template<typename Q, typename R, typename = TrySerializeFunction<Q, R>>
  static std::true_type tester(Q&&, R&&);
  static std::false_type tester(...);
  using type = decltype(tester(std::declval<S>(), std::declval<T>()));
};
template<typename S, typename T>
struct HasSerializeFunction : HasSerializeFunctionHelper<S, T>::type
{};

// helper types for HasSerializeMethod
template<typename S, typename T>
using TrySerializeMethod =
  decltype(Access::serialize(std::declval<S&>(), std::declval<T&>()));

template<typename S, typename T>
struct HasSerializeMethodHelper
{
  template<typename Q, typename R, typename = TrySerializeMethod<Q, R>>
  static std::true_type tester(Q&&, R&&);
  static std::false_type tester(...);
  using type = decltype(tester(std::declval<S>(), std::declval<T>()));
};
template<typename S, typename T>
struct HasSerializeMethod : HasSerializeMethodHelper<S, T>::type
{};

// helper types for IsBriefSyntaxIncluded
template<typename S, typename T>
using TryProcessBriefSyntax =
  decltype(processBriefSyntax(std::declval<S&>(), std::declval<T&&>()));

template<typename S, typename T>
struct IsBriefSyntaxIncludedHelper
{
  template<typename Q, typename R, typename = TryProcessBriefSyntax<Q, R>>
  static std::true_type tester(Q&&, R&&);
  static std::false_type tester(...);
  using type = decltype(tester(std::declval<S>(), std::declval<T>()));
};

template<typename S, typename T>
struct IsBriefSyntaxIncluded : IsBriefSyntaxIncludedHelper<S, T>::type
{};
#else
// helper metafunction, that is added to c++17
template<typename... Ts>
struct make_void
{
  typedef void type;
};
template<typename... Ts>
using void_t = typename make_void<Ts...>::type;

template<typename, typename, typename = void>
struct HasSerializeFunction : std::false_type
{};

template<typename S, typename T>
struct HasSerializeFunction<
  S,
  T,
  void_t<decltype(serialize(std::declval<S&>(), std::declval<T&>()))>>
  : std::true_type
{};

template<typename, typename, typename = void>
struct HasSerializeMethod : std::false_type
{};

template<typename S, typename T>
struct HasSerializeMethod<
  S,
  T,
  void_t<decltype(Access::serialize(std::declval<S&>(), std::declval<T&>()))>>
  : std::true_type
{};

// this solution doesn't work with visual studio, but is more elegant
template<typename, typename, typename = void>
struct IsBriefSyntaxIncluded : std::false_type
{};

template<typename S, typename T>
struct IsBriefSyntaxIncluded<
  S,
  T,
  void_t<decltype(processBriefSyntax(std::declval<S&>(), std::declval<T&&>()))>>
  : std::true_type
{};
#endif

// used for extensions when extension TValue = void
struct DummyType
{};

/*
 * this includes all integral types, floats and enums(except bool)
 */
template<typename T>
struct IsFundamentalType
  : std::integral_constant<bool,
                           std::is_enum<T>::value ||
                             std::is_floating_point<T>::value ||
                             std::is_integral<T>::value>
{};

template<typename T, typename Integral = void>
struct IntegralFromFundamental
{
  using TValue = T;
};

template<typename T>
struct IntegralFromFundamental<
  T,
  typename std::enable_if<std::is_enum<T>::value>::type>
{
  using TValue = typename std::underlying_type<T>::type;
};

template<typename T>
struct IntegralFromFundamental<
  T,
  typename std::enable_if<std::is_floating_point<T>::value>::type>
{
  using TValue = typename std::
    conditional<std::is_same<T, float>::value, uint32_t, uint64_t>::type;
};

template<typename T>
struct UnsignedFromFundamental
{
  using type = typename std::make_unsigned<
    typename IntegralFromFundamental<T>::TValue>::type;
};

template<typename T>
using SameSizeUnsigned = typename UnsignedFromFundamental<T>::type;

/*
 * functions for object serialization
 */

template<typename S, typename T>
struct SerializeFunction
{

  static void invoke(S& s, T& v)
  {
    static_assert(HasSerializeFunction<S, T>::value ||
                    HasSerializeMethod<S, T>::value,
                  "\nPlease define 'serialize' function for your type (inside "
                  "or outside of class):\n"
                  "  template<typename S>\n"
                  "  void serialize(S& s)\n"
                  "  {\n"
                  "    ...\n"
                  "  }\n");
    using TDecayed = typename std::decay<T>::type;
    selectSerializeFnc(s, v, SelectSerializeFnc<TDecayed>{});
  }

  static constexpr bool isDefined()
  {
    return HasSerializeFunction<S, T>::value || HasSerializeMethod<S, T>::value;
  }

private:
  static void selectSerializeFnc(S& s, T& v, std::integral_constant<int, 0>)
  {
    static_assert(
      !(HasSerializeFunction<S, T>::value && HasSerializeMethod<S, T>::value),
      "\nPlease define only one 'serialize' function (member OR free).\n"
      "If serialization function is inherited from base class, then explicitly "
      "select correct function for your type e.g.:\n"
      "  template <>\n"
      "  struct SelectSerializeFnc<DerivedClass>:UseMemberFnc {};\n");
    selectSerializeFnc(s,
                       v,
                       std::integral_constant < int,
                       HasSerializeFunction<S, T>::value ? 1 : 2 > {});
  }

  static void selectSerializeFnc(S& s, T& v, std::integral_constant<int, 1>)
  {
    serialize(s, v);
  }

  static void selectSerializeFnc(S& s, T& v, std::integral_constant<int, 2>)
  {
    Access::serialize(s, v);
  }
};

/*
 * functions for object serialization
 */

template<typename S, typename T, typename Enabled = void>
struct BriefSyntaxFunction
{

  static void invoke(S& s, T&& obj)
  {
    static_assert(
      IsBriefSyntaxIncluded<S, T>::value,
      "\nPlease include '<bitsery/brief_syntax.h>' to use operator():\n");

    processBriefSyntax(s, std::forward<T>(obj));
  }
};

/*
 * helper function for getting context from serializer/deserializer
 */

template<int Index, typename... Conds>
struct FindIndex : std::integral_constant<int, Index>
{};

template<int Index, typename Cond, typename... Conds>
struct FindIndex<Index, Cond, Conds...>
  : std::conditional<Cond::value,
                     std::integral_constant<int, Index>,
                     FindIndex<Index + 1, Conds...>>::type
{};

template<typename T, typename Tuple>
struct GetConvertibleTypeIndexFromTuple;

template<typename T, typename... Us>
struct GetConvertibleTypeIndexFromTuple<T, std::tuple<Us...>>
  : FindIndex<0, std::is_convertible<Us&, T&>...>
{};

template<typename T, typename Tuple>
struct IsExistsConvertibleTupleType;

template<typename T, typename... Us>
struct IsExistsConvertibleTupleType<T, std::tuple<Us...>>
  : std::integral_constant<
      bool,
      GetConvertibleTypeIndexFromTuple<T, std::tuple<Us...>>::value !=
        sizeof...(Us)>
{};

/*
 * get context from internal or external, and check if it's convertible or not
 */

template<bool AssertExists, typename TCast, typename TContext>
TCast*
getDirectlyIfExists(TContext& ctx, std::true_type)
{
  return &static_cast<TCast&>(ctx);
}

template<bool AssertExists, typename TCast, typename TContext>
TCast*
getDirectlyIfExists(TContext&, std::false_type)
{
  // TCast cannot be convertible from provided context
  static_assert(
    !AssertExists,
    "Invalid context cast. Context type doesn't exists.\nSome functionality "
    "requires (de)seserializer to have specific context.");
  return nullptr;
}

template<bool AssertExists, typename TCast, typename... TArgs>
TCast*
getFromTupleIfExists(std::tuple<TArgs...>& ctx, std::true_type)
{
  using TupleIndex =
    GetConvertibleTypeIndexFromTuple<TCast, std::tuple<TArgs...>>;
  return &static_cast<TCast&>(std::get<TupleIndex::value>(ctx));
}

template<bool AssertExists, typename TCast, typename... TArgs>
TCast*
getFromTupleIfExists(std::tuple<TArgs...>&, std::false_type)
{
  // TCast cannot be convertible from provided context
  static_assert(
    !AssertExists,
    "Invalid context cast. Context type doesn't exists.\nSome functionality "
    "requires (de)seserializer to have specific context.");
  return nullptr;
}

// non tuple context
template<bool AssertExists, typename TCast, typename TContext>
TCast*
getContext(TContext& ctx)
{
  return getDirectlyIfExists<AssertExists, TCast>(
    ctx, std::is_convertible<TContext&, TCast&>{});
}

// tuple context
template<bool AssertExists, typename TCast, typename... TArgs>
TCast*
getContext(std::tuple<TArgs...>& ctx)
{
  return getFromTupleIfExists<AssertExists, TCast>(
    ctx, IsExistsConvertibleTupleType<TCast, std::tuple<TArgs...>>{});
}

template<typename Adapter, typename Context>
class AdapterAndContextRef
{
public:
  static constexpr bool HasContext = true;
  using Config = typename Adapter::TConfig;

  // constructing adapter in place is important,
  // because enableBitPacking might create instance with bit write/read enabled
  // adapter wrapper, which has non trivial destructor
  template<typename... TArgs>
  explicit AdapterAndContextRef(Context& ctx, TArgs&&... args)
    : _adapter{ std::forward<TArgs>(args)... }
    , _context{ ctx }
  {
  }

  /*
   * get serialization context.
   * this is optional, but might be required for some specific serialization
   * flows.
   */

  template<typename T>
  T& context()
  {
    return *getContext<true, T>(_context);
  }

  template<typename T>
  T* contextOrNull()
  {
    return getContext<false, T>(_context);
  }

  Adapter& adapter() & { return _adapter; }

  Adapter adapter() && { return std::move(_adapter); }

protected:
  Adapter _adapter;
  Context& _context;
};

template<typename Adapter>
class AdapterAndContextRef<Adapter, void>
{
public:
  static constexpr bool HasContext = false;
  using Config = typename Adapter::TConfig;

  template<typename... TArgs>
  explicit AdapterAndContextRef(TArgs&&... args)
    : _adapter{ std::forward<TArgs>(args)... }
  {
  }

  template<typename T>
  T& context()
  {
    static_assert(std::is_void<T>::value, "Context is not defined (is void).");
  }

  template<typename T>
  T* contextOrNull()
  {
    return nullptr;
  }

  Adapter& adapter() & { return _adapter; }

  Adapter adapter() && { return std::move(_adapter); }

protected:
  Adapter _adapter;
};

/**
 * other helper meta-functions
 */

template<typename T, template<typename...> class Template>
struct IsSpecializationOf : std::false_type
{};

template<template<typename...> class Template, typename... Args>
struct IsSpecializationOf<Template<Args...>, Template> : std::true_type
{};

}
}

#endif // BITSERY_DETAILS_SERIALIZATION_COMMON_H
