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

#include <bitsery/ext/entropy.h>
#include <bitsery/traits/list.h>

#include "serialization_test_utils.h"
#include <gmock/gmock.h>

using namespace testing;

using bitsery::ext::Entropy;

using BPSer = SerializationContext::TSerializerBPEnabled;
using BPDes = SerializationContext::TDeserializerBPEnabled;

TEST(SerializeExtensionEntropy,
     WhenEntropyEncodedThenOnlyWriteIndexUsingMinRequiredBits)
{
  int32_t v = 4849;
  int32_t res;
  int32_t values[3] = { 485, 4849, 89 };
  SerializationContext ctx{};
  ctx.createSerializer().enableBitPacking(
    [&v, &values](BPSer& ser) { ser.ext4b(v, Entropy<int32_t[3]>{ values }); });
  ctx.createDeserializer().enableBitPacking([&res, &values](BPDes& des) {
    des.ext4b(res, Entropy<int32_t[3]>{ values });
  });

  EXPECT_THAT(res, Eq(v));
  EXPECT_THAT(ctx.getBufferSize(), Eq(1));

  SerializationContext ctx1{};
  ctx1.createSerializer().enableBitPacking(
    [&v, &values](BPSer& ser) { ser.ext4b(v, Entropy<int32_t[3]>{ values }); });
  ctx1.createDeserializer().enableBitPacking([&res](BPDes& des) {
    des.ext(
      res, bitsery::ext::ValueRange<int32_t>{ 0, static_cast<int32_t>(3 + 1) });
  });
  EXPECT_THAT(res, Eq(2));
}

TEST(SerializeExtensionEntropy,
     WhenNoEntropyEncodedThenWriteZeroBitsAndValueOrObject)
{
  int16_t v = 8945;
  int16_t res;
  std::initializer_list<int> values{ 485, 4849, 89 };
  SerializationContext ctx{};
  ctx.createSerializer().enableBitPacking([&v, &values](BPSer& ser) {
    ser.ext2b(v, Entropy<std::initializer_list<int>>{ values });
  });
  ctx.createDeserializer().enableBitPacking([&res, &values](BPDes& des) {
    des.ext2b(res, Entropy<std::initializer_list<int>>{ values });
  });

  EXPECT_THAT(res, Eq(v));
  EXPECT_THAT(ctx.getBufferSize(), Eq(sizeof(int16_t) + 1));
}

TEST(SerializeExtensionEntropy, CustomTypeEntropyEncoded)
{
  MyStruct1 v = { 12, 10 };
  MyStruct1 res;
  constexpr size_t N = 4;

  MyStruct1 values[N]{ MyStruct1{ 12, 10 },
                       MyStruct1{ 485, 454 },
                       MyStruct1{ 4849, 89 },
                       MyStruct1{ 0, 1 } };
  SerializationContext ctx{};
  ctx.createSerializer().enableBitPacking(
    [&v, &values](BPSer& ser) { ser.ext(v, Entropy<MyStruct1[4]>{ values }); });
  ctx.createDeserializer().enableBitPacking([&res, &values](BPDes& des) {
    des.ext(res, Entropy<MyStruct1[4]>{ values });
  });
  EXPECT_THAT(res, Eq(v));
  EXPECT_THAT(ctx.getBufferSize(), Eq(1));
}

TEST(SerializeExtensionEntropy, CustomTypeNotEntropyEncoded)
{
  MyStruct1 v = { 8945, 4456 };
  MyStruct1 res;

  std::initializer_list<MyStruct1> values{ MyStruct1{ 12, 10 },
                                           MyStruct1{ 485, 454 },
                                           MyStruct1{ 4849, 89 },
                                           MyStruct1{ 0, 1 } };
  SerializationContext ctx{};
  ctx.createSerializer().enableBitPacking([&v, &values](BPSer& ser) {
    ser.ext(v, Entropy<std::initializer_list<MyStruct1>>{ values });
  });
  ctx.createDeserializer().enableBitPacking([&res, &values](BPDes& des) {
    des.ext(res, Entropy<std::initializer_list<MyStruct1>>{ values });
  });

  EXPECT_THAT(res, Eq(v));
  EXPECT_THAT(ctx.getBufferSize(), Eq(MyStruct1::SIZE + 1));
}

TEST(SerializeExtensionEntropy,
     CustomFunctionNotEntropyEncodedWithNoAlignBeforeData)
{
  MyStruct1 v = { 8945, 4456 };
  MyStruct1 res;
  constexpr size_t N = 4;

  std::vector<MyStruct1> values{ MyStruct1{ 12, 10 },
                                 MyStruct1{ 485, 454 },
                                 MyStruct1{ 4849, 89 },
                                 MyStruct1{ 0, 1 } };

  auto rangeForValue = bitsery::ext::ValueRange<int>{ 0, 10000 };

  SerializationContext ctx;
  ctx.createSerializer().enableBitPacking(
    [&v, &values, &rangeForValue](BPSer& ser) {
      // lambdas differ only in capture clauses, it would make sense to use
      // std::bind, but debugger crashes when it sees std::bind...
      auto serLambda = [&rangeForValue](BPSer& ser, MyStruct1& data) {
        ser.ext(data.i1, rangeForValue);
        ser.ext(data.i2, rangeForValue);
      };
      ser.ext(v, Entropy<std::vector<MyStruct1>>(values, false), serLambda);
    });

  ctx.createDeserializer().enableBitPacking(
    [&res, &values, &rangeForValue](BPDes& des) {
      auto desLambda = [&rangeForValue](BPDes& des, MyStruct1& data) {
        des.ext(data.i1, rangeForValue);
        des.ext(data.i2, rangeForValue);
      };
      des.ext(res, Entropy<std::vector<MyStruct1>>(values, false), desLambda);
    });

  EXPECT_THAT(res, Eq(v));
  auto rangeForIndex = bitsery::ext::ValueRange<size_t>{ 0u, N + 1 };
  EXPECT_THAT(ctx.getBufferSize(),
              Eq((rangeForIndex.getRequiredBits() +
                  rangeForValue.getRequiredBits() * 2 - 1) /
                   8 +
                 1));
}

TEST(SerializeExtensionEntropy,
     CustomFunctionNotEntropyEncodedWithAlignBeforeData)
{
  MyStruct1 v = { 8945, 4456 };
  MyStruct1 res;

  std::vector<MyStruct1> values{ MyStruct1{ 12, 10 },
                                 MyStruct1{ 485, 454 },
                                 MyStruct1{ 4849, 89 },
                                 MyStruct1{ 0, 1 } };

  auto rangeForValue = bitsery::ext::ValueRange<int>{ 0, 10000 };

  SerializationContext ctx;
  ctx.createSerializer().enableBitPacking(
    [&v, &values, &rangeForValue](BPSer& ser) {
      // lambdas differ only in capture clauses, it would make sense to use
      // std::bind, but debugger crashes when it sees std::bind...
      auto serLambda = [&rangeForValue](BPSer& ser, MyStruct1& data) {
        ser.ext(data.i1, rangeForValue);
        ser.ext(data.i2, rangeForValue);
      };
      ser.ext(v, Entropy<std::vector<MyStruct1>>(values, true), serLambda);
    });
  ctx.createDeserializer().enableBitPacking(
    [&res, &values, &rangeForValue](BPDes& des) {
      auto desLambda = [&rangeForValue](BPDes& des, MyStruct1& data) {
        des.ext(data.i1, rangeForValue);
        des.ext(data.i2, rangeForValue);
      };
      des.ext(res, Entropy<std::vector<MyStruct1>>(values, true), desLambda);
    });

  EXPECT_THAT(res, Eq(v));
  auto bitsForIndex = 8u; // because aligned
  EXPECT_THAT(
    ctx.getBufferSize(),
    Eq((bitsForIndex + rangeForValue.getRequiredBits() * 2 - 1) / 8 + 1));
}

TEST(SerializeExtensionEntropy, WhenEntropyEncodedThenCustomFunctionNotInvoked)
{
  MyStruct1 v = { 4849, 89 };
  MyStruct1 res;

  std::list<MyStruct1> values{ MyStruct1{ 12, 10 },
                               MyStruct1{ 485, 454 },
                               MyStruct1{ 4849, 89 },
                               MyStruct1{ 0, 1 } };

  SerializationContext ctx;
  ctx.createSerializer().enableBitPacking([&v, &values](BPSer& ser) {
    ser.ext(
      v, Entropy<std::list<MyStruct1>>{ values }, [](BPSer&, MyStruct1&) {});
  });
  ctx.createDeserializer().enableBitPacking([&res, &values](BPDes& des) {
    des.ext(
      res, Entropy<std::list<MyStruct1>>{ values }, [](BPDes&, MyStruct1&) {});
  });

  EXPECT_THAT(res, Eq(v));
  EXPECT_THAT(ctx.getBufferSize(), Eq(1));
}
