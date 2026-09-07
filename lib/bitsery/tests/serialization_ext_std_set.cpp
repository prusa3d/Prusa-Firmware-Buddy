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

#include <bitsery/ext/std_set.h>
#include <set>

#include "serialization_test_utils.h"
#include <gmock/gmock.h>

using StdSet = bitsery::ext::StdSet;

using testing::Eq;

template<typename T>
class SerializeExtensionStdSet : public testing::Test
{
public:
  using TContainer = T;
  const TContainer src = { 4, 8, 48, 4, 9845, 64, 8 };
  TContainer res{ 78, 74, 154, 8 };
};

using SerializeExtensionStdSetTypes =
  ::testing::Types<std::unordered_set<int32_t>,
                   std::unordered_multiset<int32_t>,
                   std::set<int32_t>,
                   std::multiset<int32_t>>;

TYPED_TEST_SUITE(SerializeExtensionStdSet, SerializeExtensionStdSetTypes, );

TYPED_TEST(SerializeExtensionStdSet, ValuesSyntaxDifferentSetTypes)
{
  SerializationContext ctx1;
  ctx1.createSerializer().ext4b(this->src, StdSet{ 10 });
  ctx1.createDeserializer().ext4b(this->res, StdSet{ 10 });
  EXPECT_THAT(this->res, Eq(this->src));
}

TEST(SerializeExtensionStdSet, ObjectSyntax)
{
  SerializationContext ctx1;
  std::set<MyStruct1> t1{ MyStruct1{ 874, 456 },
                          MyStruct1{ -874, -456 },
                          MyStruct1{ 4894, 0 } };
  std::set<MyStruct1> r1{};
  ctx1.createSerializer().ext(t1, StdSet{ 10 });
  ctx1.createDeserializer().ext(r1, StdSet{ 10 });
  EXPECT_THAT(r1, Eq(t1));
}

TEST(SerializeExtensionStdSet, FunctionSyntax)
{
  SerializationContext ctx1;
  std::unordered_multiset<int32_t> t1{ 54, -484, 841, 79 };
  std::unordered_multiset<int32_t> r1{ 74, 878, 15, 16, -7, 5, -4, 8, 7 };
  auto& ser = ctx1.createSerializer();
  ser.ext(
    t1, StdSet{ 10 }, [](decltype(ser)& ser, int32_t& v) { ser.value4b(v); });
  auto& des = ctx1.createDeserializer();
  des.ext(
    r1, StdSet{ 10 }, [](decltype(des)& des, int32_t& v) { des.value4b(v); });
  EXPECT_THAT(r1, Eq(t1));
}
