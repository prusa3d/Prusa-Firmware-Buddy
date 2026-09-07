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

#ifndef BITSERY_TRAITS_STD_STRING_H
#define BITSERY_TRAITS_STD_STRING_H

#include "core/std_defaults.h"
#include <string>

namespace bitsery {

namespace traits {

// specialization for string, because string is already included for
// std::char_traits

template<typename CharT, typename Traits, typename Allocator>
struct ContainerTraits<std::basic_string<CharT, Traits, Allocator>>
  : public StdContainer<std::basic_string<CharT, Traits, Allocator>, true, true>
{};

template<typename CharT, typename Traits, typename Allocator>
struct TextTraits<std::basic_string<CharT, Traits, Allocator>>
{
  using TValue = typename ContainerTraits<
    std::basic_string<CharT, Traits, Allocator>>::TValue;
  // string is automatically null-terminated
  static constexpr bool addNUL = false;

  // is is not 100% accurate, but for performance reasons assume that string
  // stores text, not binary data
  static size_t length(const std::basic_string<CharT, Traits, Allocator>& str)
  {
    return str.size();
  }
};

// specialization for c-array
template<typename T, size_t N>
struct TextTraits<T[N]>
{
  using TValue = T;
  static constexpr bool addNUL = true;

  static size_t length(const T (&container)[N])
  {
    return std::char_traits<T>::length(container);
  }
};

template<typename CharT, typename Traits, typename Allocator>
struct BufferAdapterTraits<std::basic_string<CharT, Traits, Allocator>>
  : public StdContainerForBufferAdapter<
      std::basic_string<CharT, Traits, Allocator>>
{};

}

}

#endif // BITSERY_TRAITS_VECTOR_H
