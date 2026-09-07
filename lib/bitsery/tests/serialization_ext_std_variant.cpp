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

#if __cplusplus > 201402L

using testing::Eq;

#include <bitsery/ext/std_variant.h>

template<typename T, size_t N>
using OverloadValue = bitsery::ext::OverloadValue<T, N>;

TEST(SerializeExtensionStdVariant, UseSerializeFunction)
{

  std::variant<MyStruct1, MyStruct2> t1{ MyStruct1{ 978, 15 } };
  std::variant<MyStruct1, MyStruct2> r1{ MyStruct2{} };
  SerializationContext ctx;
  ctx.createSerializer().ext(t1, bitsery::ext::StdVariant{});
  ctx.createDeserializer().ext(r1, bitsery::ext::StdVariant{});
  EXPECT_THAT(t1, Eq(r1));
}

TEST(SerializeExtensionStdVariant,
     WhenTwoIndicesWithSameTypeThenDeserializeCorrectIndex)
{

  std::variant<MyStruct1, MyStruct2, MyStruct1> t1{ std::in_place_index_t<2>{},
                                                    MyStruct1{ 978, 15 } };
  std::variant<MyStruct1, MyStruct2, MyStruct1> r1{ MyStruct2{} };
  SerializationContext ctx;
  ctx.createSerializer().ext(t1, bitsery::ext::StdVariant{});
  ctx.createDeserializer().ext(r1, bitsery::ext::StdVariant{});
  EXPECT_THAT(t1, Eq(r1));
}

TEST(SerializeExtensionStdVariant, ValueTypesCanBeSerializedWithLambda)
{

  std::variant<float, char, MyStruct1> t1{ 5.6f };
  std::variant<float, char, MyStruct1> r1{ MyStruct1{} };
  SerializationContext ctx;
  auto fncFloat = [](auto& s, float& v) { s.value4b(v); };
  auto fncChar = [](auto& s, char& v) { s.value1b(v); };
  ctx.createSerializer().ext(t1, bitsery::ext::StdVariant{ fncFloat, fncChar });
  ctx.createDeserializer().ext(r1,
                               bitsery::ext::StdVariant{ fncFloat, fncChar });
  EXPECT_THAT(t1, Eq(r1));
}

TEST(SerializeExtensionStdVariant,
     ValueTypesCanBeSerializedWithLambdaAndOrCallableObject)
{
  std::variant<float, char, MyStruct1> t1{ 'Z' };
  std::variant<float, char, MyStruct1> r1{ MyStruct1{} };
  SerializationContext ctx;
  auto fncFloat = [](auto& s, float& v) { s.value4b(v); };

  ctx.createSerializer().ext(
    t1, bitsery::ext::StdVariant{ fncFloat, OverloadValue<char, 1>{} });
  ctx.createDeserializer().ext(
    r1, bitsery::ext::StdVariant{ fncFloat, OverloadValue<char, 1>{} });
  EXPECT_THAT(t1, Eq(r1));
}

TEST(SerializeExtensionStdVariant, CanOverloadDefaultSerializationFunction)
{
  std::variant<MyStruct2, MyStruct1, int32_t> t1{ MyStruct1{ 5, 9 } };
  std::variant<MyStruct2, MyStruct1, int32_t> r1{ MyStruct1{} };
  SerializationContext ctx;
  auto exec = [](auto& s, std::variant<MyStruct2, MyStruct1, int32_t>& o) {
    using S = decltype(s);
    s.ext(o,
          bitsery::ext::StdVariant{ [](S& s, MyStruct1& v) {
                                     s.value4b(v.i1);
                                     // do not serialize other element, it
                                     // should be 0 (default)
                                   },
                                    OverloadValue<int32_t, 4>{} });
  };

  ctx.createSerializer().object(t1, exec);
  ctx.createDeserializer().object(r1, exec);
  EXPECT_THAT(std::get<1>(r1).i2, Eq(0));
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

TEST(SerializeExtensionStdVariant, CanUseNonDefaultConstructableTypes)
{
  std::variant<NonDefaultConstructable, MyStruct1, int32_t> t1{
    NonDefaultConstructable{ 123.456f }
  };
  std::variant<NonDefaultConstructable, MyStruct1, int32_t> r1{ MyStruct1{} };
  SerializationContext ctx;

  auto exec = [](auto& s,
                 std::variant<NonDefaultConstructable, MyStruct1, int32_t>& o) {
    using S = decltype(s);
    s.ext(o,
          bitsery::ext::StdVariant{
            [](S& s, NonDefaultConstructable& v) { s.value4b(v._x); },
            OverloadValue<int32_t, 4>{} });
  };

  ctx.createSerializer().object(t1, exec);
  ctx.createDeserializer().object(r1, exec);

  EXPECT_THAT(t1, Eq(r1));
}

TEST(SerializeExtensionStdVariant, CorrectlyHandleMonoState)
{
  std::variant<std::monostate, NonDefaultConstructable, MyStruct1> t1{};
  std::variant<std::monostate, NonDefaultConstructable, MyStruct1> r1{};
  SerializationContext ctx;

  auto exec = [](auto& s, auto& o) {
    using S = decltype(s);
    s.ext(o,
          bitsery::ext::StdVariant{
            [](S& s, NonDefaultConstructable& v) { s.value4b(v._x); },
          });
  };

  ctx.createSerializer().object(t1, exec);
  ctx.createDeserializer().object(r1, exec);

  EXPECT_THAT(t1, Eq(r1));
  std::variant<std::monostate> t2{};
  std::variant<std::monostate> r2{};
  SerializationContext ctx1;
  ctx1.createSerializer().ext(t2, bitsery::ext::StdVariant{});
  ctx1.createDeserializer().ext(r2, bitsery::ext::StdVariant{});
  EXPECT_THAT(t2, Eq(r2));
}

#elif defined(_MSC_VER)
#pragma message(                                                               \
    "C++17 and /Zc:__cplusplus option is required to enable std::variant tests")
#else
#pragma message("C++17 is required to enable std::variant tests")
#endif
