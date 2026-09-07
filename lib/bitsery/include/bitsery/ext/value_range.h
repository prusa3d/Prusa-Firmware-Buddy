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

#ifndef BITSERY_EXT_VALUE_RANGE_H
#define BITSERY_EXT_VALUE_RANGE_H

#include "../details/serialization_common.h"

namespace bitsery {

namespace ext {
// this class is used to make default RangeSpec float specialization always
// prefer constructor with precision
struct BitsConstraint
{
  explicit constexpr BitsConstraint(size_t bits)
    : value{ bits }
  {
  }

  const size_t value;
};
}

// implementation details for range functionality
namespace details {

template<typename T>
constexpr size_t
getSize(T v, size_t s)
{
  return v > 0 ? getSize(v / 2, s + 1) : s;
}

template<typename T>
constexpr size_t
calcRequiredBits(T min, T max)
{
  // call recursive function, because some compilers only support constexpr
  // functions with return-only body
  return getSize(max - min, 0);
}

template<typename T, typename Enable = void>
struct RangeSpec
{

  constexpr RangeSpec(T minValue, T maxValue)
    : min{ minValue }
    , max{ maxValue }
    , bitsRequired{ calcRequiredBits(min, max) }
  {
  }

  const T min;
  const T max;
  const size_t bitsRequired;
};

template<typename T>
struct RangeSpec<T, typename std::enable_if<std::is_enum<T>::value>::type>
{

  constexpr RangeSpec(T minValue, T maxValue)
    : min{ minValue }
    , max{ maxValue }
    , bitsRequired{ calcRequiredBits(
        static_cast<typename std::underlying_type<T>::type>(min),
        static_cast<typename std::underlying_type<T>::type>(max)) }
  {
  }

  const T min;
  const T max;
  const size_t bitsRequired;
};

template<typename T>
struct RangeSpec<
  T,
  typename std::enable_if<std::is_floating_point<T>::value>::type>
{

  constexpr RangeSpec(T minValue, T maxValue, ext::BitsConstraint bits)
    : min{ minValue }
    , max{ maxValue }
    , bitsRequired{ bits.value }
  {
  }

  constexpr RangeSpec(T minValue, T maxValue, T precision)
    : min{ minValue }
    , max{ maxValue }
    , bitsRequired{ calcRequiredBits<details::SameSizeUnsigned<T>>(
        {},
        static_cast<details::SameSizeUnsigned<T>>((max - min) / precision)) }
  {
  }

  const T min;
  const T max;
  const size_t bitsRequired;
};

template<typename T,
         typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
details::SameSizeUnsigned<T>
getRangeValue(const T& v, const RangeSpec<T>& r)
{
  return static_cast<details::SameSizeUnsigned<T>>(v - r.min);
}

template<typename T,
         typename std::enable_if<std::is_enum<T>::value>::type* = nullptr>
details::SameSizeUnsigned<T>
getRangeValue(const T& v, const RangeSpec<T>& r)
{
  using VT = details::SameSizeUnsigned<T>;
  return static_cast<VT>(static_cast<VT>(v) - static_cast<VT>(r.min));
}

template<
  typename T,
  typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr>
details::SameSizeUnsigned<T>
getRangeValue(const T& v, const RangeSpec<T>& r)
{
  using VT = details::SameSizeUnsigned<T>;
  const VT maxUint = (static_cast<VT>(1) << r.bitsRequired) - 1;
  const T ratio = (v - r.min) / (r.max - r.min);
  return static_cast<VT>(ratio * static_cast<T>(maxUint));
}

template<typename T,
         typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
void
setRangeValue(T& v, const RangeSpec<T>& r)
{
  v += r.min;
}

template<typename T,
         typename std::enable_if<std::is_enum<T>::value>::type* = nullptr>
void
setRangeValue(T& v, const RangeSpec<T>& r)
{
  using VT = typename std::underlying_type<T>::type;
  reinterpret_cast<VT&>(v) += static_cast<VT>(r.min);
}

template<
  typename T,
  typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr>
void
setRangeValue(T& v, const RangeSpec<T>& r)
{
  using UIT = details::SameSizeUnsigned<T>;
  const auto intRep = reinterpret_cast<UIT&>(v);
  const UIT maxUint = (static_cast<UIT>(1) << r.bitsRequired) - 1;
  v = r.min +
      (static_cast<T>(intRep) / static_cast<T>(maxUint)) * (r.max - r.min);
}

template<typename T,
         typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr>
bool
isRangeValid(const T& v, const RangeSpec<T>& r)
{
  return !(r.min > v || v > r.max);
}

template<typename T,
         typename std::enable_if<std::is_enum<T>::value>::type* = nullptr>
bool
isRangeValid(const T& v, const RangeSpec<T>& r)
{
  using VT = typename std::underlying_type<T>::type;
  return !(static_cast<VT>(r.min) > static_cast<VT>(v) ||
           static_cast<VT>(v) > static_cast<VT>(r.max));
}
}

namespace ext {

template<typename TValue>
class ValueRange
{
public:
  template<typename... Args>
  constexpr ValueRange(const TValue& min, const TValue& max, Args&&... args)
    : _range{ min, max, std::forward<Args>(args)... }
  {
  }

  template<typename Ser, typename T, typename Fnc>
  void serialize(Ser& ser, const T& v, Fnc&&) const
  {
    assert(details::isRangeValid(v, _range));
    using BT = decltype(details::getRangeValue(v, _range));
    ser.adapter().template writeBits<BT>(details::getRangeValue(v, _range),
                                         _range.bitsRequired);
  }

  template<typename Des, typename T, typename Fnc>
  void deserialize(Des& des, T& v, Fnc&&) const
  {
    auto& reader = des.adapter();
    reader.readBits(reinterpret_cast<details::SameSizeUnsigned<T>&>(v),
                    _range.bitsRequired);
    details::setRangeValue(v, _range);
    handleInvalidRange(
      reader, v, std::integral_constant<bool, Des::TConfig::CheckDataErrors>{});
  }

  constexpr size_t getRequiredBits() const { return _range.bitsRequired; };

private:
  template<typename Reader, typename T>
  void handleInvalidRange(Reader& reader, T& v, std::true_type) const
  {
    if (!details::isRangeValid(v, _range)) {
      reader.error(ReaderError::InvalidData);
      v = _range.min;
    }
  }

  template<typename Reader, typename T>
  void handleInvalidRange(Reader&, T&, std::false_type) const
  {
  }

  details::RangeSpec<TValue> _range;
};
}

namespace traits {
template<typename T>
struct ExtensionTraits<ext::ValueRange<T>, T>
{
  using TValue = void;
  static constexpr bool SupportValueOverload = false;
  static constexpr bool SupportObjectOverload = true;
  static constexpr bool SupportLambdaOverload = false;
};
}

}

#endif // BITSERY_EXT_VALUE_RANGE_H
