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

#ifndef BITSERY_BRIEF_SYNTAX_H
#define BITSERY_BRIEF_SYNTAX_H

#include "details/brief_syntax_common.h"
#include "details/serialization_common.h"

namespace bitsery {

// define function that enables s(....) usage
template<typename S, typename T>
void
processBriefSyntax(S& s, T&& head)
{
  static_assert(
    std::is_lvalue_reference<T>::value ||
      std::is_base_of<brief_syntax::ModFnc, T>::value,
    "Argument must be either lvalue or subclass of brief_syntax::ModFnc");
  s.object(head);
}

// wrapper functions that enables to serialize as container or string
template<typename T, size_t N>
brief_syntax::CArray<T, N, true>
asText(T (&str)[N])
{
  return { str };
}

template<typename T, size_t N>
brief_syntax::CArray<T, N, false>
asContainer(T (&obj)[N])
{
  return { obj };
}

template<typename T>
brief_syntax::MaxSize<T>
maxSize(T& obj, size_t max)
{
  return { obj, max };
}

// define serialize function for fundamental types
template<typename S>
void
serialize(S& s, bool& v)
{
  s.boolValue(v);
}

template<typename S,
         typename T,
         typename std::enable_if<details::IsFundamentalType<T>::value>::type* =
           nullptr>
void
serialize(S& s, T& v)
{
  s.template value<sizeof(T)>(v);
}

// define serialization for c-style container

// if array is integral type, specify explicitly how to process: as text or
// container
template<typename S,
         typename T,
         size_t N,
         typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
void
serialize(S&, T (&)[N])
{
  static_assert(N == 0,
                "\nPlease use 'asText(obj)' or 'asContainer(obj)' when using "
                "c-style array with integral types\n");
}

template<typename S,
         typename T,
         size_t N,
         typename std::enable_if<!std::is_integral<T>::value>::type* = nullptr>
void
serialize(S& s, T (&obj)[N])
{
  brief_syntax::processContainer(s, obj);
}

// this is a helper class that enforce fundamental type sizes, when used on
// multiple platforms
template<size_t TShort, size_t TInt, size_t TLong, size_t TLongLong>
void
assertFundamentalTypeSizes()
{
  // http://en.cppreference.com/w/cpp/language/types
  static_assert(sizeof(short) == TShort, "");
  static_assert(sizeof(int) == TInt, "");
  static_assert(sizeof(long) == TLong, "");
  static_assert(sizeof(long long) == TLongLong, "");
  // for completion we also need pointer type size, but serializer doesn't
  // support pointer serialization.
}

}

#endif // BITSERY_BRIEF_SYNTAX_H
