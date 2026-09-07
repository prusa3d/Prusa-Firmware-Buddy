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

#include "serialization_test_utils.h"
#include <gmock/gmock.h>

using testing::Eq;

#if __cplusplus > 201402L

#include <bitsery/ext/std_tuple.h>

template<typename T, size_t N>
using OverloadValue = bitsery::ext::OverloadValue<T, N>;

TEST(SerializeExtensionStdTuple, UseDefaultSerializeFunction)
{
  std::tuple<MyStruct1, MyStruct2> t1{
    MyStruct1{ -789, 45 }, MyStruct2{ MyStruct2::MyEnum::V3, MyStruct1{} }
  };
  std::tuple<MyStruct1, MyStruct2> r1{};
  SerializationContext ctx;
  ctx.createSerializer().ext(t1, bitsery::ext::StdTuple{});
  ctx.createDeserializer().ext(r1, bitsery::ext::StdTuple{});
  EXPECT_THAT(t1, Eq(r1));
}

TEST(SerializeExtensionStdTuple,
     ValueTypesCanBeSerializedWithLambdaAndOrCallableObject)
{
  std::tuple<float, int32_t> t1{ 123.456f, -898754656 };
  std::tuple<float, int32_t> r1{};
  SerializationContext ctx;
  auto exec = [](auto& s, auto& o) {
    s.ext(o,
          bitsery::ext::StdTuple{ [](auto& s1, float& o1) { s1.value4b(o1); },
                                  OverloadValue<int32_t, 4>{} });
  };
  ctx.createSerializer().object(t1, exec);
  ctx.createDeserializer().object(r1, exec);
  EXPECT_THAT(t1, Eq(r1));
}

TEST(SerializeExtensionStdTuple, CanOverloadDefaultSerializeFunction)
{
  std::tuple<MyStruct1, MyStruct2> t1{
    MyStruct1{ -789, 45 }, MyStruct2{ MyStruct2::MyEnum::V3, MyStruct1{} }
  };
  std::tuple<MyStruct1, MyStruct2> r1{};
  SerializationContext ctx;
  auto exec = [](auto& s, auto& o) {
    s.ext(o,
          bitsery::ext::StdTuple{
            [](auto& s1, MyStruct1& o1) {
              s1.value4b(o1.i1);
              // do not serialize other element, it should be 0 (default)
            },
          });
  };
  ctx.createSerializer().object(t1, exec);
  ctx.createDeserializer().object(r1, exec);
  EXPECT_THAT(std::get<1>(t1), Eq(std::get<1>(r1)));
  EXPECT_THAT(std::get<0>(t1).i1, Eq(std::get<0>(r1).i1));
  EXPECT_THAT(std::get<0>(t1).i2, ::testing::Ne(std::get<0>(r1).i2));
}

TEST(SerializeExtensionStdTuple, EmptyTuple)
{
  std::tuple<> t1{};
  std::tuple<> r1{};
  SerializationContext ctx;
  ctx.createSerializer().ext(t1, bitsery::ext::StdTuple{});
  ctx.createDeserializer().ext(r1, bitsery::ext::StdTuple{});
  EXPECT_THAT(t1, Eq(r1));
}

struct NonDefaultConstructable
{
  explicit NonDefaultConstructable(float x)
    : _x{ x }
  {
  }

  float _x;

  bool operator==(const NonDefaultConstructable& rhs) const
  {
    return _x == rhs._x;
  }

private:
  friend class bitsery::Access;

  NonDefaultConstructable()
    : _x{ 0.0f } {};
};

TEST(SerializeExtensionStdTuple, NonDefaultConstructable)
{
  std::tuple<NonDefaultConstructable> t1{ 34.0f };
  std::tuple<NonDefaultConstructable> r1{ 8.0f };
  SerializationContext ctx;
  ctx.createSerializer().ext(
    t1,
    bitsery::ext::StdTuple{
      [](auto& s, NonDefaultConstructable& v) { s.value4b(v._x); },
    });
  ctx.createDeserializer().ext(
    r1,
    bitsery::ext::StdTuple{
      [](auto& s, NonDefaultConstructable& v) { s.value4b(v._x); },
    });
  EXPECT_THAT(t1, Eq(r1));
}

#elif defined(_MSC_VER)
#pragma message(                                                               \
    "C++17 and /Zc:__cplusplus option is required to enable std::tuple tests")
#else
#pragma message("C++17 is required to enable std::tuple tests")
#endif
