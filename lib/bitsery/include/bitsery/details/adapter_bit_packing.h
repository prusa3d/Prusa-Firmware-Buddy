// MIT License
//
// Copyright (c) 2022 Mindaugas Vinkelis
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

#ifndef BITSERY_DETAILS_ADAPTER_BIT_PACKING_H
#define BITSERY_DETAILS_ADAPTER_BIT_PACKING_H

#include "../common.h"
#include "./adapter_common.h"
#include "not_defined_type.h"
#include <limits>

namespace bitsery {

namespace details {

template<typename TAdapter>
class InputAdapterBitPackingWrapper
{
public:
  // in order to check if adapter is BP enabled, we use `std::is_same<Adapter,
  // typename Adapter::BitPackingEnabled>` so when current implementation is BP
  // enabled, we always specify current class as BitPackingEnabled.
  using BitPackingEnabled = InputAdapterBitPackingWrapper<TAdapter>;
  using TConfig = typename TAdapter::TConfig;
  using TValue = typename TAdapter::TValue;

  InputAdapterBitPackingWrapper(TAdapter& adapter)
    : _wrapped{ adapter }
  {
  }

  ~InputAdapterBitPackingWrapper() { align(); }

  template<size_t SIZE, typename T>
  void readBytes(T& v)
  {
    static_assert(std::is_integral<T>(), "");
    static_assert(sizeof(T) == SIZE, "");
    using UT = typename std::make_unsigned<T>::type;
    if (!m_scratchBits)
      this->_wrapped.template readBytes<SIZE, T>(v);
    else
      readBits(reinterpret_cast<UT&>(v), details::BitsSize<T>::value);
  }

  template<size_t SIZE, typename T>
  void readBuffer(T* buf, size_t count)
  {
    static_assert(std::is_integral<T>(), "");
    static_assert(sizeof(T) == SIZE, "");

    if (!m_scratchBits) {
      this->_wrapped.template readBuffer<SIZE, T>(buf, count);
    } else {
      using UT = typename std::make_unsigned<T>::type;
      // todo improve implementation
      const auto end = buf + count;
      for (auto it = buf; it != end; ++it)
        readBits(reinterpret_cast<UT&>(*it), details::BitsSize<T>::value);
    }
  }

  template<typename T>
  void readBits(T& v, size_t bitsCount)
  {
    static_assert(std::is_integral<T>() && std::is_unsigned<T>(), "");
    readBitsInternal(v, bitsCount);
  }

  void align()
  {
    if (m_scratchBits) {
      ScratchType tmp{};
      readBitsInternal(tmp, m_scratchBits);
      handleAlignErrors(
        tmp, std::integral_constant<bool, TConfig::CheckDataErrors>{});
    }
  }

  void currentReadPos(size_t pos)
  {
    align();
    this->_wrapped.currentReadPos(pos);
  }

  size_t currentReadPos() const { return this->_wrapped.currentReadPos(); }

  void currentReadEndPos(size_t pos) { this->_wrapped.currentReadEndPos(pos); }

  size_t currentReadEndPos() const
  {
    return this->_wrapped.currentReadEndPos();
  }

  bool isCompletedSuccessfully() const
  {
    return this->_wrapped.isCompletedSuccessfully();
  }

  ReaderError error() const { return this->_wrapped.error(); }

  void error(ReaderError error) { this->_wrapped.error(error); }

private:
  TAdapter& _wrapped;
  using UnsignedValue =
    typename std::make_unsigned<typename TAdapter::TValue>::type;
  using ScratchType = typename details::ScratchType<UnsignedValue>::type;

  ScratchType m_scratch{};
  size_t m_scratchBits{};

  template<typename T>
  void readBitsInternal(T& v, size_t size)
  {
    auto bitsLeft = size;
    using TFast = typename FastType<T>::type;
    TFast res{};
    while (bitsLeft > 0) {
      auto bits = (std::min)(bitsLeft, details::BitsSize<UnsignedValue>::value);
      if (m_scratchBits < bits) {
        UnsignedValue tmp;
        this->_wrapped.template readBytes<sizeof(UnsignedValue), UnsignedValue>(
          tmp);
        m_scratch |= static_cast<ScratchType>(tmp) << m_scratchBits;
        m_scratchBits += details::BitsSize<UnsignedValue>::value;
      }
      auto shiftedRes =
        static_cast<T>(m_scratch & ((static_cast<ScratchType>(1) << bits) - 1))
        << (size - bitsLeft);
      res = static_cast<TFast>(res | static_cast<TFast>(shiftedRes));
      m_scratch >>= bits;
      m_scratchBits -= bits;
      bitsLeft -= bits;
    }
    v = static_cast<T>(res);
  }

  void handleAlignErrors(ScratchType value, std::true_type)
  {
    if (value)
      error(ReaderError::InvalidData);
  }

  void handleAlignErrors(ScratchType, std::false_type) {}
};

template<typename TAdapter>
class BasicMeasureSizeBitPackingWrapper
{
public:
  using BitPackingEnabled = BasicMeasureSizeBitPackingWrapper<TAdapter>;
  using TConfig = typename TAdapter::TConfig;
  using TValue = typename TAdapter::TValue;

  BasicMeasureSizeBitPackingWrapper(TAdapter& adapter)
    : _wrapped{ adapter }
  {
  }

  ~BasicMeasureSizeBitPackingWrapper() { align(); }

  template<size_t SIZE, typename T>
  void writeBytes(const T& value)
  {
    _wrapped.template writeBytes<SIZE>(value);
  }

  template<size_t SIZE, typename T>
  void writeBuffer(const T* buf, size_t count)
  {
    _wrapped.template writeBuffer<SIZE>(buf, count);
  }

  template<typename T>
  void writeBits(const T&, size_t bitsCount)
  {
    _scratchBits += bitsCount;
    while (_scratchBits >= 8) {
      writeOneByte();
      _scratchBits -= 8;
    }
  }

  void align()
  {
    if (_scratchBits > 0) {
      _scratchBits = 0;
      writeOneByte();
    }
  }

  void currentWritePos(size_t pos)
  {
    align();
    this->_wrapped.currentWritePos(pos);
  }

  size_t currentWritePos() const { return this->_wrapped.currentWritePos(); }

  void flush()
  {
    align();
    this->_wrapped.flush();
  }

  size_t writtenBytesCount() const
  {
    return this->_wrapped.writtenBytesCount();
  }

private:
  void writeOneByte() { _wrapped.template writeBytes<1>(uint8_t{}); }
  TAdapter& _wrapped;
  size_t _scratchBits{};
};

template<typename TAdapter>
class OutputAdapterBitPackingWrapper
{
public:
  // in order to check if adapter is BP enabled, we use `std::is_same<Adapter,
  // typename Adapter::BitPackingEnabled>` so when current implementation is BP
  // enabled, we always specify current class as BitPackingEnabled.
  using BitPackingEnabled = OutputAdapterBitPackingWrapper<TAdapter>;
  using TConfig = typename TAdapter::TConfig;
  using TValue = typename TAdapter::TValue;

  OutputAdapterBitPackingWrapper(TAdapter& adapter)
    : _wrapped{ adapter }
  {
  }

  ~OutputAdapterBitPackingWrapper() { align(); }

  template<size_t SIZE, typename T>
  void writeBytes(const T& v)
  {
    static_assert(std::is_integral<T>(), "");
    static_assert(sizeof(T) == SIZE, "");

    if (!_scratchBits) {
      this->_wrapped.template writeBytes<SIZE, T>(v);
    } else {
      using UT = typename std::make_unsigned<T>::type;
      writeBitsInternal(reinterpret_cast<const UT&>(v),
                        details::BitsSize<T>::value);
    }
  }

  template<size_t SIZE, typename T>
  void writeBuffer(const T* buf, size_t count)
  {
    static_assert(std::is_integral<T>(), "");
    static_assert(sizeof(T) == SIZE, "");
    if (!_scratchBits) {
      this->_wrapped.template writeBuffer<SIZE, T>(buf, count);
    } else {
      using UT = typename std::make_unsigned<T>::type;
      // todo improve implementation
      const auto end = buf + count;
      for (auto it = buf; it != end; ++it)
        writeBitsInternal(reinterpret_cast<const UT&>(*it),
                          details::BitsSize<T>::value);
    }
  }

  template<typename T>
  void writeBits(const T& v, size_t bitsCount)
  {
    static_assert(std::is_integral<T>() && std::is_unsigned<T>(), "");
    assert(0 < bitsCount && bitsCount <= details::BitsSize<T>::value);
    assert(v <= (bitsCount < 64 ? (1ULL << bitsCount) - 1
                                : (1ULL << (bitsCount - 1)) +
                                    ((1ULL << (bitsCount - 1)) - 1)));
    writeBitsInternal(v, bitsCount);
  }

  void align()
  {
    writeBitsInternal(UnsignedType{},
                      (details::BitsSize<UnsignedType>::value - _scratchBits) %
                        8);
  }

  void currentWritePos(size_t pos)
  {
    align();
    this->_wrapped.currentWritePos(pos);
  }

  size_t currentWritePos() const { return this->_wrapped.currentWritePos(); }

  void flush()
  {
    align();
    this->_wrapped.flush();
  }

  size_t writtenBytesCount() const
  {
    return this->_wrapped.writtenBytesCount();
  }

private:
  TAdapter& _wrapped;

  using UnsignedType =
    typename std::make_unsigned<typename TAdapter::TValue>::type;
  using ScratchType = typename details::ScratchType<UnsignedType>::type;
  static_assert(details::IsDefined<ScratchType>::value,
                "Underlying adapter value type is not supported");

  template<typename T>
  void writeBitsInternal(const T& v, size_t size)
  {
    constexpr size_t valueSize = details::BitsSize<UnsignedType>::value;
    T value = v;
    size_t bitsLeft = size;
    while (bitsLeft > 0) {
      auto bits = (std::min)(bitsLeft, valueSize);
      _scratch |= static_cast<ScratchType>(value) << _scratchBits;
      _scratchBits += bits;
      if (_scratchBits >= valueSize) {
        auto tmp = static_cast<UnsignedType>(_scratch & _MASK);
        this->_wrapped.template writeBytes<sizeof(UnsignedType), UnsignedType>(
          tmp);
        _scratch >>= valueSize;
        _scratchBits -= valueSize;

        value = static_cast<T>(value >> valueSize);
      }
      bitsLeft -= bits;
    }
  }

  // overload for TValue, for better performance
  void writeBitsInternal(const UnsignedType& v, size_t size)
  {
    if (size > 0) {
      _scratch |= static_cast<ScratchType>(v) << _scratchBits;
      _scratchBits += size;
      if (_scratchBits >= details::BitsSize<UnsignedType>::value) {
        auto tmp = static_cast<UnsignedType>(_scratch & _MASK);
        this->_wrapped.template writeBytes<sizeof(UnsignedType), UnsignedType>(
          tmp);
        _scratch >>= details::BitsSize<UnsignedType>::value;
        _scratchBits -= details::BitsSize<UnsignedType>::value;
      }
    }
  }

  const UnsignedType _MASK = (std::numeric_limits<UnsignedType>::max)();
  ScratchType _scratch{};
  size_t _scratchBits{};
};
}
}

#endif // BITSERY_DETAILS_ADAPTER_BIT_PACKING_H
