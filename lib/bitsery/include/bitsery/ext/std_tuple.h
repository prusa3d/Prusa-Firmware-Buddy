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

#ifndef BITSERY_EXT_STD_TUPLE_H
#define BITSERY_EXT_STD_TUPLE_H

#include "../traits/core/traits.h"
#include "utils/composite_type_overloads.h"
#include <tuple>

namespace bitsery {
namespace ext {

template<typename... Overloads>
class StdTuple
  : public details::CompositeTypeOverloadsUtils<std::tuple, Overloads...>
{
public:
  template<typename Ser, typename Fnc, typename... Ts>
  void serialize(Ser& ser, const std::tuple<Ts...>& obj, Fnc&&) const
  {
    serializeAll(ser, const_cast<std::tuple<Ts...>&>(obj));
  }

  template<typename Des, typename Fnc, typename... Ts>
  void deserialize(Des& des, std::tuple<Ts...>& obj, Fnc&&) const
  {
    serializeAll(des, obj);
  }

private:
  template<typename S, typename... Ts>
  void serializeAll(S& s, std::tuple<Ts...>& obj) const
  {
    this->execAll(obj, [this, &s](auto& data, auto index) {
      constexpr size_t Index = decltype(index)::value;
      this->serializeType(s, std::get<Index>(data));
    });
  }
};

// deduction guide
template<typename... Overloads>
StdTuple(Overloads...) -> StdTuple<Overloads...>;
}

namespace traits {

template<typename Tuple, typename... Overloads>
struct ExtensionTraits<ext::StdTuple<Overloads...>, Tuple>
{
  static_assert(bitsery::details::IsSpecializationOf<Tuple, std::tuple>::value,
                "StdTuple only works with std::tuple");
  using TValue = void;
  static constexpr bool SupportValueOverload = false;
  static constexpr bool SupportObjectOverload = true;
  static constexpr bool SupportLambdaOverload = false;
};

}

}

#endif // BITSERY_EXT_STD_TUPLE_H
