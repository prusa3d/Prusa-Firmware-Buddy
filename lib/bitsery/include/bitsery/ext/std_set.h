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

#ifndef BITSERY_EXT_STD_SET_H
#define BITSERY_EXT_STD_SET_H

#include "../details/serialization_common.h"
#include <unordered_set>

namespace bitsery {
namespace ext {

class StdSet
{
public:
  constexpr explicit StdSet(size_t maxSize)
    : _maxSize{ maxSize }
  {
  }

  template<typename Ser, typename T, typename Fnc>
  void serialize(Ser& ser, const T& obj, Fnc&& fnc) const
  {
    using TKey = typename T::key_type;
    auto size = obj.size();
    assert(size <= _maxSize);
    details::writeSize(ser.adapter(), size);

    for (auto& v : obj)
      fnc(ser, const_cast<TKey&>(v));
  }

  template<typename Des, typename T, typename Fnc>
  void deserialize(Des& des, T& obj, Fnc&& fnc) const
  {
    using TKey = typename T::key_type;

    size_t size{};
    details::readSize(
      des.adapter(),
      size,
      _maxSize,
      std::integral_constant<bool, Des::TConfig::CheckDataErrors>{});
    obj.clear();
    reserve(obj, size);
    auto hint = obj.begin();
    for (auto i = 0u; i < size; ++i) {
      auto key = bitsery::Access::create<TKey>();
      fnc(des, key);
      hint = obj.emplace_hint(hint, std::move(key));
    }
  }

private:
  template<typename Key, typename Hash, typename KeyEqual, typename Allocator>
  void reserve(std::unordered_set<Key, Hash, KeyEqual, Allocator>& obj,
               size_t size) const
  {
    obj.reserve(size);
  }
  template<typename Key, typename Hash, typename KeyEqual, typename Allocator>
  void reserve(std::unordered_multiset<Key, Hash, KeyEqual, Allocator>& obj,
               size_t size) const
  {
    obj.reserve(size);
  }

  template<typename T>
  void reserve(T&, size_t) const
  {
    // for ordered container do nothing
  }
  size_t _maxSize;
};
}

namespace traits {
template<typename T>
struct ExtensionTraits<ext::StdSet, T>
{
  using TValue = typename T::key_type;
  static constexpr bool SupportValueOverload = true;
  static constexpr bool SupportObjectOverload = true;
  static constexpr bool SupportLambdaOverload = true;
};
}

}

#endif // BITSERY_EXT_STD_SET_H
