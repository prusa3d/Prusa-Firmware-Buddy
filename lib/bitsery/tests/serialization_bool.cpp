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

TEST(SerializeBooleans, BoolAsBit)
{

  SerializationContext ctx{};
  bool t1{ true };
  bool t2{ false };
  bool res1;
  bool res2;
  auto& ser = ctx.createSerializer();
  ser.enableBitPacking(
    [&t1, &t2](SerializationContext::TSerializerBPEnabled& sbp) {
      sbp.boolValue(t1);
      sbp.boolValue(t2);
    });
  auto& des = ctx.createDeserializer();
  des.enableBitPacking(
    [&res1, &res2](SerializationContext::TDeserializerBPEnabled& sbp) {
      sbp.boolValue(res1);
      sbp.boolValue(res2);
    });

  EXPECT_THAT(res1, Eq(t1));
  EXPECT_THAT(res2, Eq(t2));
  EXPECT_THAT(ctx.getBufferSize(), Eq(1));
}

TEST(SerializeBooleans, BoolAsByte)
{
  SerializationContext ctx;
  bool t1{ true };
  bool t2{ false };
  bool res1;
  bool res2;
  auto& ser = ctx.createSerializer();
  ser.boolValue(t1);
  ser.boolValue(t2);
  auto& des = ctx.createDeserializer();
  des.boolValue(res1);
  des.boolValue(res2);

  EXPECT_THAT(res1, Eq(t1));
  EXPECT_THAT(res2, Eq(t2));
  EXPECT_THAT(ctx.getBufferSize(), Eq(2));
}

TEST(SerializeBooleans,
     WhenReadingBoolByteReadsMoreThanOneThenInvalidDataErrorAndResultIsFalse)
{
  SerializationContext ctx;
  auto& ser = ctx.createSerializer();
  ser.value1b(uint8_t{ 1 });
  ser.value1b(uint8_t{ 2 });
  bool res{};
  auto& des = ctx.createDeserializer();
  des.boolValue(res);
  EXPECT_THAT(res, Eq(true));
  des.boolValue(res);
  EXPECT_THAT(res, Eq(false));
  EXPECT_THAT(ctx.des->adapter().error(),
              Eq(bitsery::ReaderError::InvalidData));
}
