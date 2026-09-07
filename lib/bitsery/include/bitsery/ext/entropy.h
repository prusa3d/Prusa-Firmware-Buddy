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

#ifndef BITSERY_EXT_ENTROPY_H
#define BITSERY_EXT_ENTROPY_H

#include "value_range.h"

namespace bitsery {

namespace details {
template<typename TValue, typename TContainer>
size_t
findEntropyIndex(const TValue& v, const TContainer& defValues)
{
  size_t index{ 1u };
  for (auto& d : defValues) {
    if (d == v)
      return index;
    ++index;
  }
  return 0u;
}
}

namespace ext {

template<typename TContainer>
class Entropy
{
public:
  /**
   * Allows entropy-encoding technique, by writing few bits for most common
   * values
   * @param values list of most common values
   * @param alignBeforeData only makes sense when bit-packing enabled, by
   * default aligns after writing bits for index
   */
  constexpr Entropy(TContainer& values, bool alignBeforeData = true)
    : _values{ values }
    , _alignBeforeData{ alignBeforeData } {};

  template<typename Ser, typename T, typename Fnc>
  void serialize(Ser& s, const T& obj, Fnc&& fnc) const
  {
    assert(traits::ContainerTraits<TContainer>::size(_values) > 0);
    auto index = details::findEntropyIndex(obj, _values);
    s.ext(index,
          ext::ValueRange<size_t>{
            0u, traits::ContainerTraits<TContainer>::size(_values) });
    if (_alignBeforeData)
      s.adapter().align();
    if (!index)
      fnc(s, const_cast<T&>(obj));
  }

  template<typename Des, typename T, typename Fnc>
  void deserialize(Des& d, T& obj, Fnc&& fnc) const
  {
    assert(traits::ContainerTraits<TContainer>::size(_values) > 0);
    size_t index{};
    d.ext(index,
          ext::ValueRange<size_t>{
            0u, traits::ContainerTraits<TContainer>::size(_values) });
    if (_alignBeforeData)
      d.adapter().align();
    if (index) {
      using TDiff = typename std::iterator_traits<decltype(std::begin(
        _values))>::difference_type;
      obj = static_cast<T>(
        *std::next(std::begin(_values), static_cast<TDiff>(index - 1)));
    } else
      fnc(d, obj);
  }

private:
  TContainer& _values;
  bool _alignBeforeData;
};
}

namespace traits {
template<typename TContainer, typename T>
struct ExtensionTraits<ext::Entropy<TContainer>, T>
{
  using TValue = T;
  static constexpr bool SupportValueOverload = true;
  static constexpr bool SupportObjectOverload = true;
  static constexpr bool SupportLambdaOverload = true;
};
}

}

#endif // BITSERY_EXT_ENTROPY_H
