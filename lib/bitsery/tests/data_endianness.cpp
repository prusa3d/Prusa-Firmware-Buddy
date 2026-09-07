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

#include "serialization_test_utils.h"
#include <bitsery/deserializer.h>
#include <bitsery/ext/value_range.h>
#include <bitsery/serializer.h>
#include <gmock/gmock.h>

using bitsery::DefaultConfig;
using bitsery::EndiannessType;
using testing::ContainerEq;
using testing::Eq;

constexpr EndiannessType
getInverseEndianness(EndiannessType e)
{
  return e == EndiannessType::LittleEndian ? EndiannessType::BigEndian
                                           : EndiannessType::LittleEndian;
}

struct InverseEndiannessConfig
{
  static constexpr bitsery::EndiannessType Endianness =
    getInverseEndianness(DefaultConfig::Endianness);
  static constexpr bool CheckDataErrors = true;
  static constexpr bool CheckAdapterErrors = true;
};

struct IntegralTypes
{
  int64_t a;
  uint32_t b;
  int16_t c;
  uint8_t d;
  int8_t e;
};

using InverseReader =
  bitsery::InputBufferAdapter<Buffer, InverseEndiannessConfig>;

TEST(DataEndianness, WhenWriteBytesThenBytesAreSwapped)
{
  // fill initial values
  IntegralTypes src{};
  src.a = static_cast<int64_t>(0x1122334455667788u);
  src.b = 0xBBCCDDEEu;
  src.c = static_cast<int16_t>(0xCCDDu);
  src.d = static_cast<uint8_t>(0xDDu);
  src.e = static_cast<int8_t>(0xEEu);

  // fill expected result after swap
  IntegralTypes resInv{};
  resInv.a = static_cast<int64_t>(0x8877665544332211u);
  resInv.b = 0xEEDDCCBBu;
  resInv.c = static_cast<int16_t>(0xDDCCu);
  resInv.d = static_cast<uint8_t>(0xDDu);
  resInv.e = static_cast<int8_t>(0xEEu);

  // create and write to buffer
  Buffer buf{};
  Writer bw{ buf };
  bw.writeBytes<8>(src.a);
  bw.writeBytes<4>(src.b);
  bw.writeBytes<2>(src.c);
  bw.writeBytes<1>(src.d);
  bw.writeBytes<1>(src.e);
  bw.flush();
  // read from buffer using inverse endianness config
  InverseReader br{ buf.begin(), bw.writtenBytesCount() };
  IntegralTypes res{};
  br.readBytes<8>(res.a);
  br.readBytes<4>(res.b);
  br.readBytes<2>(res.c);
  br.readBytes<1>(res.d);
  br.readBytes<1>(res.e);
  // check results
  EXPECT_THAT(res.a, Eq(resInv.a));
  EXPECT_THAT(res.b, Eq(resInv.b));
  EXPECT_THAT(res.c, Eq(resInv.c));
  EXPECT_THAT(res.d, Eq(resInv.d));
  EXPECT_THAT(res.e, Eq(resInv.e));
}

TEST(DataEndianness, WhenWrite1ByteValuesThenEndiannessIsIgnored)
{
  // fill initial values
  constexpr size_t SIZE = 4;
  uint8_t src[SIZE] = { 0xAA, 0xBB, 0xCC, 0xDD };
  uint8_t res[SIZE] = {};
  // create and write to buffer
  Buffer buf{};
  Writer bw{ buf };
  bw.writeBuffer<1>(src, SIZE);
  bw.flush();
  // read from buffer using inverse endianness config
  InverseReader br{ buf.begin(), bw.writtenBytesCount() };
  br.readBuffer<1>(res, SIZE);
  // result is identical, because we write separate values, of size 1byte, that
  // requires no swapping check results
  EXPECT_THAT(res, ContainerEq(src));
}

TEST(DataEndianness, WhenWriteMoreThan1ByteValuesThenValuesAreSwapped)
{
  // fill initial values
  constexpr size_t SIZE = 4;
  uint16_t src[SIZE] = { 0xAA00, 0xBB11, 0xCC22, 0xDD33 };
  uint16_t resInv[SIZE] = { 0x00AA, 0x11BB, 0x22CC, 0x33DD };
  uint16_t res[SIZE] = {};
  // create and write to buffer
  Buffer buf{};
  Writer bw{ buf };
  bw.writeBuffer<2>(src, SIZE);
  bw.flush();
  // read from buffer using inverse endianness config
  InverseReader br{ buf.begin(), bw.writtenBytesCount() };
  br.readBuffer<2>(res, SIZE);
  // result is identical, because we write separate values, of size 1byte, that
  // requires no swapping check results
  EXPECT_THAT(res, ContainerEq(resInv));
}

template<typename T>
constexpr size_t
getBits(T v)
{
  return bitsery::details::calcRequiredBits<T>({}, v);
}

struct IntegralUnsignedTypes
{
  uint64_t a;
  uint32_t b;
  uint16_t c;
  uint8_t d;
};

TEST(DataEndianness,
     WhenValueTypeIs1ByteThenBitOperationsIsNotAffectedByEndianness)
{
  // fill initial values
  constexpr IntegralUnsignedTypes src{
    0x0000334455667788,
    0x00CCDDEE,
    0x00DD,
    0x0F,
  };

  constexpr size_t aBITS = getBits(src.a) + 8;
  constexpr size_t bBITS = getBits(src.b) + 0;
  constexpr size_t cBITS = getBits(src.c) + 5;
  constexpr size_t dBITS = getBits(src.d) + 2;
  // create and write to buffer
  Buffer buf{};
  Writer bw{ buf };
  bitsery::details::OutputAdapterBitPackingWrapper<Writer> bpw{ bw };
  bpw.writeBits(src.a, aBITS);
  bpw.writeBits(src.b, bBITS);
  bpw.writeBits(src.c, cBITS);
  bpw.writeBits(src.d, dBITS);
  bpw.flush();
  // read from buffer using inverse endianness config
  InverseReader br{ buf.begin(), bpw.writtenBytesCount() };
  bitsery::details::InputAdapterBitPackingWrapper<InverseReader> bpr{ br };
  IntegralUnsignedTypes res{};
  bpr.readBits(res.a, aBITS);
  bpr.readBits(res.b, bBITS);
  bpr.readBits(res.c, cBITS);
  bpr.readBits(res.d, dBITS);
  // check results
  EXPECT_THAT(res.a, Eq(src.a));
  EXPECT_THAT(res.b, Eq(src.b));
  EXPECT_THAT(res.c, Eq(src.c));
  EXPECT_THAT(res.d, Eq(src.d));
}
