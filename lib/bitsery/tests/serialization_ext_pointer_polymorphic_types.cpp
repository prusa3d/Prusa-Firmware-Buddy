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

#include <bitsery/ext/inheritance.h>
#include <bitsery/ext/pointer.h>

#include "serialization_test_utils.h"
#include <gmock/gmock.h>

using bitsery::ext::BaseClass;
using bitsery::ext::VirtualBaseClass;

using bitsery::ext::InheritanceContext;
using bitsery::ext::PointerLinkingContext;
using bitsery::ext::PolymorphicContext;
using bitsery::ext::StandardRTTI;

using bitsery::ext::PointerObserver;
using bitsery::ext::PointerOwner;
using bitsery::ext::ReferencedByPointer;

using testing::Eq;

using TContext = std::tuple<PointerLinkingContext,
                            InheritanceContext,
                            PolymorphicContext<StandardRTTI>>;
using SerContext = BasicSerializationContext<TContext>;

// this is useful for PolymorphicContext to bind classes to
// serializer/deserializer
using TSerializer = typename SerContext::TSerializer;
using TDeserializer = typename SerContext::TDeserializer;

/*
 * base class
 */
struct Base
{
  uint8_t x{};

  virtual ~Base() = default;
};

template<typename S>
void
serialize(S& s, Base& o)
{
  s.value1b(o.x);
}

struct Derived1 : virtual Base
{
  uint8_t y1{};
};

template<typename S>
void
serialize(S& s, Derived1& o)
{
  s.ext(o, VirtualBaseClass<Base>{});
  s.value1b(o.y1);
}

struct Derived2 : virtual Base
{
  uint8_t y2{};
};

template<typename S>
void
serialize(S& s, Derived2& o)
{
  s.ext(o, VirtualBaseClass<Base>{});
  s.value1b(o.y2);
}

struct MultipleVirtualInheritance
  : Derived1
  , Derived2
{
  int8_t z{};

  MultipleVirtualInheritance() = default;

  MultipleVirtualInheritance(uint8_t x_, uint8_t y1_, uint8_t y2_, int8_t z_)
  {
    x = x_;
    y1 = y1_;
    y2 = y2_;
    z = z_;
  }

  template<typename S>
  void serialize(S& s)
  {
    s.ext(*this, BaseClass<Derived1>{});
    s.ext(*this, BaseClass<Derived2>{});
    s.value1b(z);
  }
};

// this class has no relationships specified via PolymorphicBaseClass
struct NoRelationshipSpecifiedDerived : Base
{};

// these classes will be used to "cheat" a little bit when testing
// deserialization flows
struct BaseClone
{
  uint8_t x{};

  virtual ~BaseClone() = default;
};

template<typename S>
void
serialize(S& s, BaseClone& o)
{
  s.value1b(o.x);
}

// define relationships between base class and derived classes for runtime
// polymorphism

namespace bitsery {
namespace ext {

template<>
struct PolymorphicBaseClass<Base>
  : PolymorphicDerivedClasses<Derived1, Derived2>
{};

// this is commented on purpose, to test scenario when base class is registered
// (Base) but using instance of Derived1 which is not registered as base
//        template<>
//        struct PolymorphicBaseClass<Derived1> :
//        PolymorphicDerivedClasses<MultipleVirtualInheritance> {
//        };

template<>
struct PolymorphicBaseClass<Derived2>
  : PolymorphicDerivedClasses<MultipleVirtualInheritance>
{};

}
}

class SerializeExtensionPointerPolymorphicTypes : public testing::Test
{
public:
  TContext plctx{};
  SerContext sctx{};

  typename SerContext::TSerializer& createSerializer()
  {
    auto& res = sctx.createSerializer(plctx);
    std::get<2>(plctx).clear();
    // bind serializer with classes
    std::get<2>(plctx).registerBasesList<SerContext::TSerializer>(
      bitsery::ext::PolymorphicClassesList<Base>{});
    return res;
  }

  typename SerContext::TDeserializer& createDeserializer()
  {
    auto& res = sctx.createDeserializer(plctx);
    std::get<2>(plctx).clear();
    // bind deserializer with classes
    std::get<2>(plctx).registerBasesList<SerContext::TDeserializer>(
      bitsery::ext::PolymorphicClassesList<Base>{});
    return res;
  }

  bool isPointerContextValid() { return std::get<0>(plctx).isValid(); }

  virtual void TearDown() override { EXPECT_TRUE(isPointerContextValid()); }
};

TEST_F(SerializeExtensionPointerPolymorphicTypes, Data0Result0)
{
  Base* baseData = nullptr;
  createSerializer().ext(baseData, PointerOwner{});
  Base* baseRes = nullptr;
  createDeserializer().ext(baseRes, PointerOwner{});

  EXPECT_THAT(baseRes, ::testing::IsNull());
  EXPECT_THAT(baseData, ::testing::IsNull());
}

TEST_F(SerializeExtensionPointerPolymorphicTypes, Data0Result1)
{
  Base* baseData = nullptr;
  createSerializer().ext(baseData, PointerOwner{});

  Base* baseRes = new Derived1{};
  createDeserializer().ext(baseRes, PointerOwner{});

  EXPECT_THAT(baseRes, ::testing::IsNull());
  EXPECT_THAT(baseData, ::testing::IsNull());
}

TEST_F(SerializeExtensionPointerPolymorphicTypes, Data1Result0)
{
  Derived1 d1{};
  d1.x = 3;
  d1.y1 = 78;
  Base* baseData = &d1;
  createSerializer().ext(baseData, PointerOwner{});
  Base* baseRes = nullptr;
  createDeserializer().ext(baseRes, PointerOwner{});

  auto* data = dynamic_cast<Derived1*>(baseData);
  auto* res = dynamic_cast<Derived1*>(baseRes);

  EXPECT_THAT(baseRes, ::testing::NotNull());
  EXPECT_THAT(data, ::testing::NotNull());
  EXPECT_THAT(res, ::testing::NotNull());
  EXPECT_THAT(res->x, Eq(data->x));
  EXPECT_THAT(res->y1, Eq(data->y1));
  delete baseRes;
}

TEST_F(SerializeExtensionPointerPolymorphicTypes, Data1Result1)
{
  Derived1 d1{};
  d1.x = 3;
  d1.y1 = 78;
  Base* baseData = &d1;
  createSerializer().ext(baseData, PointerOwner{});
  Base* baseRes = &d1;
  createDeserializer().ext(baseRes, PointerOwner{});

  auto* data = dynamic_cast<Derived1*>(baseData);
  auto* res = dynamic_cast<Derived1*>(baseRes);

  EXPECT_THAT(baseRes, ::testing::NotNull());
  EXPECT_THAT(data, ::testing::NotNull());
  EXPECT_THAT(res, ::testing::NotNull());
  EXPECT_THAT(res->x, Eq(data->x));
  EXPECT_THAT(res->y1, Eq(data->y1));
}

TEST_F(SerializeExtensionPointerPolymorphicTypes,
       ComplexTypeWithVirtualInheritanceData1Result0)
{
  MultipleVirtualInheritance md1{};
  md1.x = 3;
  md1.y1 = 78;
  md1.y2 = 14;
  md1.z = -33;
  Base* baseData = &md1;
  createSerializer().ext(baseData, PointerOwner{});
  Base* baseRes = nullptr;
  createDeserializer().ext(baseRes, PointerOwner{});

  auto* data = dynamic_cast<MultipleVirtualInheritance*>(baseData);
  auto* res = dynamic_cast<MultipleVirtualInheritance*>(baseRes);

  EXPECT_THAT(baseRes, ::testing::NotNull());
  EXPECT_THAT(data, ::testing::NotNull());
  EXPECT_THAT(res, ::testing::NotNull());
  EXPECT_THAT(res->x, Eq(data->x));
  EXPECT_THAT(res->y1, Eq(data->y1));
  EXPECT_THAT(res->y2, Eq(data->y2));
  EXPECT_THAT(res->z, Eq(data->z));
  delete baseRes;
}

TEST_F(SerializeExtensionPointerPolymorphicTypes,
       WhenResultIsDifferentTypeThenRecreate)
{
  MultipleVirtualInheritance md1{};
  md1.x = 3;
  md1.y1 = 78;
  md1.y2 = 14;
  md1.z = -33;
  Base* baseData = &md1;
  createSerializer().ext(baseData, PointerOwner{});
  Base* baseRes = new Derived1{};
  EXPECT_THAT(dynamic_cast<MultipleVirtualInheritance*>(baseRes),
              ::testing::IsNull());
  createDeserializer().ext(baseRes, PointerOwner{});
  EXPECT_THAT(dynamic_cast<MultipleVirtualInheritance*>(baseRes),
              ::testing::NotNull());
  delete baseRes;
}

#ifndef NDEBUG

TEST_F(
  SerializeExtensionPointerPolymorphicTypes,
  WhenSerializingDerivedTypeWithoutSpecifiedRelationshipsWithBaseThenAssert)
{

  NoRelationshipSpecifiedDerived
    md1; // this class has no relationships specified via PolymorphicBaseClass
  Base* baseData = &md1;
  EXPECT_DEATH(createSerializer().ext(baseData, PointerOwner{}), "");
}

TEST_F(
  SerializeExtensionPointerPolymorphicTypes,
  WhenDeserializingDerivedTypeNotRegisteredWithPolymorphicContextThenAssert)
{

  Derived1 d1{};
  Base* baseData = &d1;
  createSerializer().ext(baseData, PointerOwner{});

  BaseClone* baseRes = nullptr; // this class is not registered
  EXPECT_DEATH(createDeserializer().ext(baseRes, PointerOwner{}), "");
}

#endif

TEST_F(
  SerializeExtensionPointerPolymorphicTypes,
  CompileTimeTypeIsDerivedAndReachableFromBaseRegisteredWithPolymorphicContext)
{

  MultipleVirtualInheritance md;
  Derived2* derivedData =
    &md; // this class is not registered via PolymorphicContext

  createSerializer().ext(derivedData, PointerOwner{});
  Derived2* derivedRes = new Derived2{};
  EXPECT_THAT(dynamic_cast<MultipleVirtualInheritance*>(derivedRes),
              ::testing::IsNull());
  createDeserializer().ext(derivedRes, PointerOwner{});
  EXPECT_THAT(dynamic_cast<MultipleVirtualInheritance*>(derivedRes),
              ::testing::NotNull());
  delete derivedRes;
}

TEST_F(SerializeExtensionPointerPolymorphicTypes,
       WhenPolymorphicTypeNotFoundDuringDeserializionThenInvalidPointerError)
{

  Derived1 d1{};
  Base* baseData = &d1;
  createSerializer().ext(baseData, PointerOwner{});

  BaseClone* baseRes =
    nullptr; // this class will be registered, but it doesn't have relationships
             // specified via PolymorphicBaseClass
  auto& des = sctx.createDeserializer(plctx);
  auto& pc = std::get<2>(plctx);
  pc.clear();
  pc.registerBasesList<SerContext::TDeserializer>(
    bitsery::ext::PolymorphicClassesList<BaseClone>{});
  des.ext(baseRes, PointerOwner{});
  EXPECT_THAT(sctx.des->adapter().error(),
              Eq(bitsery::ReaderError::InvalidPointer));
}
