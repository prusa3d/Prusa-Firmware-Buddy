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
#include <gmock/gmock.h>

using testing::Eq;

template<typename T>
bool
SerializeDeserializeValue(const T& v)
{
  T res{};
  SerializationContext ctx;
  ctx.createSerializer().value<sizeof(T)>(v);
  ctx.createDeserializer().value<sizeof(T)>(res);
  return v == res;
}

TEST(SerializeValues, IntegerTypes)
{
  EXPECT_THAT(SerializeDeserializeValue(-449874), Eq(true));
  EXPECT_THAT(SerializeDeserializeValue(34u), Eq(true));
}

TEST(SerializeValues, EnumTypes)
{
  enum E1
  {
    A1,
    B1,
    C1,
    D1
  };
  EXPECT_THAT(SerializeDeserializeValue(E1::C1), Eq(true));
  enum class E2
  {
    A2,
    B2,
    C2,
    D2
  };
  EXPECT_THAT(SerializeDeserializeValue(E2::B2), Eq(true));
  enum class E3 : short
  {
    A3,
    B3,
    C3 = 4568,
    D3
  };
  EXPECT_THAT(SerializeDeserializeValue(E3::C3), Eq(true));
}

TEST(SerializeValues, FloatingPointTypes)
{
  EXPECT_THAT(SerializeDeserializeValue(-484.465), Eq(true));
  EXPECT_THAT(SerializeDeserializeValue(0.00000015f), Eq(true));
}

TEST(SerializeValues, ValueSizeOverload1Byte)
{
  int8_t v{ 54 };
  int8_t res;
  constexpr size_t TSIZE = sizeof(v);

  SerializationContext ctx;
  ctx.createSerializer().value1b(v);
  ctx.createDeserializer().value1b(res);

  EXPECT_THAT(res, Eq(v));
  EXPECT_THAT(TSIZE, Eq(ctx.getBufferSize()));
}

TEST(SerializeValues, ValueSizeOverload2Byte)
{
  int16_t v{ 54 };
  int16_t res;
  constexpr size_t TSIZE = sizeof(v);

  SerializationContext ctx;
  ctx.createSerializer().value2b(v);
  ctx.createDeserializer().value2b(res);

  EXPECT_THAT(res, Eq(v));
  EXPECT_THAT(TSIZE, Eq(ctx.getBufferSize()));
}

TEST(SerializeValues, ValueSizeOverload4Byte)
{
  float v{ 54.498f };
  float res;
  constexpr size_t TSIZE = sizeof(v);

  SerializationContext ctx;
  ctx.createSerializer().value4b(v);
  ctx.createDeserializer().value4b(res);

  EXPECT_THAT(res, Eq(v));
  EXPECT_THAT(TSIZE, Eq(ctx.getBufferSize()));
}

TEST(SerializeValues, ValueSizeOverload8Byte)
{
  int64_t v{ 54 };
  int64_t res;
  constexpr size_t TSIZE = sizeof(v);

  SerializationContext ctx;
  ctx.createSerializer().value8b(v);
  ctx.createDeserializer().value8b(res);

  EXPECT_THAT(res, Eq(v));
  EXPECT_THAT(TSIZE, Eq(ctx.getBufferSize()));
}
