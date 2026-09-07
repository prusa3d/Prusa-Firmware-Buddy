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

#ifndef BITSERY_ADAPTER_MEASURE_SIZE_H
#define BITSERY_ADAPTER_MEASURE_SIZE_H

#include "../details/adapter_bit_packing.h"
#include <cstddef>
#include <type_traits>

namespace bitsery {

template<typename Config>
class BasicMeasureSize
{
public:
  using BitPackingEnabled =
    details::BasicMeasureSizeBitPackingWrapper<BasicMeasureSize<Config>>;
  using TConfig = Config;
  using TValue = void;

  template<size_t SIZE, typename T>
  void writeBytes(const T&)
  {
    static_assert(std::is_integral<T>(), "");
    static_assert(sizeof(T) == SIZE, "");
    _currPos += SIZE;
  }

  template<size_t SIZE, typename T>
  void writeBuffer(const T*, size_t count)
  {
    static_assert(std::is_integral<T>(), "");
    static_assert(sizeof(T) == SIZE, "");
    _currPos += SIZE * count;
  }

  void currentWritePos(size_t pos)
  {
    const auto maxPos = _currPos > pos ? _currPos : pos;
    if (maxPos > _biggestCurrentPos) {
      _biggestCurrentPos = maxPos;
    }
    _currPos = pos;
  }

  size_t currentWritePos() const { return _currPos; }

  void align() {}

  void flush() {}

  // get size in bytes
  size_t writtenBytesCount() const
  {
    return _currPos > _biggestCurrentPos ? _currPos : _biggestCurrentPos;
  }

private:
  size_t _biggestCurrentPos{};
  size_t _currPos{};
};

// helper type for default config
using MeasureSize = BasicMeasureSize<DefaultConfig>;

}

#endif // BITSERY_ADAPTER_MEASURE_SIZE_H
