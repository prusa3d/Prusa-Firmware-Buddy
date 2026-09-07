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

#if __cplusplus > 201402L

#include <bitsery/ext/std_optional.h>
#include <bitsery/ext/value_range.h>

using StdOptional = bitsery::ext::StdOptional;

using BPSer = SerializationContext::TSerializer::BPEnabledType;
using BPDes = SerializationContext::TDeserializer::BPEnabledType;

using testing::Eq;

template<typename T>
void
test(SerializationContext& ctx, const T& v, T& r)
{
  ctx.createSerializer().ext4b(v, StdOptional{});
  ctx.createDeserializer().ext4b(r, StdOptional{});
}

TEST(SerializeExtensionStdOptional, EmptyOptional)
{
  std::optional<int32_t> t1{};
  std::optional<int32_t> r1{};

  SerializationContext ctx1;
  test(ctx1, t1, r1);
  EXPECT_THAT(ctx1.getBufferSize(), Eq(1));
  EXPECT_THAT(t1, Eq(r1));

  r1 = 3;
  SerializationContext ctx2;
  test(ctx2, t1, r1);
  EXPECT_THAT(ctx2.getBufferSize(), Eq(1));
  EXPECT_THAT(t1, Eq(r1));
}

TEST(SerializeExtensionStdOptional, OptionalHasValue)
{
  std::optional<int32_t> t1{ 43 };
  std::optional<int32_t> r1{ 52 };

  SerializationContext ctx1;
  test(ctx1, t1, r1);
  EXPECT_THAT(ctx1.getBufferSize(), Eq(1 + sizeof(int)));
  EXPECT_THAT(t1.value(), Eq(r1.value()));

  r1 = std::optional<int>{};
  SerializationContext ctx2;
  test(ctx2, t1, r1);
  EXPECT_THAT(ctx2.getBufferSize(), Eq(1 + sizeof(int)));
  EXPECT_THAT(t1.value(), Eq(r1.value()));
}

TEST(SerializeExtensionStdOptional, AlignAfterStateWriteRead)
{
  std::optional<int32_t> t1{ 43 };
  std::optional<int32_t> r1{ 52 };
  auto range = bitsery::ext::ValueRange<int>{ 40, 60 };

  SerializationContext ctx;
  ctx.createSerializer().enableBitPacking([&t1, &range](BPSer& ser) {
    ser.ext(t1, StdOptional(true), [&range](BPSer& ser, int32_t& v) {
      ser.ext(v, range);
    });
  });
  ctx.createDeserializer().enableBitPacking([&r1, &range](BPDes& des) {
    des.ext(r1, StdOptional(true), [&range](BPDes& des, int32_t& v) {
      des.ext(v, range);
    });
  });

  EXPECT_THAT(ctx.getBufferSize(), Eq(2)); // 1byte for index + 1byte for value
  EXPECT_THAT(t1.value(), Eq(r1.value()));
}

TEST(SerializeExtensionStdOptional, NoAlignAfterStateWriteRead)
{
  std::optional<int32_t> t1{ 43 };
  std::optional<int32_t> r1{ 52 };
  auto range = bitsery::ext::ValueRange<int>{ 40, 60 };

  SerializationContext ctx;
  ctx.createSerializer().enableBitPacking([&t1, &range](BPSer& ser) {
    ser.ext(t1, StdOptional(false), [&range](BPSer& ser, int32_t& v) {
      ser.ext(v, range);
    });
  });
  ctx.createDeserializer().enableBitPacking([&r1, &range](BPDes& des) {
    des.ext(r1, StdOptional(false), [&range](BPDes& des, int32_t& v) {
      des.ext(v, range);
    });
  });

  EXPECT_THAT(range.getRequiredBits() + 1, ::testing::Lt(8));
  EXPECT_THAT(ctx.getBufferSize(), Eq(1));
  EXPECT_THAT(t1.value(), Eq(r1.value()));
}

#elif defined(_MSC_VER)
#pragma message(                                                               \
    "C++17 and /Zc:__cplusplus option is required to enable std::optional tests")
#else
#pragma message("C++17 is required to enable std::optional tests")
#endif
