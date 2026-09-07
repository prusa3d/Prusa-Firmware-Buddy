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

#ifndef BITSERY_EXT_STD_VARIANT_H
#define BITSERY_EXT_STD_VARIANT_H

#include "../traits/core/traits.h"
#include "utils/composite_type_overloads.h"
#include <variant>

namespace bitsery {
namespace ext {

template<typename... Overloads>
class StdVariant
  : public details::CompositeTypeOverloadsUtils<std::variant, Overloads...>
{
public:
  template<typename Ser, typename Fnc, typename... Ts>
  void serialize(Ser& ser, const std::variant<Ts...>& obj, Fnc&&) const
  {
    auto index = obj.index();
    assert(index != std::variant_npos);
    details::writeSize(ser.adapter(), index);
    this->execIndex(index,
                    const_cast<std::variant<Ts...>&>(obj),
                    [this, &ser](auto& data, auto index) {
                      constexpr size_t Index = decltype(index)::value;
                      this->serializeType(ser, std::get<Index>(data));
                    });
  }

  template<typename Des, typename Fnc, typename... Ts>
  void deserialize(Des& des, std::variant<Ts...>& obj, Fnc&&) const
  {
    size_t index{};
    details::readSize(
      des.adapter(),
      index,
      sizeof...(Ts),
      std::integral_constant<bool, Des::TConfig::CheckDataErrors>{});
    this->execIndex(index, obj, [this, &des](auto& data, auto index) {
      constexpr size_t Index = decltype(index)::value;
      using TElem =
        typename std::variant_alternative<Index, std::variant<Ts...>>::type;

      // Reinitializing nontrivial types may be expensive especially when they
      // reference heap data, so if `data` is already holding the requested
      // variant then we'll deserialize into the existing object
      if constexpr (!std::is_trivial_v<TElem>) {
        if (auto item = std::get_if<Index>(&data)) {
          this->serializeType(des, *item);
          return;
        }
      }

      TElem item = ::bitsery::Access::create<TElem>();
      this->serializeType(des, item);
      data =
        std::variant<Ts...>(std::in_place_index_t<Index>{}, std::move(item));
    });
  }
};

// deduction guide
template<typename... Overloads>
StdVariant(Overloads...) -> StdVariant<Overloads...>;
}

// defines empty fuction, that handles monostate
template<typename S>
void
serialize(S&, std::monostate&)
{
}

namespace traits {

template<typename Variant, typename... Overloads>
struct ExtensionTraits<ext::StdVariant<Overloads...>, Variant>
{
  static_assert(
    bitsery::details::IsSpecializationOf<Variant, std::variant>::value,
    "StdVariant only works with std::variant");
  using TValue = void;
  static constexpr bool SupportValueOverload = false;
  static constexpr bool SupportObjectOverload = true;
  static constexpr bool SupportLambdaOverload = false;
};

}

}

#endif // BITSERY_EXT_STD_VARIANT_H
