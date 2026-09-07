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

#ifndef BITSERY_ADAPTER_BUFFER_H
#define BITSERY_ADAPTER_BUFFER_H

#include "../bitsery.h"
#include "../details/adapter_bit_packing.h"
#include "../traits/core/traits.h"
#include <algorithm>
#include <cassert>
#include <cstring>

namespace bitsery {

template<typename Buffer, typename Config = DefaultConfig>
class InputBufferAdapter
  : public details::InputAdapterBaseCRTP<InputBufferAdapter<Buffer, Config>>
{
public:
  friend details::InputAdapterBaseCRTP<InputBufferAdapter<Buffer, Config>>;

  using BitPackingEnabled =
    details::InputAdapterBitPackingWrapper<InputBufferAdapter<Buffer, Config>>;
  using TConfig = Config;
  using TIterator = typename traits::BufferAdapterTraits<
    typename std::remove_const<Buffer>::type>::TConstIterator;
  using TValue = typename traits::BufferAdapterTraits<
    typename std::remove_const<Buffer>::type>::TValue;
  static_assert(
    details::IsDefined<TValue>::value,
    "Please define BufferAdapterTraits or include from <bitsery/traits/...>");
  static_assert(traits::ContainerTraits<
                  typename std::remove_const<Buffer>::type>::isContiguous,
                "BufferAdapter only works with contiguous containers");
  static_assert(sizeof(TValue) == 1,
                "BufferAdapter underlying type must be 1byte.");

  InputBufferAdapter(TIterator beginIt, size_t size)
    : _beginIt{ beginIt }
    , _currOffset{ 0 }
    , _endReadOffset{ size }
    , _bufferSize{ size } {};

  InputBufferAdapter(TIterator beginIt, TIterator endIt)
    : InputBufferAdapter(beginIt,
                         static_cast<size_t>(std::distance(beginIt, endIt)))
  {
  }

  InputBufferAdapter(const InputBufferAdapter&) = delete;
  InputBufferAdapter& operator=(const InputBufferAdapter&) = delete;

  InputBufferAdapter(InputBufferAdapter&&) = default;
  InputBufferAdapter& operator=(InputBufferAdapter&&) = default;

  void currentReadPos(size_t pos)
  {
    currentReadPosChecked(
      pos, std::integral_constant<bool, Config::CheckAdapterErrors>{});
  }

  size_t currentReadPos() const
  {
    return currentReadPosChecked(
      std::integral_constant<bool, Config::CheckAdapterErrors>{});
  }

  void currentReadEndPos(size_t pos)
  {
    // assert that CheckAdapterErrors is enabled, otherwise it will simply will
    // not work even if data and buffer is not corrupted
    static_assert(
      Config::CheckAdapterErrors,
      "Please enable CheckAdapterErrors to use this functionality.");
    if (_bufferSize >= pos && error() == ReaderError::NoError) {
      _overflowOnReadEndPos = pos == 0;
      if (pos == 0)
        pos = _bufferSize;
      _endReadOffset = pos;
    } else {
      error(ReaderError::DataOverflow);
    }
  }

  size_t currentReadEndPos() const
  {
    if (_overflowOnReadEndPos)
      return 0;
    return _endReadOffset;
  }

  ReaderError error() const
  {
    return _currOffset <= _endReadOffset
             ? ReaderError::NoError
             : static_cast<ReaderError>(_currOffset - _endReadOffset);
  }

  void error(ReaderError error)
  {
    if (_currOffset <= _endReadOffset) {
      _endReadOffset = 0;
      _bufferSize = 0;
      _currOffset = static_cast<size_t>(error);
    }
  }

  bool isCompletedSuccessfully() const { return _currOffset == _bufferSize; }

private:
  using diff_t = typename std::iterator_traits<TIterator>::difference_type;

  template<size_t SIZE>
  void readInternalValue(TValue* data)
  {
    readInternalImpl(
      data, SIZE, std::integral_constant<bool, Config::CheckAdapterErrors>{});
  }

  void readInternalBuffer(TValue* data, size_t size)
  {
    readInternalImpl(
      data, size, std::integral_constant<bool, Config::CheckAdapterErrors>{});
  }

  void readInternalImpl(TValue* data, size_t size, std::false_type)
  {
    const size_t newOffset = _currOffset + size;
    assert(newOffset <= _endReadOffset);
    std::copy_n(_beginIt + static_cast<diff_t>(_currOffset), size, data);
    _currOffset = newOffset;
  }

  void readInternalImpl(TValue* data, size_t size, std::true_type)
  {
    const size_t newOffset = _currOffset + size;
    if (newOffset <= _endReadOffset) {
      std::copy_n(_beginIt + static_cast<diff_t>(_currOffset), size, data);
      _currOffset = newOffset;
    } else {
      // set everything to zeros
      std::memset(data, 0, size);
      if (_overflowOnReadEndPos)
        error(ReaderError::DataOverflow);
    }
  }

  void currentReadPosChecked(size_t pos, std::true_type)
  {
    if (_bufferSize >= pos && error() == ReaderError::NoError) {
      _currOffset = pos;
    } else {
      error(ReaderError::DataOverflow);
    }
  }

  void currentReadPosChecked(size_t pos, std::false_type) { _currOffset = pos; }

  size_t currentReadPosChecked(std::true_type) const
  {
    return error() == ReaderError::NoError ? _currOffset : 0;
  }

  size_t currentReadPosChecked(std::false_type) const { return _currOffset; }

  TIterator _beginIt;
  size_t _currOffset;
  size_t _endReadOffset;
  size_t _bufferSize;
  bool _overflowOnReadEndPos = true;
};

template<typename Buffer, typename Config = DefaultConfig>
class OutputBufferAdapter
  : public details::OutputAdapterBaseCRTP<OutputBufferAdapter<Buffer, Config>>
{
public:
  friend details::OutputAdapterBaseCRTP<OutputBufferAdapter<Buffer, Config>>;

  using BitPackingEnabled = details::OutputAdapterBitPackingWrapper<
    OutputBufferAdapter<Buffer, Config>>;
  using TConfig = Config;
  using TIterator = typename traits::BufferAdapterTraits<Buffer>::TIterator;
  using TValue = typename traits::BufferAdapterTraits<Buffer>::TValue;

  static_assert(
    details::IsDefined<TValue>::value,
    "Please define BufferAdapterTraits or include from <bitsery/traits/...>");
  static_assert(traits::ContainerTraits<Buffer>::isContiguous,
                "BufferAdapter only works with contiguous containers");
  static_assert(sizeof(TValue) == 1,
                "BufferAdapter underlying type must be 1byte.");

  OutputBufferAdapter(Buffer& buffer)
    : _buffer{ std::addressof(buffer) }
    , _beginIt{ std::begin(buffer) }
    , _bufferSize{ traits::ContainerTraits<Buffer>::size(buffer) }
  {
  }

  OutputBufferAdapter(const OutputBufferAdapter&) = delete;
  OutputBufferAdapter& operator=(const OutputBufferAdapter&) = delete;
  OutputBufferAdapter(OutputBufferAdapter&&) = default;
  OutputBufferAdapter& operator=(OutputBufferAdapter&&) = default;

  void currentWritePos(size_t pos)
  {
    const auto maxPos = _currOffset > pos ? _currOffset : pos;
    if (maxPos > _biggestCurrentPos) {
      _biggestCurrentPos = maxPos;
    }
    maybeResize(pos, TResizable{});
    _currOffset = pos;
  }

  size_t currentWritePos() const { return _currOffset; }

  void flush()
  {
    // this function might be useful for stream adapters
  }

  size_t writtenBytesCount() const
  {
    return _currOffset > _biggestCurrentPos ? _currOffset : _biggestCurrentPos;
  }

private:
  using TResizable =
    std::integral_constant<bool, traits::ContainerTraits<Buffer>::isResizable>;
  using diff_t = typename std::iterator_traits<TIterator>::difference_type;

  template<size_t SIZE>
  void writeInternalValue(const TValue* data)
  {
    writeInternalImpl(data, SIZE);
  }

  void writeInternalBuffer(const TValue* data, size_t size)
  {
    writeInternalImpl(data, size);
  }

  Buffer* _buffer;
  TIterator _beginIt;
  size_t _currOffset{ 0 };
  size_t _bufferSize{ 0 };
  size_t _biggestCurrentPos{ 0 };

  void maybeResize(size_t newOffset, std::true_type)
  {
    if (newOffset > _bufferSize)
      BITSERY_UNLIKELY
      {
        doResize(newOffset);
      }
  }

  void maybeResize(size_t newOffset, std::false_type)
  {
    static_cast<void>(newOffset);
    assert(newOffset <= _bufferSize);
  }

  void writeInternalImpl(const TValue* data, size_t size)
  {
    const size_t newOffset = _currOffset + size;
    maybeResize(newOffset, TResizable{});
    std::copy_n(data, size, _beginIt + static_cast<diff_t>(_currOffset));
    _currOffset = newOffset;
  }

  BITSERY_NOINLINE void doResize(size_t newOffset)
  {
    traits::BufferAdapterTraits<Buffer>::increaseBufferSize(
      *_buffer, _currOffset, newOffset);
    _beginIt = std::begin(*_buffer);
    _bufferSize = traits::ContainerTraits<Buffer>::size(*_buffer);
  }
};

}

#endif // BITSERY_ADAPTER_BUFFER_H
