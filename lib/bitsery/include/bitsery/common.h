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

#ifndef BITSERY_COMMON_H
#define BITSERY_COMMON_H

namespace bitsery {

/*
 * endianness
 */
enum class EndiannessType
{
  LittleEndian,
  BigEndian
};

// default configuration for serialization and deserialization
struct DefaultConfig
{
  // defines endianness of data that is read from input adapter and written to
  // output adapter.
  static constexpr EndiannessType Endianness = EndiannessType::LittleEndian;
  // these flags allow to improve deserialization performance if data is trusted
  // enables/disables checks for buffer end or stream read errors in input
  // adapter
  static constexpr bool CheckAdapterErrors = true;
  // enables/disables checks for other errors that can significantly affect
  // performance
  static constexpr bool CheckDataErrors = true;
};

}

#endif // BITSERY_COMMON_H
