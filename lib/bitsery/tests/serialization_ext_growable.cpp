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
#include <bitsery/ext/growable.h>
#include <gmock/gmock.h>

using namespace testing;

using bitsery::ext::Growable;

struct DataV1
{
  int32_t v1;
};

template<typename S>
void
serialize(S& s, DataV1& o)
{
  s.value4b(o.v1);
}

struct DataV2
{
  int32_t v1;
  int32_t v2;
};

template<typename S>
void
serialize(S& s, DataV2& o)
{
  s.value4b(o.v1);
  s.value4b(o.v2);
}

struct DataV3
{
  int32_t v1;
  int32_t v2;
  int32_t v3;
  template<typename S>
  void serialize(S& s)
  {
    s.value4b(v1);
    s.value4b(v2);
    s.value4b(v3);
  }
};

TEST(SerializeExtensionGrowable,
     SessionsLengthIsStoredWith4BytesBeforeSessionDataStarts)
{
  SerializationContext ctx;
  auto& ser = ctx.createSerializer();
  // session cannot be empty
  ser.value2b(int16_t{ 1 });
  ser.ext(int8_t{ 2 }, Growable{}, [](decltype(ser)& ser, int8_t& v) {
    ser.value1b(v);
  });
  ser.value1b(int8_t{ 3 });

  auto& des = ctx.createDeserializer();
  uint8_t res1b{};
  uint16_t res2b{};
  uint32_t res4b{};
  des.value2b(res2b);
  EXPECT_THAT(res2b, Eq(1));
  des.value4b(res4b);
  EXPECT_THAT(res4b, Eq(1 + 4)); // size + 4bytes
  des.value1b(res1b);
  EXPECT_THAT(res1b, Eq(2));
  des.value1b(res1b);
  EXPECT_THAT(res1b, Eq(3));
  EXPECT_THAT(ctx.ser->adapter().writtenBytesCount(), Eq(8));
}

TEST(SerializeExtensionGrowable, MultipleSessionsReadSameVersionData)
{
  SerializationContext ctx;
  DataV2 data{ 8454, 987451 };
  auto& ser = ctx.createSerializer();

  for (auto i = 0; i < 10; ++i) {
    ser.ext(data, Growable{});
  }
  ctx.createDeserializer();
  DataV2 res{};
  auto& des = ctx.createDeserializer();
  for (auto i = 0; i < 10; ++i) {
    des.ext(res, Growable{});
    EXPECT_THAT(res.v1, Eq(data.v1));
    EXPECT_THAT(res.v2, Eq(data.v2));
  }
  EXPECT_THAT(ctx.des->adapter().isCompletedSuccessfully(), Eq(true));
}

TEST(SerializeExtensionGrowable, MultipleSessionsReadNewerVersionData)
{
  SerializationContext ctx;
  DataV3 data{ 8454, 987451, 45612 };
  auto& ser = ctx.createSerializer();

  for (auto i = 0; i < 10; ++i) {
    ser.ext(data, Growable{});
  }
  ctx.createDeserializer();
  DataV2 res{};
  auto& des = ctx.createDeserializer();
  for (auto i = 0; i < 10; ++i) {
    des.ext(res, Growable{});
    EXPECT_THAT(res.v1, Eq(data.v1));
    EXPECT_THAT(res.v2, Eq(data.v2));
  }
  EXPECT_THAT(ctx.des->adapter().isCompletedSuccessfully(), Eq(true));
}

TEST(SerializeExtensionGrowable, MultipleSessionsReadOlderVersionData)
{
  SerializationContext ctx;
  DataV2 data{ 8454, 987451 };
  auto& ser = ctx.createSerializer();

  for (auto i = 0; i < 10; ++i) {
    ser.ext(data, Growable{});
  }
  ctx.createDeserializer();
  DataV3 res{};
  auto& des = ctx.createDeserializer();
  for (auto i = 0; i < 10; ++i) {
    des.ext(res, Growable{});
    EXPECT_THAT(res.v1, Eq(data.v1));
    EXPECT_THAT(res.v2, Eq(data.v2));
    EXPECT_THAT(res.v3, Eq(0));
  }
  EXPECT_THAT(ctx.des->adapter().isCompletedSuccessfully(), Eq(true));
}

TEST(SerializeExtensionGrowable, MultipleNestedSessionsReadSameVersionData)
{
  SerializationContext ctx;
  DataV2 data{ 8454, 987451 };
  auto& ser = ctx.createSerializer();

  for (auto i = 0; i < 10; ++i) {
    ser.ext(data, Growable{}, [](decltype(ser)& ser, DataV2& o) {
      ser.value4b(o.v1);
      ser.value4b(o.v2);
      ser.ext(o, Growable{});
    });
  }
  ctx.createDeserializer();
  DataV2 res{};
  auto& des = ctx.createDeserializer();
  for (auto i = 0; i < 10; ++i) {
    des.ext(res, Growable{}, [&res, &data](decltype(des)& des, DataV2& o) {
      des.value4b(o.v1);
      des.value4b(o.v2);
      EXPECT_THAT(res.v1, Eq(data.v1));
      EXPECT_THAT(res.v2, Eq(data.v2));
      des.ext(o, Growable{});
      EXPECT_THAT(res.v1, Eq(data.v1));
      EXPECT_THAT(res.v2, Eq(data.v2));
    });
  }
  EXPECT_THAT(ctx.des->adapter().isCompletedSuccessfully(), Eq(true));
}

TEST(SerializeExtensionGrowable, MultipleNestedSessionsReadNewerVersionData)
{
  SerializationContext ctx;
  DataV3 data{ 8454, 987451, 54124 };
  auto& ser = ctx.createSerializer();

  for (auto i = 0; i < 10; ++i) {
    ser.ext(data, Growable{}, [](decltype(ser)& ser, DataV3& o) {
      ser.value4b(o.v1);
      ser.value4b(o.v2);
      ser.ext(o, Growable{});
      // new fields can only be added at the end
      ser.value4b(o.v3);
    });
  }
  ctx.createDeserializer();
  DataV2 res{};
  auto& des = ctx.createDeserializer();
  for (auto i = 0; i < 10; ++i) {
    des.ext(res, Growable{}, [&res, &data](decltype(des)& des, DataV2& o) {
      des.value4b(o.v1);
      des.value4b(o.v2);
      EXPECT_THAT(res.v1, Eq(data.v1));
      EXPECT_THAT(res.v2, Eq(data.v2));
      des.ext(o, Growable{});
      EXPECT_THAT(res.v1, Eq(data.v1));
      EXPECT_THAT(res.v2, Eq(data.v2));
    });
  }
  EXPECT_THAT(ctx.des->adapter().isCompletedSuccessfully(), Eq(true));
}

TEST(SerializeExtensionGrowable, MultipleNestedSessionsReadOlderVersionData)
{
  SerializationContext ctx;
  DataV2 data{ 8454, 987451 };
  auto& ser = ctx.createSerializer();

  for (auto i = 0; i < 10; ++i) {
    ser.ext(data, Growable{}, [](decltype(ser)& ser, DataV2& o) {
      ser.value4b(o.v1);
      ser.value4b(o.v2);
      ser.ext(o, Growable{});
    });
  }
  ctx.createDeserializer();
  DataV3 res{};
  auto& des = ctx.createDeserializer();
  for (auto i = 0; i < 10; ++i) {
    des.ext(res, Growable{}, [&res, &data](decltype(des)& des, DataV3& o) {
      des.value4b(o.v1);
      des.value4b(o.v2);
      EXPECT_THAT(res.v1, Eq(data.v1));
      EXPECT_THAT(res.v2, Eq(data.v2));
      des.ext(o, Growable{});
      EXPECT_THAT(res.v1, Eq(data.v1));
      EXPECT_THAT(res.v2, Eq(data.v2));
      EXPECT_THAT(res.v3, Eq(0));
      // new fields can only be added at the end
      des.value4b(o.v3);
      EXPECT_THAT(res.v3, Eq(0));
    });
  }
  EXPECT_THAT(ctx.des->adapter().isCompletedSuccessfully(), Eq(true));
}
