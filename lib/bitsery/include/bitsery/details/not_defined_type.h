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

#ifndef BITSERY_DETAILS_NOT_DEFINED_TYPE_H
#define BITSERY_DETAILS_NOT_DEFINED_TYPE_H

#include <iterator>

namespace bitsery {
namespace details {
// this type is used to show clearer error messages
struct NotDefinedType
{
  // just swallow anything that is passed during creating
  template<typename... T>
  NotDefinedType(T&&...)
  {
  }
  NotDefinedType() = default;
  // define operators so that we also swallow deeper errors, to reduce error
  // stack this time will be used as iterator, so define all operators necessary
  // to work with iterators
  friend bool operator==(const NotDefinedType&, const NotDefinedType&)
  {
    return true;
  }
  friend bool operator!=(const NotDefinedType&, const NotDefinedType&)
  {
    return false;
  }
  NotDefinedType& operator+=(int) { return *this; }
  NotDefinedType& operator-=(int) { return *this; }

  friend int operator-(const NotDefinedType&, const NotDefinedType&)
  {
    return 0;
  }

  int& operator*() { return data; }
  int data{};
};

template<typename T>
struct IsDefined
  : public std::integral_constant<bool, !std::is_same<NotDefinedType, T>::value>
{};
}
}

namespace std {
// define iterator traits to work with standart algorithms
template<>
struct iterator_traits<bitsery::details::NotDefinedType>
{
  using difference_type = int;
  using value_type = int;
  using pointer = int*;
  using reference = int&;
  using iterator_category = std::random_access_iterator_tag;
};
}
#endif // BITSERY_DETAILS_NOT_DEFINED_TYPE_H
