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

#include <bitsery/traits/array.h>
#include <bitsery/traits/string.h>

#include "serialization_test_utils.h"
#include <gmock/gmock.h>

using testing::ContainerEq;
using testing::Eq;
using testing::StrEq;

struct X
{
  X(){};
  X(int v)
    : x{ v }
  {
  }
  std::string s{};
  int x{};
  bool operator==(const X& r) const { return r.x == x && r.s == s; }
};

struct Y
{
  int y{};
  int carr[3]{};
  std::array<int, 3> arr{};
  std::vector<X> vx{};
  std::string s{};
};
struct Z
{
  X x{};
  Y y{};
};

template<typename S>
void
serialize(S& s, Z& o)
{
  s.object(o.x);
  s.object(o.y);
}

template<typename S>
void
serialize(S& s, X& o)
{
  s.template value<sizeof(o.x)>(o.x);
  s.template text<1>(o.s, 1000);
}

template<typename S>
void
serialize(S& s, Y& o)
{
  auto writeInt = [](S& s, int& v) { s.template value<sizeof(v)>(v); };
  s.template text<1>(o.s, 10000);
  s.template value<sizeof(o.y)>(o.y);
  s.container(o.arr, writeInt);
  s.container(o.carr, writeInt);
  s.container(o.vx, 10000, [](S& s, X& v) { s.object(v); });
}

TEST(SerializeObject, GeneralConceptTest)
{
  // std::string buf;
  SerializationContext ctx;
  Y y{};
  y.y = 3423;
  y.arr[0] = 111;
  y.arr[1] = 222;
  y.arr[2] = 333;
  y.carr[0] = 123;
  y.carr[1] = 456;
  y.carr[2] = 789;
  y.vx.push_back(X(234));
  y.vx.push_back(X(6245));
  y.vx.push_back(X(613461));
  y.s = "labal diena";

  Z z{};
  z.y = y;
  z.x = X{ 234 };

  auto& ser = ctx.createSerializer();
  ser.object(y);
  ser.object(z);

  Y yres{};
  Z zres{};

  auto& des = ctx.createDeserializer();
  des.object(yres);
  des.object(zres);

  EXPECT_THAT(yres.y, Eq(y.y));
  EXPECT_THAT(yres.vx, ContainerEq(y.vx));
  EXPECT_THAT(yres.arr, ContainerEq(y.arr));
  EXPECT_THAT(yres.carr, ContainerEq(y.carr));
  EXPECT_THAT(yres.s, StrEq(y.s));

  EXPECT_THAT(zres.y.y, Eq(z.y.y));
  EXPECT_THAT(zres.y.vx, ContainerEq(z.y.vx));
  EXPECT_THAT(zres.y.arr, ContainerEq(z.y.arr));
  EXPECT_THAT(zres.y.carr, ContainerEq(z.y.carr));
  EXPECT_THAT(zres.y.s, StrEq(z.y.s));
  EXPECT_THAT(zres.x.s, StrEq(z.x.s));
  EXPECT_THAT(zres.x.x, Eq(z.x.x));
}
