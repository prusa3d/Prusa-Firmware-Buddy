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

using bitsery::DefaultConfig;

using SingleTypeContext = int;
using MultipleTypesContext = std::tuple<int, float, char>;

TEST(SerializationContext, WhenContextIsNotTupleThenReturnThisContext)
{
  SingleTypeContext ctx{ 54 };
  BasicSerializationContext<SingleTypeContext> c1;
  auto& ser1 = c1.createSerializer(ctx);

  EXPECT_THAT(ser1.context<SingleTypeContext>(), Eq(ctx));
}

TEST(SerializationContext, WhenContextIsTupleThenReturnsTupleElements)
{

  MultipleTypesContext ctx{ 5, 798.654f, 'F' };
  BasicSerializationContext<MultipleTypesContext> c1;
  auto& ser1 = c1.createSerializer(ctx);

  EXPECT_THAT(ser1.context<int>(), std::get<0>(ctx));
  EXPECT_THAT(ser1.context<float>(), std::get<1>(ctx));
  EXPECT_THAT(ser1.context<char>(), std::get<2>(ctx));
}

TEST(SerializationContext, WhenContextDoesntExistsThenContextOrNullReturnsNull)
{
  SingleTypeContext ctx1 = 32;
  BasicSerializationContext<SingleTypeContext> c1;
  auto& ser = c1.createSerializer(ctx1);
  EXPECT_THAT(ser.contextOrNull<char>(), ::testing::IsNull());
  EXPECT_THAT(ser.contextOrNull<int>(), ::testing::NotNull());
  *ser.contextOrNull<int>() = 2;
  EXPECT_THAT(ctx1, Eq(2));

  MultipleTypesContext ctx2{ 5, 798.654f, 'F' };
  BasicSerializationContext<MultipleTypesContext> c2;
  auto& des = c2.createDeserializer(ctx2);
  EXPECT_THAT(des.contextOrNull<double>(), ::testing::IsNull());
  EXPECT_THAT(des.contextOrNull<int>(), ::testing::NotNull());
  EXPECT_THAT(*des.contextOrNull<char>(), Eq('F'));
  EXPECT_THAT(*des.contextOrNull<int>(), Eq(5));
}

struct Base
{
  int value{};
};
struct Derived : Base
{};

TEST(SerializationContext, ContextWillTryToConvertIfTypeIsConvertible)
{
  Derived ctx1{};
  BasicSerializationContext<Derived> c1;
  auto& ser = c1.createSerializer(ctx1);
  EXPECT_THAT(ser.contextOrNull<Derived>(), ::testing::NotNull());
  EXPECT_THAT(ser.contextOrNull<Base>(), ::testing::NotNull());
  ser.context<Derived>();
  ser.context<Base>();
}

TEST(SerializationContext,
     WhenMultipleConvertibleTypesExistsThenFirstMatchIsTaken)
{
  {
    using CTX1 = std::tuple<Base, int, Derived>;
    CTX1 ctx1{};
    std::get<0>(ctx1).value = 1;
    std::get<2>(ctx1).value = 2;
    BasicSerializationContext<CTX1> c1;
    auto& ser = c1.createSerializer(ctx1);
    EXPECT_THAT(ser.context<Derived>().value, Eq(std::get<2>(ctx1).value));
    EXPECT_THAT(ser.context<Base>().value, Eq(std::get<0>(ctx1).value));
  }

  {
    using CTX2 = std::tuple<float, Derived, Base>;
    CTX2 ctx2{};
    std::get<1>(ctx2).value = 1;
    std::get<2>(ctx2).value = 2;
    BasicSerializationContext<CTX2> c2;
    auto& des = c2.createSerializer(ctx2);

    EXPECT_THAT(des.context<Derived>().value, Eq(std::get<1>(ctx2).value));
    // Base will not be accessable in this case, because Derived is first valid
    // match
    EXPECT_THAT(des.context<Base>().value, Eq(std::get<1>(ctx2).value));
  }
}
