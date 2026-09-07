// MIT License
//
// Copyright (c) 2020 Mindaugas Vinkelis
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

#ifndef BITSERY_EXT_STD_BITSET_H
#define BITSERY_EXT_STD_BITSET_H

#include "../traits/core/traits.h"
#include <bitset>
#include <cstdint>

namespace bitsery {
namespace ext {

class StdBitset
{
public:
  template<typename Ser, typename Fnc, size_t N>
  void serialize(Ser& ser, const std::bitset<N>& obj, Fnc&&) const
  {
    constexpr size_t BYTES = N / 8;
    constexpr size_t LEFTOVER = N % 8;
    if (BYTES > sizeof(unsigned long long)) {
      for (size_t i = 0u; i < BYTES; ++i) {
        size_t offset = i * 8;
        auto data = obj[offset + 0] + (obj[offset + 1] << 1) +
                    (obj[offset + 2] << 2) + (obj[offset + 3] << 3) +
                    (obj[offset + 4] << 4) + (obj[offset + 5] << 5) +
                    (obj[offset + 6] << 6) + (obj[offset + 7] << 7);
        ser.value1b(static_cast<uint8_t>(data));
      }

    } else {
      // more performant way
      auto data = obj.to_ullong();
      for (size_t i = 0u; i < BYTES; ++i) {
        ser.value1b(static_cast<uint8_t>(data & 0xFF));
        data >>= 8;
      }
    }
    if (LEFTOVER > 0) {
      serializeLeftoverImpl(ser.adapter(),
                            obj,
                            N - LEFTOVER,
                            N,
                            std::is_same<Ser, typename Ser::BPEnabledType>{});
    }
  }

  template<typename Des, typename Fnc, size_t N>
  void deserialize(Des& des, std::bitset<N>& obj, Fnc&&) const
  {
    constexpr size_t BYTES = N / 8;
    constexpr size_t LEFTOVER = N % 8;
    for (size_t i = 0u; i < BYTES; ++i) {
      size_t offset = i * 8;
      uint8_t data = 0;
      des.value1b(data);
      obj[offset + 0] = data & 0x01u;
      obj[offset + 1] = data & 0x02u;
      obj[offset + 2] = data & 0x04u;
      obj[offset + 3] = data & 0x08u;
      obj[offset + 4] = data & 0x10u;
      obj[offset + 5] = data & 0x20u;
      obj[offset + 6] = data & 0x40u;
      obj[offset + 7] = data & 0x80u;
    }
    if (LEFTOVER > 0) {
      deserializeLeftoverImpl(des.adapter(),
                              obj,
                              N - LEFTOVER,
                              N,
                              std::is_same<Des, typename Des::BPEnabledType>{});
    }
  }

private:
  template<typename Writer, size_t N>
  void serializeLeftoverImpl(Writer& w,
                             const std::bitset<N>& obj,
                             size_t from,
                             size_t to,
                             std::integral_constant<bool, false>) const
  {
    auto data = 0;
    for (auto i = from; i < to; ++i) {
      data += obj[i] << (i - from);
    }
    w.template writeBytes<1>(static_cast<uint8_t>(data));
  }

  template<typename Writer, size_t N>
  void serializeLeftoverImpl(Writer& w,
                             const std::bitset<N>& obj,
                             size_t from,
                             size_t to,
                             std::integral_constant<bool, true>) const
  {
    for (auto i = from; i < to; ++i) {
      w.writeBits(obj[i] ? 1u : 0u, 1);
    }
  }

  template<typename Reader, size_t N>
  void deserializeLeftoverImpl(Reader& r,
                               std::bitset<N>& obj,
                               size_t from,
                               size_t to,
                               std::integral_constant<bool, false>) const
  {
    uint8_t data = 0u;
    r.template readBytes<1>(data);
    for (auto i = from; i < to; ++i) {
      obj[i] = data & (1u << (i - from));
    }
  }

  template<typename Reader, size_t N>
  void deserializeLeftoverImpl(Reader& r,
                               std::bitset<N>& obj,
                               size_t from,
                               size_t to,
                               std::integral_constant<bool, true>) const
  {
    for (auto i = from; i < to; ++i) {
      uint8_t res = 0u;
      r.readBits(res, 1);
      obj[i] = res == 1;
    }
  }
};
}

namespace traits {
template<size_t N>
struct ExtensionTraits<ext::StdBitset, std::bitset<N>>
{
  using TValue = void;
  static constexpr bool SupportValueOverload = false;
  static constexpr bool SupportObjectOverload = true;
  static constexpr bool SupportLambdaOverload = false;
};
}
}

#endif // BITSERY_EXT_STD_BITSET_H
