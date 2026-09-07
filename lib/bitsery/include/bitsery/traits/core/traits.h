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

#ifndef BITSERY_TRAITS_CORE_TRAITS_H
#define BITSERY_TRAITS_CORE_TRAITS_H

#include "../../details/not_defined_type.h"
#include <type_traits>

namespace bitsery {
namespace traits {

/*
 * core library traits, used to extend library for custom types
 */

// traits for extension
template<typename Extension, typename T>
struct ExtensionTraits
{
  // this type is used, when using extesion without custom lambda
  //  eg.: extension4b>(obj, myextension{}) will call s.value4b(obj) for TValue
  //  or extesion(obj, myextension{})  will call s.object(obj) for TValue
  // when this is void, it will compile, but value and object overloads will do
  // nothing.
  using TValue = details::NotDefinedType;

  // does extension support ext<N>(...) syntax, by calling value<N> with TValue
  static constexpr bool SupportValueOverload = false;
  // does extension support ext(...) syntax, by calling object with TValue
  static constexpr bool SupportObjectOverload = false;
  // does extension support ext(..., lambda)
  static constexpr bool SupportLambdaOverload = false;
};

// primary traits for containers
template<typename T>
struct ContainerTraits
{

  using TValue = details::NotDefinedType;

  static constexpr bool isResizable = false;
  // contiguous arrays has oppurtunity to memcpy whole buffer directly when
  // using funtamental types contiguous doesn't nesessary equal to random access
  // iterator. contiguous hopefully will be available in c++20
  static constexpr bool isContiguous = false;
  // resize function, called only if container is resizable
  static void resize(T&, size_t)
  {
    static_assert(std::is_void<T>::value,
                  "Define ContainerTraits or include from <bitsery/traits/...> "
                  "to use as container");
  }
  // get container size
  static size_t size(const T&)
  {
    static_assert(std::is_void<T>::value,
                  "Define ContainerTraits or include from <bitsery/traits/...> "
                  "to use as container");
    return 0u;
  }
};

// specialization for C style array
template<typename T, size_t N>
struct ContainerTraits<T[N]>
{
  using TValue = T;
  static constexpr bool isResizable = false;
  static constexpr bool isContiguous = true;
  static size_t size(const T (&)[N]) { return N; }
};

// specialization for initializer list.
// only serializer can use it
template<typename T>
struct ContainerTraits<std::initializer_list<T>>
{
  using TValue = T;
  static constexpr bool isResizable = false;
  static constexpr bool isContiguous = true;
  static size_t size(const std::initializer_list<T>& container)
  {
    return container.size();
  }
};

// specialization for pointer type buffer
// only deserializer can use it
template<typename T>
struct ContainerTraits<const T*>
{
  using TValue = T;
  static constexpr bool isResizable = false;
  static constexpr bool isContiguous = true;
  static size_t size(const T*)
  {
    static_assert(std::is_void<T>::value,
                  "cannot get size for container of type T*");
    return 0u;
  }
};

template<typename T>
struct ContainerTraits<T*>
{
  using TValue = T;
  static constexpr bool isResizable = false;
  static constexpr bool isContiguous = true;
  static size_t size(const T*)
  {
    static_assert(std::is_void<T>::value,
                  "cannot get size for container of type T*");
    return 0u;
  }
};

// traits for text, default adds null-terminated character at the end
template<typename T>
struct TextTraits
{
  using TValue = details::NotDefinedType;
  // if container is not null-terminated by default, add NUL at the end
  static constexpr bool addNUL = true;

  // get length of null terminated container
  static size_t length(const T&)
  {
    static_assert(
      std::is_void<T>::value,
      "Define TextTraits or include from <bitsery/traits/...> to use as text");
    return 0u;
  }
};

// traits only for buffer adapters
template<typename T>
struct BufferAdapterTraits
{
  using TIterator = details::NotDefinedType;
  using TConstIterator = details::NotDefinedType;
  using TValue = typename ContainerTraits<T>::TValue;

  // this function is only applies to resizable containers

  // this function is only used by Writer, when writing data to buffer,
  // it is called only current buffer size is not enough to write.
  // it is used to dramaticaly improve performance by updating buffer directly
  // instead of using back_insert_iterator to append each byte to buffer.

  static void increaseBufferSize(T&, size_t, size_t)
  {
    static_assert(std::is_void<T>::value,
                  "Define BufferAdapterTraits or include from "
                  "<bitsery/traits/...> to use as buffer adapter container");
  }
};

// specialization for c-style buffer
template<typename T, size_t N>
struct BufferAdapterTraits<T[N]>
{
  using TIterator = T*;
  using TConstIterator = const T*;
  using TValue = T;
};

// specialization for pointer type buffer
template<typename T>
struct BufferAdapterTraits<const T*>
{
  using TIterator = const T*;
  using TConstIterator = const T*;
  using TValue = T;
};

template<typename T>
struct BufferAdapterTraits<T*>
{
  using TIterator = T*;
  using TConstIterator = const T*;
  using TValue = T;
};

}
}

#endif // BITSERY_TRAITS_CORE_TRAITS_H
