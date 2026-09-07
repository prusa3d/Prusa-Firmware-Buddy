// MIT License
//
// Copyright (c) 2019 Mindaugas Vinkelis
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

#include <bitsery/ext/std_chrono.h>

#include "serialization_test_utils.h"
#include <gmock/gmock.h>

using StdDuration = bitsery::ext::StdDuration;
using StdTimePoint = bitsery::ext::StdTimePoint;

using testing::Eq;

TEST(SerializeExtensionStdChrono, IntegralDuration)
{
  SerializationContext ctx1;
  using Hours = std::chrono::duration<int32_t, std::ratio<60>>;

  Hours data{ 43 };
  Hours res{};

  ctx1.createSerializer().ext4b(data, StdDuration{});
  ctx1.createDeserializer().ext4b(res, StdDuration{});
  EXPECT_THAT(res, Eq(data));
}

TEST(SerializeExtensionStdChrono, IntegralTimePoint)
{
  SerializationContext ctx1;
  using Duration = std::chrono::duration<int64_t, std::milli>;
  using TP = std::chrono::time_point<std::chrono::system_clock, Duration>;

  TP data{ Duration{ 243 } };
  TP res{};

  ctx1.createSerializer().ext8b(data, StdTimePoint{});
  ctx1.createDeserializer().ext8b(res, StdTimePoint{});
  EXPECT_THAT(res, Eq(data));
}

TEST(SerializeExtensionStdChrono, FloatDuration)
{
  SerializationContext ctx1;
  using Hours = std::chrono::duration<float, std::ratio<60>>;

  Hours data{ 43.5f };
  Hours res{};

  ctx1.createSerializer().ext4b(data, StdDuration{});
  ctx1.createDeserializer().ext4b(res, StdDuration{});
  EXPECT_THAT(res, Eq(data));
}

TEST(SerializeExtensionStdChrono, FloatTimePoint)
{
  SerializationContext ctx1;
  using Duration = std::chrono::duration<double, std::milli>;
  using TP = std::chrono::time_point<std::chrono::system_clock, Duration>;

  TP data{ Duration{ 243457.4 } };
  TP res{};

  ctx1.createSerializer().ext8b(data, StdTimePoint{});
  ctx1.createDeserializer().ext8b(res, StdTimePoint{});
  EXPECT_THAT(res, Eq(data));
}
