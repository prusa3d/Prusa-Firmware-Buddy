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

#include <bitsery/ext/pointer.h>
#include <bitsery/ext/std_map.h>
#include <bitsery/ext/std_set.h>
#include <bitsery/ext/std_smart_ptr.h>
#include <bitsery/traits/forward_list.h>
#include <forward_list>

#include "serialization_test_utils.h"
#include <gmock/gmock.h>

using testing::ContainerEq;
using testing::Eq;

// forward declare, for testing with std::unordered_map
class HasherForNonDefaultConstructible;

class NonDefaultConstructible
{
  int32_t i{ 0 };
  friend class HasherForNonDefaultConstructible;

  friend class bitsery::Access;
  NonDefaultConstructible() = default;

  template<typename S>
  void serialize(S& s)
  {
    s.value4b(i);
  }

public:
  explicit NonDefaultConstructible(int32_t v)
    : i{ v }
  {
  }

  bool operator==(const NonDefaultConstructible& other) const
  {
    return i == other.i;
  }

  bool operator<(const NonDefaultConstructible& other) const
  {
    return i < other.i;
  }
};

class HasherForNonDefaultConstructible
{
public:
  size_t operator()(const NonDefaultConstructible& o) const
  {
    return std::hash<int32_t>()(o.i);
  }
};

TEST(DeserializeNonDefaultConstructible, Container)
{
  SerializationContext ctx{};
  std::vector<NonDefaultConstructible> data{};
  data.emplace_back(1);
  data.emplace_back(2);
  data.emplace_back(3);
  std::vector<NonDefaultConstructible> res{};

  ctx.createSerializer().container(data, 10);
  ctx.createDeserializer().container(res, 10);

  EXPECT_THAT(res, ContainerEq(data));
}
// this test is here, because when object is not constructible we cannot simple
// "resize" container
TEST(DeserializeNonDefaultConstructible, ResultContainerShouldShrink)
{
  SerializationContext ctx{};
  std::vector<NonDefaultConstructible> data{};
  data.emplace_back(1);
  std::vector<NonDefaultConstructible> res{};
  res.emplace_back(2);
  res.emplace_back(3);
  res.emplace_back(4);

  ctx.createSerializer().container(data, 10);
  ctx.createDeserializer().container(res, 10);

  EXPECT_THAT(res, ContainerEq(data));
}

TEST(DeserializeNonDefaultConstructible, ResultStdForwardListShouldShrink)
{
  // forward list doesn't have .erase function, bet has erase_after
  // in this case, if new size is 0 it must call clear, so we need to check two
  // cases
  {
    // 1) when result should have more than 0 elements
    SerializationContext ctx{};
    std::forward_list<NonDefaultConstructible> data{};
    data.push_front(NonDefaultConstructible{ 1 });
    std::forward_list<NonDefaultConstructible> res{};
    res.push_front(NonDefaultConstructible{ 21 });
    res.push_front(NonDefaultConstructible{ 14 });

    ctx.createSerializer().container(data, 10);
    ctx.createDeserializer().container(res, 10);

    auto resIt = res.begin();
    for (auto it = data.begin(); it != data.end(); ++it, ++resIt) {
      EXPECT_THAT(*resIt, Eq(*it));
    }
    EXPECT_THAT(resIt, Eq(res.end()));
  }
  {
    // 1) when result should have 0 elements
    SerializationContext ctx{};
    std::forward_list<NonDefaultConstructible> data{};
    std::forward_list<NonDefaultConstructible> res{};
    res.push_front(NonDefaultConstructible{ 1 });
    res.push_front(NonDefaultConstructible{ 14 });

    ctx.createSerializer().container(data, 10);
    ctx.createDeserializer().container(res, 10);

    EXPECT_THAT(res.begin(), Eq(res.end()));
  }

  {
    // also check if correctly expands if source is bigger than destination
    SerializationContext ctx{};
    std::forward_list<NonDefaultConstructible> data{};
    data.push_front(NonDefaultConstructible{ 1 });
    data.push_front(NonDefaultConstructible{ 14 });
    std::forward_list<NonDefaultConstructible> res{};

    ctx.createSerializer().container(data, 10);
    ctx.createDeserializer().container(res, 10);

    auto resIt = res.begin();
    for (auto it = data.begin(); it != data.end(); ++it, ++resIt) {
      EXPECT_THAT(*resIt, Eq(*it));
    }
    EXPECT_THAT(resIt, Eq(res.end()));
  }
}

TEST(DeserializeNonDefaultConstructible, StdSet)
{
  SerializationContext ctx{};
  std::set<NonDefaultConstructible> data;
  data.insert(NonDefaultConstructible{ 1 });
  data.insert(NonDefaultConstructible{ 2 });
  std::set<NonDefaultConstructible> res{};
  data.insert(NonDefaultConstructible{ 3 });

  ctx.createSerializer().ext(data, bitsery::ext::StdSet{ 10 });
  ctx.createDeserializer().ext(res, bitsery::ext::StdSet{ 10 });

  EXPECT_THAT(res, ContainerEq(data));
}

TEST(DeserializeNonDefaultConstructible, StdMap)
{
  SerializationContext ctx{};
  std::unordered_map<NonDefaultConstructible,
                     NonDefaultConstructible,
                     HasherForNonDefaultConstructible>
    data;
  data.emplace(NonDefaultConstructible{ 2 }, NonDefaultConstructible{ 3 });

  std::unordered_map<NonDefaultConstructible,
                     NonDefaultConstructible,
                     HasherForNonDefaultConstructible>
    res{};
  data.emplace(NonDefaultConstructible{ 2 }, NonDefaultConstructible{ 3 });
  data.emplace(NonDefaultConstructible{ 4 }, NonDefaultConstructible{ 4 });

  auto& ser = ctx.createSerializer();
  ser.ext(data,
          bitsery::ext::StdMap{ 10 },
          [](decltype(ser)& ser,
             NonDefaultConstructible& key,
             NonDefaultConstructible& value) {
            ser.object(key);
            ser.object(value);
          });
  auto& des = ctx.createDeserializer();
  des.ext(res,
          bitsery::ext::StdMap{ 10 },
          [](decltype(des)& des,
             NonDefaultConstructible& key,
             NonDefaultConstructible& value) {
            des.object(key);
            des.object(value);
          });

  EXPECT_THAT(res, ContainerEq(data));
}

struct NonPolymorphicPointers
{
  NonDefaultConstructible* pp;
  std::unique_ptr<NonDefaultConstructible> up;
  std::shared_ptr<NonDefaultConstructible> sp;
  std::weak_ptr<NonDefaultConstructible> wp;
};

template<typename S>
void
serialize(S& s, NonPolymorphicPointers& o)
{
  s.ext(o.pp, bitsery::ext::PointerOwner{});
  s.ext(o.up, bitsery::ext::StdSmartPtr{});
  s.ext(o.sp, bitsery::ext::StdSmartPtr{});
  s.ext(o.wp, bitsery::ext::StdSmartPtr{});
}

TEST(DeserializeNonDefaultConstructible, NonPolymorphicPointerAndSmartPointer)
{
  using SerContext =
    BasicSerializationContext<bitsery::ext::PointerLinkingContext>;
  SerContext ctx{};
  NonPolymorphicPointers data{};
  data.pp = new NonDefaultConstructible{ 3 };
  data.up =
    std::unique_ptr<NonDefaultConstructible>(new NonDefaultConstructible{ 54 });
  data.sp = std::shared_ptr<NonDefaultConstructible>(
    new NonDefaultConstructible{ -481 });
  data.wp = data.sp;

  NonPolymorphicPointers res{};
  bitsery::ext::PointerLinkingContext plctx1{};
  ctx.createSerializer(plctx1).object(data);
  ctx.createDeserializer(plctx1).object(res);

  EXPECT_THAT(*res.pp, Eq(*data.pp));
  delete res.pp;
  delete data.pp;
  EXPECT_THAT(*res.up, Eq(*data.up));
  EXPECT_THAT(*res.sp, Eq(*data.sp));
  EXPECT_THAT(*(res.wp.lock()), Eq(*(data.wp.lock())));
}

class PolymorphicNDCBase
{
public:
  virtual ~PolymorphicNDCBase() = 0;
  template<typename S>
  void serialize(S&)
  {
  }
};

PolymorphicNDCBase::~PolymorphicNDCBase() = default;

class PolymorphicNDC1 : public PolymorphicNDCBase
{
  int8_t i{};
  friend class bitsery::Access;

  template<typename S>
  void serialize(S& s)
  {
    s.value1b(i);
  }

public:
  PolymorphicNDC1() = default;
  PolymorphicNDC1(int8_t v)
    : i{ v }
  {
  }
  bool operator==(const PolymorphicNDC1& other) const { return i == other.i; }
};

class PolymorphicNDC2 : public PolymorphicNDCBase
{
  uint16_t ui{};

  friend class bitsery::Access;

  template<typename S>
  void serialize(S& s)
  {
    s.value2b(ui);
  }

public:
  PolymorphicNDC2() = default;
  PolymorphicNDC2(uint16_t v)
    : ui{ v }
  {
  }
  bool operator==(const PolymorphicNDC2& other) const { return ui == other.ui; }
};

namespace bitsery {
namespace ext {

template<>
struct PolymorphicBaseClass<PolymorphicNDCBase>
  : PolymorphicDerivedClasses<PolymorphicNDC1, PolymorphicNDC2>
{};
}
}

struct PolymorphicPointers
{
  PolymorphicNDCBase* pp;
  std::unique_ptr<PolymorphicNDCBase> up;
  std::shared_ptr<PolymorphicNDCBase> sp;
  std::weak_ptr<PolymorphicNDCBase> wp;
};

template<typename S>
void
serialize(S& s, PolymorphicPointers& o)
{
  s.ext(o.pp, bitsery::ext::PointerOwner{});
  s.ext(o.up, bitsery::ext::StdSmartPtr{});
  s.ext(o.sp, bitsery::ext::StdSmartPtr{});
  s.ext(o.wp, bitsery::ext::StdSmartPtr{});
}

TEST(DeserializeNonDefaultConstructible, PolymorphicPointerAndSmartPointer)
{
  using TContext =
    std::tuple<bitsery::ext::PointerLinkingContext,
               bitsery::ext::PolymorphicContext<bitsery::ext::StandardRTTI>>;
  using SerContext = BasicSerializationContext<TContext>;
  SerContext ctx{};
  PolymorphicPointers data{};
  data.pp = new PolymorphicNDC1{ -4 };
  data.up = std::unique_ptr<PolymorphicNDCBase>(new PolymorphicNDC2{ 54 });
  data.sp = std::shared_ptr<PolymorphicNDCBase>(new PolymorphicNDC1{ 15 });
  data.wp = data.sp;

  PolymorphicPointers res{};

  TContext serCtx{};
  TContext desCtx{};

  std::get<1>(serCtx).registerBasesList<typename SerContext::TSerializer>(
    bitsery::ext::PolymorphicClassesList<PolymorphicNDCBase>{});
  std::get<1>(desCtx).registerBasesList<typename SerContext::TDeserializer>(
    bitsery::ext::PolymorphicClassesList<PolymorphicNDCBase>{});

  ctx.createSerializer(serCtx).object(data);
  ctx.createDeserializer(desCtx).object(res);
  auto respp = dynamic_cast<PolymorphicNDC1*>(res.pp);
  auto resup = dynamic_cast<PolymorphicNDC2*>(res.up.get());
  auto ressp = dynamic_cast<PolymorphicNDC1*>(res.sp.get());
  auto reswp = dynamic_cast<PolymorphicNDC1*>(res.wp.lock().get());

  auto datapp = dynamic_cast<PolymorphicNDC1*>(data.pp);
  auto dataup = dynamic_cast<PolymorphicNDC2*>(data.up.get());
  auto datasp = dynamic_cast<PolymorphicNDC1*>(data.sp.get());
  auto datawp = dynamic_cast<PolymorphicNDC1*>(data.wp.lock().get());

  EXPECT_THAT(respp, ::testing::Ne(nullptr));
  EXPECT_THAT(resup, ::testing::Ne(nullptr));
  EXPECT_THAT(ressp, ::testing::Ne(nullptr));
  EXPECT_THAT(reswp, ::testing::Ne(nullptr));

  EXPECT_THAT(*respp, Eq(*datapp));
  delete res.pp;
  delete data.pp;
  EXPECT_THAT(*resup, Eq(*dataup));
  EXPECT_THAT(*ressp, Eq(*datasp));
  EXPECT_THAT(*reswp, Eq(*datawp));
  std::get<0>(serCtx).clearSharedState();
  std::get<0>(desCtx).clearSharedState();
}
