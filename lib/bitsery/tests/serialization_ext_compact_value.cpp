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
#include <bitsery/ext/compact_value.h>
#include <gmock/gmock.h>

#include <bitsery/traits/array.h>
#include <bitset>
#include <chrono>

using bitsery::EndiannessType;
using bitsery::ext::CompactValue;
using bitsery::ext::CompactValueAsObject;
using testing::Eq;

// helper function, that gets value filled with specified number of bits
template<typename TValue>
TValue
getValue(bool isPositive, size_t significantBits)
{
  TValue v = isPositive ? 0 : static_cast<TValue>(-1);
  if (significantBits == 0)
    return v;

  using TUnsigned = typename std::make_unsigned<TValue>::type;
  TUnsigned mask = {};
  mask = static_cast<TUnsigned>(~mask); // invert shiftByBits
  auto shiftBy = bitsery::details::BitsSize<TValue>::value - significantBits;
  mask = static_cast<TUnsigned>(mask >> shiftBy);
  // cast to unsigned when applying mask
  return v ^ static_cast<TValue>(mask);
}

// helper function, that serialize and return deserialized value
template<typename TConfig, typename TValue>
std::pair<TValue, size_t>
serializeAndGetDeserialized(TValue data)
{
  Buffer buf{};
  bitsery::Serializer<bitsery::OutputBufferAdapter<Buffer, TConfig>> ser{ buf };
  ser.template ext<sizeof(TValue)>(data, CompactValue{});

  bitsery::Deserializer<bitsery::InputBufferAdapter<Buffer, TConfig>> des{
    buf.begin(), ser.adapter().writtenBytesCount()
  };
  TValue res;
  des.template ext<sizeof(TValue)>(res, CompactValue{});
  return { res, ser.adapter().writtenBytesCount() };
}

struct LittleEndianConfig
{
  static constexpr EndiannessType Endianness = EndiannessType::LittleEndian;
  static constexpr bool CheckDataErrors = true;
  static constexpr bool CheckAdapterErrors = true;
};

struct BigEndianConfig
{
  static constexpr EndiannessType Endianness = EndiannessType::BigEndian;
  static constexpr bool CheckDataErrors = true;
  static constexpr bool CheckAdapterErrors = true;
};

template<typename TValue, bool isPositiveNr, typename TConfig>
struct TC
{
  static_assert(isPositiveNr || std::is_signed<TValue>::value, "");

  using Value = TValue;
  using Config = TConfig;
  bool isPositive = isPositiveNr;
};

template<typename T>
class SerializeExtensionCompactValueCorrectness : public testing::Test
{
public:
  using TestCase = T;
};

using AllValueSizesTestCases =
  ::testing::Types<TC<uint8_t, true, LittleEndianConfig>,
                   TC<uint16_t, true, LittleEndianConfig>,
                   TC<uint32_t, true, LittleEndianConfig>,
                   TC<uint64_t, true, LittleEndianConfig>,
                   TC<int8_t, true, LittleEndianConfig>,
                   TC<int16_t, true, LittleEndianConfig>,
                   TC<int32_t, true, LittleEndianConfig>,
                   TC<int64_t, true, LittleEndianConfig>,
                   TC<int8_t, false, LittleEndianConfig>,
                   TC<int16_t, false, LittleEndianConfig>,
                   TC<int32_t, false, LittleEndianConfig>,
                   TC<int64_t, false, LittleEndianConfig>,
                   TC<uint8_t, true, BigEndianConfig>,
                   TC<uint16_t, true, BigEndianConfig>,
                   TC<uint32_t, true, BigEndianConfig>,
                   TC<uint64_t, true, BigEndianConfig>,
                   TC<int8_t, true, BigEndianConfig>,
                   TC<int16_t, true, BigEndianConfig>,
                   TC<int32_t, true, BigEndianConfig>,
                   TC<int64_t, true, BigEndianConfig>,
                   TC<int8_t, false, BigEndianConfig>,
                   TC<int16_t, false, BigEndianConfig>,
                   TC<int32_t, false, BigEndianConfig>,
                   TC<int64_t, false, BigEndianConfig>>;

TYPED_TEST_SUITE(SerializeExtensionCompactValueCorrectness,
                 AllValueSizesTestCases, );

TYPED_TEST(SerializeExtensionCompactValueCorrectness, TestDifferentSizeValues)
{
  using TCase = typename TestFixture::TestCase;
  using TValue = typename TCase::Value;
  TCase tc{};

  for (auto i = 0u; i < bitsery::details::BitsSize<TValue>::value + 1; ++i) {
    auto data = getValue<TValue>(tc.isPositive, i);
    auto res = serializeAndGetDeserialized<typename TCase::Config>(data);
    EXPECT_THAT(res.first, Eq(data));
  }
}

// this stucture will contain test data and result, as type paramters
template<typename TValue,
         bool isPositiveNr,
         size_t significantBits,
         size_t resultBytes>
struct SizeTC
{
  static_assert(isPositiveNr || std::is_signed<TValue>::value, "");
  static_assert(bitsery::details::BitsSize<TValue>::value >= significantBits,
                "");

  using Value = TValue;
  bool isPositive = isPositiveNr;
  size_t fillBits = significantBits;
  size_t bytesCount = resultBytes;
};

template<typename T>
class SerializeExtensionCompactValueRequiredBytes : public testing::Test
{
public:
  using TestCase = T;
};

using RequiredBytesTestCases = ::testing::Types<
  // 1 byte always writes to 1 byte
  SizeTC<uint8_t, true, 0, 1>,
  SizeTC<uint8_t, true, 8, 1>,
  SizeTC<int8_t, false, 0, 1>,
  SizeTC<int8_t, true, 8, 1>,

  // 2 byte, +1 byte after 15 significant bits
  SizeTC<uint16_t, true, 7, 1>,
  SizeTC<uint16_t, true, 8, 2>,
  SizeTC<uint16_t, true, 14, 2>,
  SizeTC<uint16_t, true, 15, 3>,
  // 2 byte, +1 byte after 15-1 significant bits (1 bit for sign)
  SizeTC<int16_t, true, 6, 1>,
  SizeTC<int16_t, false, 7, 2>,
  SizeTC<int16_t, true, 13, 2>,
  SizeTC<int16_t, false, 14, 3>,

  // 4 byte, +1 byte after 29 significant bits
  SizeTC<uint32_t, true, 14, 2>,
  SizeTC<uint32_t, true, 21, 3>,
  SizeTC<uint32_t, true, 28, 4>,
  SizeTC<uint32_t, true, 29, 5>,
  SizeTC<uint32_t, true, 32, 5>,
  // 4 byte
  SizeTC<int32_t, true, 13, 2>,
  SizeTC<int32_t, false, 20, 3>,
  SizeTC<int32_t, true, 27, 4>,
  SizeTC<int32_t, false, 28, 5>,
  SizeTC<int32_t, true, 31, 5>,

  // 8 byte, +1 byte after 57 significant bits, or +2 byte when all bits are
  // significant
  SizeTC<uint64_t, true, 28, 4>,
  SizeTC<uint64_t, true, 35, 5>,
  SizeTC<uint64_t, true, 42, 6>,
  SizeTC<uint64_t, true, 49, 7>,
  SizeTC<uint64_t, true, 56, 8>,
  SizeTC<uint64_t, true, 57, 9>,
  SizeTC<uint64_t, true, 63, 9>,
  SizeTC<uint64_t, true, 64, 10>,
  // 8 byte,
  SizeTC<int64_t, true, 27, 4>,
  SizeTC<int64_t, false, 34, 5>,
  SizeTC<int64_t, true, 41, 6>,
  SizeTC<int64_t, false, 48, 7>,
  SizeTC<int64_t, true, 55, 8>,
  SizeTC<int64_t, false, 56, 9>,
  SizeTC<int64_t, true, 62, 9>,
  SizeTC<int64_t, false, 63, 10>>;

TYPED_TEST_SUITE(SerializeExtensionCompactValueRequiredBytes,
                 RequiredBytesTestCases, );

TYPED_TEST(SerializeExtensionCompactValueRequiredBytes, Test)
{
  using TCase = typename TestFixture::TestCase;
  using TValue = typename TCase::Value;
  TCase tc{};
  TValue data = getValue<TValue>(tc.isPositive, tc.fillBits);
  auto res = serializeAndGetDeserialized<bitsery::DefaultConfig>(data);
  EXPECT_THAT(res.first, Eq(data));
  EXPECT_THAT(res.second, tc.bytesCount);
}

enum b1En : uint8_t
{
  A,
  B,
  C,
  D = 54,
  E
};

enum class b8En : int64_t
{
  A = -874987489,
  B,
  C = 0,
  D,
  E = 489748978,
  F,
  G
};

TEST(SerializeExtensionCompactValueEnum, TestEnums)
{
  auto d1 = b1En::E;
  auto d2 = b8En::B;
  auto d3 = b8En::F;
  EXPECT_THAT(serializeAndGetDeserialized<bitsery::DefaultConfig>(d1).first,
              Eq(d1));
  EXPECT_THAT(serializeAndGetDeserialized<bitsery::DefaultConfig>(d2).first,
              Eq(d2));
  EXPECT_THAT(serializeAndGetDeserialized<bitsery::DefaultConfig>(d3).first,
              Eq(d3));
}

TEST(SerializeExtensionCompactValueAsObjectDeserializeOverflow, TestEnums)
{
  SerializationContext ctx;
  auto data = getValue<uint32_t>(true, 17);
  uint16_t res{};
  ctx.createSerializer().ext(data, CompactValueAsObject{});
  ctx.createDeserializer().ext(res, CompactValueAsObject{});
  EXPECT_THAT(data, ::testing::Ne(res));
  EXPECT_THAT(ctx.des->adapter().error(),
              Eq(bitsery::ReaderError::InvalidData));
}
