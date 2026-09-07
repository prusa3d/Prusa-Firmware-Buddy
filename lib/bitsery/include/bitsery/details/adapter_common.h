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

#ifndef BITSERY_DETAILS_ADAPTER_COMMON_H
#define BITSERY_DETAILS_ADAPTER_COMMON_H

#include "../common.h"
#include "not_defined_type.h"
#include <algorithm>
#include <cassert>
#include <climits>
#include <cstdint>

namespace bitsery {

enum class ReaderError
{
  NoError,
  ReadingError, // this might be used with stream adapter
  DataOverflow,
  InvalidData,
  InvalidPointer
};

namespace details {

/**
 * size read/write functions
 */

template<typename Reader>
void
handleReadMaxSize(Reader& r, size_t& size, size_t maxSize, std::true_type)
{
  if (size > maxSize) {
    r.error(ReaderError::InvalidData);
    size = {};
  }
}

template<typename Reader>
void
handleReadMaxSize(Reader&, size_t&, size_t, std::false_type)
{
}

template<typename Reader, bool CheckMaxSize>
void
readSize(Reader& r,
         size_t& size,
         size_t maxSize,
         std::integral_constant<bool, CheckMaxSize> checkMaxSize)
{
  uint8_t hb{};
  r.template readBytes<1>(hb);
  if (hb < 0x80u) {
    size = hb;
  } else {
    uint8_t lb{};
    r.template readBytes<1>(lb);
    if (hb & 0x40u) {
      uint16_t lw{};
      r.template readBytes<2>(lw);
      size = ((((hb & 0x3Fu) << 8) | lb) << 16) | lw;
    } else {
      size = ((hb & 0x7Fu) << 8) | lb;
    }
  }
  handleReadMaxSize(r, size, maxSize, checkMaxSize);
}

template<typename Writer>
void
writeSize(Writer& w, const size_t size)
{
  if (size < 0x80u) {
    w.template writeBytes<1>(static_cast<uint8_t>(size));
  } else {
    if (size < 0x4000u) {
      w.template writeBytes<1>(static_cast<uint8_t>((size >> 8) | 0x80u));
      w.template writeBytes<1>(static_cast<uint8_t>(size));
    } else {
      assert(size < 0x40000000u);
      w.template writeBytes<1>(static_cast<uint8_t>((size >> 24) | 0xC0u));
      w.template writeBytes<1>(static_cast<uint8_t>(size >> 16));
      w.template writeBytes<2>(static_cast<uint16_t>(size));
    }
  }
}

/**
 * swap utils
 */

// add swap functions to class, to avoid compilation warning about unused
// functions
struct SwapImpl
{
  static uint64_t exec(uint64_t value)
  {
#ifdef __GNUC__
    return __builtin_bswap64(value);
#else
    value =
      (value & 0x00000000FFFFFFFF) << 32 | (value & 0xFFFFFFFF00000000) >> 32;
    value =
      (value & 0x0000FFFF0000FFFF) << 16 | (value & 0xFFFF0000FFFF0000) >> 16;
    value =
      (value & 0x00FF00FF00FF00FF) << 8 | (value & 0xFF00FF00FF00FF00) >> 8;
    return value;
#endif
  }

  static uint32_t exec(uint32_t value)
  {
#ifdef __GNUC__
    return __builtin_bswap32(value);
#else
    return (value & 0x000000ff) << 24 | (value & 0x0000ff00) << 8 |
           (value & 0x00ff0000) >> 8 | (value & 0xff000000) >> 24;
#endif
  }

  static uint16_t exec(uint16_t value)
  {
    return static_cast<uint16_t>((value & 0x00ff) << 8 | (value & 0xff00) >> 8);
  }

  static uint8_t exec(uint8_t value) { return value; }
};

template<typename TValue>
TValue
swap(TValue value)
{
  constexpr size_t TSize = sizeof(TValue);
  using UT = typename std::conditional<
    TSize == 1,
    uint8_t,
    typename std::conditional<
      TSize == 2,
      uint16_t,
      typename std::conditional<TSize == 4, uint32_t, uint64_t>::type>::type>::
    type;
  return static_cast<TValue>(SwapImpl::exec(static_cast<UT>(value)));
}

/**
 * endianness utils
 */
// add test data in separate struct, because some compilers only support
// constexpr functions with return-only body suppress msvc warnings.
#ifdef _MSC_VER
#pragma warning(disable : 4310)
#endif
struct EndiannessTestData
{
  static constexpr uint32_t _sample4Bytes = 0x01020304;
  static constexpr uint8_t _sample1stByte = (const uint8_t&)_sample4Bytes;
};
#ifdef _MSC_VER
#pragma warning(default : 4310)
#endif

constexpr EndiannessType
getSystemEndianness()
{
  static_assert(EndiannessTestData::_sample1stByte == 0x04 ||
                  EndiannessTestData::_sample1stByte == 0x01,
                "system must be either little or big endian");
  return EndiannessTestData::_sample1stByte == 0x04
           ? EndiannessType::LittleEndian
           : EndiannessType::BigEndian;
}

template<typename Config, typename T>
using ShouldSwap =
  std::integral_constant<bool,
                         Config::Endianness != details::getSystemEndianness() &&
                           sizeof(T) != 1>;

/**
 * helper types to work with bits
 */
template<typename T>
struct BitsSize : public std::integral_constant<size_t, sizeof(T) * 8>
{
  static_assert(CHAR_BIT == 8, "only support systems with byte size of 8 bits");
};

template<typename T>
struct ScratchType
{
  using type = NotDefinedType;
};

template<>
struct ScratchType<uint8_t>
{
  using type = uint_fast16_t;
};

template<typename T>
struct FastType
{
  using type = T;
};

template<>
struct FastType<uint8_t>
{
  using type = uint_fast8_t;
};

template<>
struct FastType<uint16_t>
{
  using type = uint_fast16_t;
};

template<>
struct FastType<uint32_t>
{
  using type = uint_fast32_t;
};

template<>
struct FastType<uint64_t>
{
  using type = uint_fast64_t;
};

template<>
struct FastType<int8_t>
{
  using type = int_fast8_t;
};

template<>
struct FastType<int16_t>
{
  using type = int_fast16_t;
};

template<>
struct FastType<int32_t>
{
  using type = int_fast32_t;
};

template<>
struct FastType<int64_t>
{
  using type = int_fast64_t;
};

/**
 * output/input adapter base that handles endianness
 */

template<typename Adapter>
struct OutputAdapterBaseCRTP
{

  template<size_t SIZE, typename T>
  void writeBytes(const T& v)
  {
    static_assert(std::is_integral<T>(), "");
    static_assert(sizeof(T) == SIZE, "");
    writeSwappedValue(&v, ShouldSwap<typename Adapter::TConfig, T>{});
  }

  template<size_t SIZE, typename T>
  void writeBuffer(const T* buf, size_t count)
  {
    static_assert(std::is_integral<T>(), "");
    static_assert(sizeof(T) == SIZE, "");
    writeSwappedBuffer(buf, count, ShouldSwap<typename Adapter::TConfig, T>{});
  }

  template<typename T>
  void writeBits(const T&, size_t)
  {
    static_assert(
      std::is_void<T>::value,
      "Bit-packing is not enabled.\nEnable by call to `enableBitPacking`) or "
      "create Serializer with bit packing enabled.");
  }

  void align() {}

  OutputAdapterBaseCRTP() = default;
  OutputAdapterBaseCRTP(const OutputAdapterBaseCRTP&) = delete;
  OutputAdapterBaseCRTP& operator=(const OutputAdapterBaseCRTP&) = delete;
  OutputAdapterBaseCRTP(OutputAdapterBaseCRTP&&) = default;
  OutputAdapterBaseCRTP& operator=(OutputAdapterBaseCRTP&&) = default;

private:
  template<typename T>
  void writeSwappedValue(const T* v, std::true_type)
  {
    const auto res = details::swap(*v);
    static_cast<Adapter*>(this)->template writeInternalValue<sizeof(T)>(
      reinterpret_cast<const typename Adapter::TValue*>(&res));
  }

  template<typename T>
  void writeSwappedValue(const T* v, std::false_type)
  {
    static_cast<Adapter*>(this)->template writeInternalValue<sizeof(T)>(
      reinterpret_cast<const typename Adapter::TValue*>(v));
  }

  template<typename T>
  void writeSwappedBuffer(const T* v, size_t count, std::true_type)
  {
    std::for_each(v, std::next(v, count), [this](const T& inner_v) {
      const auto res = details::swap(inner_v);
      static_cast<Adapter*>(this)->template writeInternalValue<sizeof(T)>(
        reinterpret_cast<const typename Adapter::TValue*>(&res));
    });
  }

  template<typename T>
  void writeSwappedBuffer(const T* v, size_t count, std::false_type)
  {
    static_cast<Adapter*>(this)->writeInternalBuffer(
      reinterpret_cast<const typename Adapter::TValue*>(v), count * sizeof(T));
  }
};

template<typename Adapter>
struct InputAdapterBaseCRTP
{

  template<size_t SIZE, typename T>
  void readBytes(T& v)
  {
    static_assert(std::is_integral<T>(), "");
    static_assert(sizeof(T) == SIZE, "");
    static_cast<Adapter*>(this)->template readInternalValue<sizeof(T)>(
      reinterpret_cast<typename Adapter::TValue*>(&v));
    swapDataBits(v, ShouldSwap<typename Adapter::TConfig, T>{});
  }

  template<size_t SIZE, typename T>
  void readBuffer(T* buf, size_t count)
  {
    static_assert(std::is_integral<T>(), "");
    static_assert(sizeof(T) == SIZE, "");
    static_cast<Adapter*>(this)->readInternalBuffer(
      reinterpret_cast<typename Adapter::TValue*>(buf), sizeof(T) * count);
    swapDataBits(buf, count, ShouldSwap<typename Adapter::TConfig, T>{});
  }

  template<typename T>
  void readBits(T&, size_t)
  {
    static_assert(
      std::is_void<T>::value,
      "Bit-packing is not enabled.\nEnable by call to `enableBitPacking`) or "
      "create Deserializer with bit packing enabled.");
  }

  void align() {}

  InputAdapterBaseCRTP() = default;
  InputAdapterBaseCRTP(const InputAdapterBaseCRTP&) = delete;
  InputAdapterBaseCRTP& operator=(const InputAdapterBaseCRTP&) = delete;

  InputAdapterBaseCRTP(InputAdapterBaseCRTP&&) = default;
  InputAdapterBaseCRTP& operator=(InputAdapterBaseCRTP&&) = default;

  virtual ~InputAdapterBaseCRTP() = default;

private:
  template<typename T>
  void swapDataBits(T* v, size_t count, std::true_type)
  {
    using diff_t = typename std::iterator_traits<T*>::difference_type;
    std::for_each(v, std::next(v, static_cast<diff_t>(count)), [](T& x) {
      x = details::swap(x);
    });
  }

  template<typename T>
  void swapDataBits(T*, size_t, std::false_type)
  {
    // empty function because no swap is required
  }

  template<typename T>
  void swapDataBits(T& v, std::true_type)
  {
    v = details::swap(v);
  }

  template<typename T>
  void swapDataBits(T&, std::false_type)
  {
    // empty function because no swap is required
  }
};
}
}

#endif // BITSERY_DETAILS_ADAPTER_COMMON_H
