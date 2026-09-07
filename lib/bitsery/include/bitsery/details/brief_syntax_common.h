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

#ifndef BITSERY_DETAILS_BRIEF_SYNTAX_COMMON_H
#define BITSERY_DETAILS_BRIEF_SYNTAX_COMMON_H

#include "../traits/core/traits.h"
#include "serialization_common.h"
#include <limits>

namespace bitsery {
namespace brief_syntax {

// these function overloads is required to apply maxSize, and optimize for
// fundamental types for contigous arrays of fundamenal types, memcpy will be
// applied

template<typename S,
         typename T,
         typename std::enable_if<
           details::IsFundamentalType<
             typename traits::ContainerTraits<T>::TValue>::value &&
           traits::ContainerTraits<T>::isResizable>::type* = nullptr>
void
processContainer(S& s,
                 T& c,
                 size_t maxSize = std::numeric_limits<size_t>::max())
{
  using TValue = typename traits::ContainerTraits<T>::TValue;
  s.template container<sizeof(TValue)>(c, maxSize);
}

template<typename S,
         typename T,
         typename std::enable_if<
           !details::IsFundamentalType<
             typename traits::ContainerTraits<T>::TValue>::value &&
           traits::ContainerTraits<T>::isResizable>::type* = nullptr>
void
processContainer(S& s,
                 T& c,
                 size_t maxSize = std::numeric_limits<size_t>::max())
{
  s.container(c, maxSize);
}

template<typename S,
         typename T,
         typename std::enable_if<
           details::IsFundamentalType<
             typename traits::ContainerTraits<T>::TValue>::value &&
           !traits::ContainerTraits<T>::isResizable>::type* = nullptr>
void
processContainer(S& s, T& c)
{
  using TValue = typename traits::ContainerTraits<T>::TValue;
  s.template container<sizeof(TValue)>(c);
}

template<typename S,
         typename T,
         typename std::enable_if<
           !details::IsFundamentalType<
             typename traits::ContainerTraits<T>::TValue>::value &&
           !traits::ContainerTraits<T>::isResizable>::type* = nullptr>
void
processContainer(S& s, T& c)
{
  s.container(c);
}

// overloads for text processing to apply maxSize

template<typename S,
         typename T,
         typename std::enable_if<
           traits::ContainerTraits<T>::isResizable>::type* = nullptr>
void
processText(S& s, T& c, size_t maxSize = std::numeric_limits<size_t>::max())
{
  using TValue = typename traits::ContainerTraits<T>::TValue;
  s.template text<sizeof(TValue)>(c, maxSize);
}

template<typename S,
         typename T,
         typename std::enable_if<
           !traits::ContainerTraits<T>::isResizable>::type* = nullptr>
void
processText(S& s, T& c)
{
  using TValue = typename traits::ContainerTraits<T>::TValue;
  s.template text<sizeof(TValue)>(c);
}

// all wrapper functions, that modify behaviour, should inherit from this
struct ModFnc
{};

// this type is used to differentiate between container and text behaviour
template<typename T, size_t N, bool isText>
struct CArray : public ModFnc
{
  CArray(T (&data_)[N])
    : data{ data_ } {};
  T (&data)[N];
};

template<typename S, typename T, size_t N>
void
serialize(S& s, CArray<T, N, true>& str)
{
  processText(s, str.data);
}

template<typename S, typename T, size_t N>
void
serialize(S& s, CArray<T, N, false>& obj)
{
  processContainer(s, obj.data);
}

// used to set max container size
template<typename T>
struct MaxSize : public ModFnc
{
  MaxSize(T& data_, size_t maxSize_)
    : data{ data_ }
    , maxSize{ maxSize_ } {};
  T& data;
  size_t maxSize;
};

// if container, then call procesContainer, this memcpy for fundamental types
// contiguous container
template<typename S, typename T>
void
processMaxSize(S& s, T& data, size_t maxSize, std::true_type)
{
  processContainer(s, data, maxSize);
}

// overload for const T&
template<typename S, typename T>
void
processMaxSize(S& s, const T& data, size_t maxSize, std::true_type)
{
  processContainer(s, const_cast<T&>(data), maxSize);
}

// try to call serialize overload with maxsize, extensions use this technique
template<typename S, typename T>
void
processMaxSize(S& s, T& data, size_t maxSize, std::false_type)
{
  serialize(s, data, maxSize);
}

// overload for const T&
template<typename S, typename T>
void
processMaxSize(S& s, const T& data, size_t maxSize, std::false_type)
{
  serialize(s, const_cast<T&>(data), maxSize);
}

template<typename S, typename T>
void
serialize(S& s, const MaxSize<T>& ms)
{
  processMaxSize(
    s,
    ms.data,
    ms.maxSize,
    details::IsContainerTraitsDefined<typename std::decay<T>::type>{});
}

}
}

#endif // BITSERY_DETAILS_BRIEF_SYNTAX_COMMON_H
