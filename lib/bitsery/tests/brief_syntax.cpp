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

#include <bitsery/brief_syntax.h>
#include <bitsery/brief_syntax/array.h>
#include <bitsery/brief_syntax/atomic.h>
#include <bitsery/brief_syntax/chrono.h>
#include <bitsery/brief_syntax/deque.h>
#include <bitsery/brief_syntax/forward_list.h>
#include <bitsery/brief_syntax/list.h>
#include <bitsery/brief_syntax/map.h>
#include <bitsery/brief_syntax/memory.h>

#include <bitsery/brief_syntax/queue.h>
#include <bitsery/brief_syntax/set.h>
#include <bitsery/brief_syntax/stack.h>
#include <bitsery/brief_syntax/string.h>
#include <bitsery/brief_syntax/unordered_map.h>
#include <bitsery/brief_syntax/unordered_set.h>
#include <bitsery/brief_syntax/vector.h>
#if __cplusplus > 201402L
#include <bitsery/brief_syntax/optional.h>
#include <bitsery/brief_syntax/tuple.h>
#include <bitsery/brief_syntax/variant.h>
#if __cplusplus > 202002L
#include <bitsery/brief_syntax/bitset.h>
#endif
#elif defined(_MSC_VER)
#pragma message(                                                               \
    "C++17 and /Zc:__cplusplus option is required to enable std::tuple and std::variant brief syntax tests")
#else
#pragma message(                                                               \
    "C++17 is required to enable std::tuple and std::variant brief syntax tests")
#endif

#include "serialization_test_utils.h"
#include <gmock/gmock.h>

#include <atomic>
#include <utility>

using testing::Eq;

TEST(BriefSyntax, FundamentalTypesAndBool)
{
  int ti = 8745;
  MyEnumClass te = MyEnumClass::E4;
  float tf = 485.042f;
  double td = -454184.48445;
  bool tb = true;
  SerializationContext ctx{};
  ctx.createSerializer()(ti, te, tf, td, tb);

  // result
  int ri{};
  MyEnumClass re{};
  float rf{};
  double rd{};
  bool rb{};
  ctx.createDeserializer()(ri, re, rf, rd, rb);

  // test
  EXPECT_THAT(ri, Eq(ti));
  EXPECT_THAT(re, Eq(te));
  EXPECT_THAT(rf, Eq(tf));
  EXPECT_THAT(rd, Eq(td));
  EXPECT_THAT(rb, Eq(tb));
}

TEST(BriefSyntax, UseObjectFncInsteadOfValueN)
{
  int ti = 8745;
  MyEnumClass te = MyEnumClass::E4;
  float tf = 485.042f;
  double td = -454184.48445;
  bool tb = true;
  SerializationContext ctx;
  auto& ser = ctx.createSerializer();
  ser.object(ti);
  ser.object(te);
  ser.object(tf);
  ser.object(td);
  ser.object(tb);

  // result
  int ri{};
  MyEnumClass re{};
  float rf{};
  double rd{};
  bool rb{};
  auto& des = ctx.createDeserializer();
  des.object(ri);
  des.object(re);
  des.object(rf);
  des.object(rd);
  des.object(rb);

  // test
  EXPECT_THAT(ri, Eq(ti));
  EXPECT_THAT(re, Eq(te));
  EXPECT_THAT(rf, Eq(tf));
  EXPECT_THAT(rd, Eq(td));
  EXPECT_THAT(rb, Eq(tb));
}

TEST(BriefSyntax, MixDifferentSyntax)
{
  int ti = 8745;
  MyEnumClass te = MyEnumClass::E4;
  float tf = 485.042f;
  double td = -454184.48445;
  bool tb = true;
  SerializationContext ctx;
  auto& ser = ctx.createSerializer();
  ser.value<sizeof(ti)>(ti);
  ser(te, tf, td);
  ser.object(tb);

  // result
  int ri{};
  MyEnumClass re{};
  float rf{};
  double rd{};
  bool rb{};
  auto& des = ctx.createDeserializer();
  des(ri, re, rf);
  des.value8b(rd);
  des.object(rb);

  // test
  EXPECT_THAT(ri, Eq(ti));
  EXPECT_THAT(re, Eq(te));
  EXPECT_THAT(rf, Eq(tf));
  EXPECT_THAT(rd, Eq(td));
  EXPECT_THAT(rb, Eq(tb));
}

template<typename T>
T
procBriefSyntax(const T& testData)
{
  SerializationContext ctx;
  ctx.createSerializer()(testData);
  T res{};
  ctx.createDeserializer()(res);
  return res;
}

template<typename T>
T&&
procBriefSyntaxRvalue(T&& init_value, const T& testData)
{
  SerializationContext ctx;
  ctx.createSerializer()(testData);
  ctx.createDeserializer()(init_value);
  return std::move(init_value);
}

template<typename T>
T
procBriefSyntaxWithMaxSize(const T& testData)
{
  SerializationContext ctx;
  ctx.createSerializer()(bitsery::maxSize(testData, 100));
  T res{};
  ctx.createDeserializer()(bitsery::maxSize(res, 100));
  return res;
}

TEST(BriefSyntax, CStyleArrayForValueTypesAsContainer)
{
  const int t1[3]{ 8748, -484, 45 };
  int r1[3]{ 0, 0, 0 };

  SerializationContext ctx;
  ctx.createSerializer()(bitsery::asContainer(t1));
  ctx.createDeserializer()(bitsery::asContainer(r1));

  EXPECT_THAT(r1, ::testing::ContainerEq(t1));
}

TEST(BriefSyntax, CStyleArrayForIntegralTypesAsText)
{
  const char t1[3]{ "hi" };
  char r1[3]{ 0, 0, 0 };

  SerializationContext ctx;
  ctx.createSerializer()(bitsery::asText(t1));
  ctx.createDeserializer()(bitsery::asText(r1));

  EXPECT_THAT(r1, ::testing::ContainerEq(t1));
}

TEST(BriefSyntax, CStyleArray)
{
  const MyEnumClass t1[3]{ MyEnumClass::E1, MyEnumClass::E4, MyEnumClass::E2 };
  MyEnumClass r1[3]{};

  SerializationContext ctx;
  ctx.createSerializer()(t1);
  ctx.createDeserializer()(r1);

  EXPECT_THAT(r1, ::testing::ContainerEq(t1));
}

TEST(BriefSyntax, StdString)
{
  std::string t1{ "my nice string" };
  std::string t2{};

  EXPECT_THAT(procBriefSyntax(t1), Eq(t1));
  EXPECT_THAT(procBriefSyntax(t2), Eq(t2));
  EXPECT_THAT(procBriefSyntaxWithMaxSize(t1), Eq(t1));
  EXPECT_THAT(procBriefSyntaxWithMaxSize(t2), Eq(t2));
}

TEST(BriefSyntax, StdArray)
{
  std::array<int, 3> t1{ 8748, -484, 45 };
  std::array<int, 0> t2{};

  EXPECT_THAT(procBriefSyntax(t1), Eq(t1));
  EXPECT_THAT(procBriefSyntax(t2), Eq(t2));
}

TEST(BriefSyntax, StdVector)
{
  std::vector<int> t1{ 8748, -484, 45 };
  std::vector<float> t2{ 5.f, 0.198f };

  EXPECT_THAT(procBriefSyntax(t1), Eq(t1));
  EXPECT_THAT(procBriefSyntax(t2), Eq(t2));
  EXPECT_THAT(procBriefSyntaxWithMaxSize(t1), Eq(t1));
  EXPECT_THAT(procBriefSyntaxWithMaxSize(t2), Eq(t2));
}

TEST(BriefSyntax, StdList)
{
  std::list<int> t1{ 8748, -484, 45 };
  std::list<float> t2{ 5.f, 0.198f };

  EXPECT_THAT(procBriefSyntax(t1), Eq(t1));
  EXPECT_THAT(procBriefSyntax(t2), Eq(t2));
  EXPECT_THAT(procBriefSyntaxWithMaxSize(t1), Eq(t1));
  EXPECT_THAT(procBriefSyntaxWithMaxSize(t2), Eq(t2));
}

TEST(BriefSyntax, StdForwardList)
{
  std::forward_list<int> t1{ 8748, -484, 45 };
  std::forward_list<float> t2{ 5.f, 0.198f };

  EXPECT_THAT(procBriefSyntax(t1), Eq(t1));
  EXPECT_THAT(procBriefSyntax(t2), Eq(t2));
  EXPECT_THAT(procBriefSyntaxWithMaxSize(t1), Eq(t1));
  EXPECT_THAT(procBriefSyntaxWithMaxSize(t2), Eq(t2));
}

TEST(BriefSyntax, StdDeque)
{
  std::deque<int> t1{ 8748, -484, 45 };
  std::deque<float> t2{ 5.f, 0.198f };

  EXPECT_THAT(procBriefSyntax(t1), Eq(t1));
  EXPECT_THAT(procBriefSyntax(t2), Eq(t2));
  EXPECT_THAT(procBriefSyntaxWithMaxSize(t1), Eq(t1));
  EXPECT_THAT(procBriefSyntaxWithMaxSize(t2), Eq(t2));
}

TEST(BriefSyntax, StdQueue)
{
  std::queue<std::string> t1;
  t1.push("first");
  t1.push("second string");

  EXPECT_THAT(procBriefSyntax(t1), Eq(t1));
  EXPECT_THAT(procBriefSyntaxWithMaxSize(t1), Eq(t1));
}

TEST(BriefSyntax, StdPriorityQueue)
{
  std::priority_queue<std::string> t1;
  t1.push("first");
  t1.push("second string");
  t1.push("third");
  t1.push("fourth");
  auto r1 = procBriefSyntax(t1);
  // we cannot compare priority queue directly

  EXPECT_THAT(r1.size(), Eq(t1.size()));
  for (auto i = 0u; i < r1.size(); ++i) {
    EXPECT_THAT(r1.top(), Eq(t1.top()));
    r1.pop();
    t1.pop();
  }
}

TEST(BriefSyntax, StdStack)
{
  std::stack<std::string> t1;
  t1.push("first");
  t1.push("second string");

  EXPECT_THAT(procBriefSyntax(t1), Eq(t1));
  EXPECT_THAT(procBriefSyntaxWithMaxSize(t1), Eq(t1));
}

TEST(BriefSyntax, StdUnorderedMap)
{
  std::unordered_map<int, int> t1;
  t1.emplace(3423, 624);
  t1.emplace(-5484, -845);

  EXPECT_THAT(procBriefSyntax(t1), Eq(t1));
  EXPECT_THAT(procBriefSyntaxWithMaxSize(t1), Eq(t1));
}

TEST(BriefSyntax, StdUnorderedMultiMap)
{
  std::unordered_multimap<std::string, int> t1;
  t1.emplace("one", 624);
  t1.emplace("two", -845);
  t1.emplace("one", 897);

  EXPECT_TRUE(procBriefSyntax(t1) == t1);
  EXPECT_TRUE(procBriefSyntaxWithMaxSize(t1) == t1);
}

TEST(BriefSyntax, StdMap)
{
  std::map<int, int> t1;
  t1.emplace(3423, 624);
  t1.emplace(-5484, -845);

  EXPECT_THAT(procBriefSyntax(t1), Eq(t1));
  EXPECT_THAT(procBriefSyntaxWithMaxSize(t1), Eq(t1));
}

TEST(BriefSyntax, StdMultiMap)
{
  std::multimap<std::string, int> t1;
  t1.emplace("one", 624);
  t1.emplace("two", -845);
  t1.emplace("one", 897);

  auto res = procBriefSyntax(t1);
  // same key values is not ordered, and operator == compares each element at
  // same position so we need to compare our selves
  EXPECT_THAT(res.size(), Eq(3));
  for (auto it = t1.begin(); it != t1.end();) {
    const auto lr = t1.equal_range(it->first);
    const auto rr = res.equal_range(it->first);
    EXPECT_TRUE(std::distance(lr.first, lr.second) ==
                std::distance(rr.first, rr.second));
    EXPECT_TRUE(std::is_permutation(lr.first, lr.second, rr.first));
    it = lr.second;
  }
}

TEST(BriefSyntax, StdUnorderedSet)
{
  std::unordered_set<std::string> t1;
  t1.emplace("one");
  t1.emplace("two");
  t1.emplace("three");

  EXPECT_TRUE(procBriefSyntax(t1) == t1);
  EXPECT_TRUE(procBriefSyntaxWithMaxSize(t1) == t1);
}

TEST(BriefSyntax, StdUnorderedMultiSet)
{
  std::unordered_multiset<std::string> t1;
  t1.emplace("one");
  t1.emplace("two");
  t1.emplace("three");
  t1.emplace("one");

  EXPECT_TRUE(procBriefSyntax(t1) == t1);
  EXPECT_TRUE(procBriefSyntaxWithMaxSize(t1) == t1);
}

TEST(BriefSyntax, StdSet)
{
  std::set<std::string> t1;
  t1.emplace("one");
  t1.emplace("two");
  t1.emplace("three");

  EXPECT_TRUE(procBriefSyntax(t1) == t1);
  EXPECT_TRUE(procBriefSyntaxWithMaxSize(t1) == t1);
}

TEST(BriefSyntax, StdMultiSet)
{
  std::multiset<std::string> t1;
  t1.emplace("one");
  t1.emplace("two");
  t1.emplace("three");
  t1.emplace("one");
  t1.emplace("two");

  EXPECT_TRUE(procBriefSyntax(t1) == t1);
  EXPECT_TRUE(procBriefSyntaxWithMaxSize(t1) == t1);
}

TEST(BriefSyntax, StdSmartPtr)
{
  std::shared_ptr<int> dataShared1(new int{ 4 });
  std::weak_ptr<int> dataWeak1(dataShared1);
  std::unique_ptr<std::string> dataUnique1{ new std::string{ "hello world" } };

  bitsery::ext::PointerLinkingContext plctx1{};
  BasicSerializationContext<bitsery::ext::PointerLinkingContext> ctx;
  ctx.createSerializer(plctx1)(dataShared1, dataWeak1, dataUnique1);

  std::shared_ptr<int> resShared1{};
  std::weak_ptr<int> resWeak1{};
  std::unique_ptr<std::string> resUnique1{};
  ctx.createDeserializer(plctx1)(resShared1, resWeak1, resUnique1);
  // clear shared state from pointer linking context
  plctx1.clearSharedState();

  EXPECT_TRUE(plctx1.isValid());
  EXPECT_THAT(*resShared1, Eq(*dataShared1));
  EXPECT_THAT(*resWeak1.lock(), Eq(*dataWeak1.lock()));
  EXPECT_THAT(*resUnique1, Eq(*dataUnique1));
}

TEST(BriefSyntax, StdDuration)
{
  std::chrono::duration<int64_t, std::milli> t1{ 54654 };
  EXPECT_TRUE(procBriefSyntax(t1) == t1);
}

TEST(BriefSyntax, StdTimePoint)
{
  using Duration = std::chrono::duration<double, std::milli>;
  using TP = std::chrono::time_point<std::chrono::system_clock, Duration>;

  TP data{ Duration{ 874656.4798 } };
  EXPECT_TRUE(procBriefSyntax(data) == data);
}

TEST(BriefSyntax, StdAtomic)
{
  std::atomic<int32_t> atm0{ 54654 };
  EXPECT_TRUE(procBriefSyntaxRvalue(std::atomic<int32_t>{}, atm0) == atm0);

  std::atomic<bool> atm1{ false };
  EXPECT_TRUE(procBriefSyntaxRvalue(std::atomic<bool>{}, atm1) == atm1);

  std::atomic<bool> atm2{ true };
  EXPECT_TRUE(procBriefSyntaxRvalue(std::atomic<bool>{}, atm2) == atm2);

  std::atomic<uint16_t> atm3;
  atm3.store(0x1337);
  EXPECT_TRUE(procBriefSyntaxRvalue(std::atomic<uint16_t>{}, atm3).load() ==
              0x1337);
}

#if __cplusplus > 201402L

TEST(BriefSyntax, StdTuple)
{
  std::tuple<int, std::string, std::vector<char>> t1{ 5,
                                                      "hello hello",
                                                      { 'A', 'B', 'C' } };
  EXPECT_TRUE(procBriefSyntax(t1) == t1);
}

TEST(BriefSyntax, StdVariant)
{
  std::variant<float, std::string, std::chrono::milliseconds> t1{ std::string(
    "hello hello") };
  EXPECT_TRUE(procBriefSyntax(t1) == t1);
}

TEST(BriefSyntax, StdOptional)
{
  std::optional<uint32_t> opt{ 54654 };
  EXPECT_TRUE(procBriefSyntax(opt) == opt);
}

#if __cplusplus > 202002L

TEST(BriefSyntax, StdBitset)
{
  std::bitset<17> bits{ 0b10101010101010101 };
  EXPECT_TRUE(procBriefSyntax(bits) == bits);
}

#endif

#endif

TEST(BriefSyntax, NestedTypes)
{
  std::unordered_map<std::string, std::vector<std::string>> t1;
  t1.emplace("my key", std::vector<std::string>{ "very", "nice", "string" });
  t1.emplace("other key", std::vector<std::string>{ "just a string" });

  EXPECT_THAT(procBriefSyntax(t1), Eq(t1));
  EXPECT_THAT(procBriefSyntaxWithMaxSize(t1), Eq(t1));
}
