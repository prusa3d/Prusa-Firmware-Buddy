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

#ifndef BITSERY_EXT_GROWABLE_H
#define BITSERY_EXT_GROWABLE_H

#include "../traits/core/traits.h"
#include <cstdint>

namespace bitsery {

namespace ext {

/*
 * enables forward and backward compatibility, by allowing to append additional
 * data at the end of serialization old deserialization method will ignore
 * additional data by jumping through it at the end of deserialization flow new
 * deserialization method will read all 0 for new fields if there is no data for
 * it
 */
class Growable
{
public:
  template<typename Ser, typename T, typename Fnc>
  void serialize(Ser& ser, const T& obj, Fnc&& fnc) const
  {
    auto& writer = ser.adapter();
    const auto startPos = writer.currentWritePos();
    writer.template writeBytes<4>(static_cast<uint32_t>(0));

    fnc(ser, const_cast<T&>(obj));

    const auto endPos = writer.currentWritePos();
    writer.currentWritePos(startPos);
    writer.template writeBytes<4>(static_cast<uint32_t>(endPos - startPos));
    writer.currentWritePos(endPos);
  }

  template<typename Des, typename T, typename Fnc>
  void deserialize(Des& des, T& obj, Fnc&& fnc) const
  {
    auto& reader = des.adapter();
    uint32_t size{};
    const auto readEndPos = reader.currentReadEndPos();
    const auto startPos = reader.currentReadPos();
    reader.template readBytes<4>(size);
    reader.currentReadEndPos(startPos + size);

    fnc(des, obj);

    reader.currentReadPos(startPos + size);
    reader.currentReadEndPos(readEndPos);
  }
};
}

namespace traits {
template<typename T>
struct ExtensionTraits<ext::Growable, T>
{
  using TValue = T;
  static constexpr bool SupportValueOverload = false;
  static constexpr bool SupportObjectOverload = true;
  static constexpr bool SupportLambdaOverload = true;
};
}

}

#endif // BITSERY_EXT_GROWABLE_H
