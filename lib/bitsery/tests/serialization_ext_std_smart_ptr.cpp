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
#include <bitsery/ext/std_smart_ptr.h>

#include "serialization_test_utils.h"
#include <gmock/gmock.h>

using bitsery::ext::BaseClass;
using bitsery::ext::VirtualBaseClass;

using bitsery::ext::InheritanceContext;
using bitsery::ext::PointerLinkingContext;
using bitsery::ext::PointerType;
using bitsery::ext::PolymorphicContext;
using bitsery::ext::StandardRTTI;

using bitsery::ext::PointerObserver;
using bitsery::ext::StdSmartPtr;

using testing::Eq;
using testing::Ne;

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

struct Derived : virtual Base
{
  uint8_t y{};

  Derived() = default;

  Derived(uint8_t x_, uint8_t y_)
  {
    x = x_;
    y = y_;
  }
};

template<typename S>
void
serialize(S& s, Derived& o)
{
  s.ext(o, VirtualBaseClass<Base>{});
  s.value1b(o.y);
}

struct MoreDerived : Derived
{
  uint8_t z{};

  MoreDerived() = default;

  MoreDerived(uint8_t x_, uint8_t y_, uint8_t z_)
    : Derived(x_, y_)
  {
    z = z_;
  }
};

template<typename S>
void
serialize(S& s, MoreDerived& o)
{
  s.ext(o, BaseClass<Derived>{});
  s.value1b(o.z);
}

// define relationships between base class and derived classes for runtime
// polymorphism

namespace bitsery {
namespace ext {

template<>
struct PolymorphicBaseClass<Base> : PolymorphicDerivedClasses<Derived>
{};

template<>
struct PolymorphicBaseClass<Derived> : PolymorphicDerivedClasses<MoreDerived>
{};

}
}

template<typename T>
class SerializeExtensionStdSmartPtrNonPolymorphicType : public testing::Test
{
public:
  template<typename U>
  using TPtr = typename T::template TData<U>;
  using TExt = typename T::TExt;

  using TContext = std::tuple<PointerLinkingContext>;
  using SerContext = BasicSerializationContext<TContext>;

  // this is useful for PolymorphicContext to bind classes to
  // serializer/deserializer
  using TSerializer = typename SerContext::TSerializer;
  using TDeserializer = typename SerContext::TDeserializer;

  TContext plctx{};
  SerContext sctx{};

  typename SerContext::TSerializer& createSerializer()
  {
    return sctx.createSerializer(plctx);
  }

  typename SerContext::TDeserializer& createDeserializer()
  {
    return sctx.createDeserializer(plctx);
  }

  bool isPointerContextValid() { return std::get<0>(plctx).isValid(); }

  virtual void TearDown() override { EXPECT_TRUE(isPointerContextValid()); }
};

template<typename T>
class SerializeExtensionStdSmartPtrPolymorphicType : public testing::Test
{
public:
  template<typename U>
  using TPtr = typename T::template TData<U>;
  using TExt = typename T::TExt;

  using TContext = std::tuple<PointerLinkingContext,
                              InheritanceContext,
                              PolymorphicContext<StandardRTTI>>;
  using SerContext = BasicSerializationContext<TContext>;

  // this is useful for PolymorphicContext to bind classes to
  // serializer/deserializer
  using TSerializer = typename SerContext::TSerializer;
  using TDeserializer = typename SerContext::TDeserializer;

  TContext plctx{};
  SerContext sctx{};

  typename SerContext::TSerializer& createSerializer()
  {
    auto& res = sctx.createSerializer(plctx);
    std::get<2>(plctx).clear();
    // bind serializer with classes
    std::get<2>(plctx).template registerBasesList<SerContext::TSerializer>(
      bitsery::ext::PolymorphicClassesList<Base>{});
    return res;
  }

  typename SerContext::TDeserializer& createDeserializer()
  {
    auto& res = sctx.createDeserializer(plctx);
    std::get<2>(plctx).clear();
    // bind deserializer with classes
    std::get<2>(plctx).template registerBasesList<SerContext::TDeserializer>(
      bitsery::ext::PolymorphicClassesList<Base>{});
    return res;
  }

  bool isPointerContextValid() { return std::get<0>(plctx).isValid(); }

  virtual void TearDown() override { EXPECT_TRUE(isPointerContextValid()); }
};

struct UniquePtrTest
{
  template<typename T>
  using TData = std::unique_ptr<T>;
  using TExt = StdSmartPtr;
};

struct SharedPtrTest
{
  template<typename T>
  using TData = std::shared_ptr<T>;
  using TExt = StdSmartPtr;
};

using TestingWithNonPolymorphicTypes =
  ::testing::Types<UniquePtrTest, SharedPtrTest>;

TYPED_TEST_SUITE(SerializeExtensionStdSmartPtrNonPolymorphicType,
                 TestingWithNonPolymorphicTypes, );

using TestingWithPolymorphicTypes =
  ::testing::Types<UniquePtrTest, SharedPtrTest>;

TYPED_TEST_SUITE(SerializeExtensionStdSmartPtrPolymorphicType,
                 TestingWithPolymorphicTypes, );

TYPED_TEST(SerializeExtensionStdSmartPtrNonPolymorphicType, Data0Result0)
{
  using Ptr = typename TestFixture::template TPtr<MyStruct1>;
  using Ext = typename TestFixture::TExt;

  Ptr data{};
  this->createSerializer().ext(data, Ext{});
  Ptr res{};
  this->createDeserializer().ext(res, Ext{});

  EXPECT_THAT(data.get(), ::testing::IsNull());
  EXPECT_THAT(res.get(), ::testing::IsNull());
}

TYPED_TEST(SerializeExtensionStdSmartPtrNonPolymorphicType, Data0Result1)
{
  using Ptr = typename TestFixture::template TPtr<MyStruct1>;
  using Ext = typename TestFixture::TExt;

  Ptr data{};
  this->createSerializer().ext(data, Ext{});
  Ptr res{ new MyStruct1{} };
  this->createDeserializer().ext(res, Ext{});

  EXPECT_THAT(data.get(), ::testing::IsNull());
  EXPECT_THAT(res.get(), ::testing::IsNull());
}

TYPED_TEST(SerializeExtensionStdSmartPtrNonPolymorphicType, Data1Result0)
{
  using Ptr = typename TestFixture::template TPtr<MyStruct1>;
  using Ext = typename TestFixture::TExt;

  Ptr data{ new MyStruct1{ 3, 78 } };
  this->createSerializer().ext(data, Ext{});
  Ptr res{};
  this->createDeserializer().ext(res, Ext{});

  EXPECT_THAT(data.get(), ::testing::NotNull());
  EXPECT_THAT(res.get(), ::testing::NotNull());
  EXPECT_THAT(res->i1, Eq(data->i1));
  EXPECT_THAT(res->i2, Eq(data->i2));
}

TYPED_TEST(SerializeExtensionStdSmartPtrNonPolymorphicType, Data1Result1)
{
  using Ptr = typename TestFixture::template TPtr<MyStruct1>;
  using Ext = typename TestFixture::TExt;

  Ptr data{ new MyStruct1{ 3, 78 } };
  this->createSerializer().ext(data, Ext{});
  Ptr res{ new MyStruct1{} };
  this->createDeserializer().ext(res, Ext{});

  EXPECT_THAT(data.get(), ::testing::NotNull());
  EXPECT_THAT(res.get(), ::testing::NotNull());
  EXPECT_THAT(res->i1, Eq(data->i1));
  EXPECT_THAT(res->i2, Eq(data->i2));
}

TYPED_TEST(SerializeExtensionStdSmartPtrNonPolymorphicType,
           CanUseLambdaOverload)
{
  using Ptr = typename TestFixture::template TPtr<MyStruct1>;
  using Ext = typename TestFixture::TExt;

  Ptr data{ new MyStruct1{ 3, 78 } };
  auto& ser = this->createSerializer();
  ser.ext(data, Ext{}, [](decltype(ser)& ser, MyStruct1& o) {
    // serialize only one field
    ser.value4b(o.i1);
  });
  Ptr res{ new MyStruct1{ 97, 12 } };
  auto& des = this->createDeserializer();
  des.ext(
    res, Ext{}, [](decltype(des)& des, MyStruct1& o) { des.value4b(o.i1); });

  EXPECT_THAT(res->i1, Eq(data->i1));
  EXPECT_THAT(res->i2, Ne(data->i2));
}

TYPED_TEST(SerializeExtensionStdSmartPtrNonPolymorphicType, CanUseValueOverload)
{
  using Ptr = typename TestFixture::template TPtr<uint16_t>;
  using Ext = typename TestFixture::TExt;

  Ptr data{ new uint16_t{ 3 } };
  this->createSerializer().ext2b(data, Ext{});
  Ptr res{};
  this->createDeserializer().ext2b(res, Ext{});
  EXPECT_THAT(*res, Eq(*data));
}

TYPED_TEST(SerializeExtensionStdSmartPtrNonPolymorphicType,
           FirstPtrThenPointerObserver)
{
  using Ptr = typename TestFixture::template TPtr<uint16_t>;
  using Ext = typename TestFixture::TExt;

  Ptr data{ new uint16_t{ 3 } };
  uint16_t* dataObs = data.get();
  auto& ser = this->createSerializer();
  ser.ext2b(data, Ext{});
  ser.ext2b(dataObs, PointerObserver{});
  Ptr res{};
  uint16_t* resObs = nullptr;
  auto& des = this->createDeserializer();
  des.ext2b(res, Ext{});
  des.ext2b(resObs, PointerObserver{});

  EXPECT_THAT(resObs, Eq(res.get()));
}

TYPED_TEST(SerializeExtensionStdSmartPtrNonPolymorphicType,
           FirstPointerObserverThenPtr)
{
  using Ptr = typename TestFixture::template TPtr<uint16_t>;
  using Ext = typename TestFixture::TExt;

  Ptr data{ new uint16_t{ 3 } };
  uint16_t* dataObs = data.get();
  auto& ser = this->createSerializer();
  ser.ext2b(dataObs, PointerObserver{});
  ser.ext2b(data, Ext{});
  Ptr res{};
  uint16_t* resObs = nullptr;
  auto& des = this->createDeserializer();
  des.ext2b(resObs, PointerObserver{});
  des.ext2b(res, Ext{});
  EXPECT_THAT(resObs, Eq(res.get()));
}

TYPED_TEST(SerializeExtensionStdSmartPtrPolymorphicType, Data0Result0)
{
  using Ptr = typename TestFixture::template TPtr<Base>;
  using Ext = typename TestFixture::TExt;

  Ptr baseData{};
  this->createSerializer().ext(baseData, Ext{});
  Ptr baseRes{};
  this->createDeserializer().ext(baseRes, Ext{});

  EXPECT_THAT(baseRes.get(), ::testing::IsNull());
  EXPECT_THAT(baseData.get(), ::testing::IsNull());
}

TYPED_TEST(SerializeExtensionStdSmartPtrPolymorphicType, Data0Result1)
{
  using Ptr = typename TestFixture::template TPtr<Base>;
  using Ext = typename TestFixture::TExt;

  Ptr baseData{};
  this->createSerializer().ext(baseData, Ext{});

  Ptr baseRes{ new Derived{} };
  this->createDeserializer().ext(baseRes, Ext{});

  EXPECT_THAT(baseRes.get(), ::testing::IsNull());
  EXPECT_THAT(baseData.get(), ::testing::IsNull());
}

TYPED_TEST(SerializeExtensionStdSmartPtrPolymorphicType, Data1Result0)
{
  using Ptr = typename TestFixture::template TPtr<Base>;
  using Ext = typename TestFixture::TExt;

  Ptr baseData{ new Derived{ 3, 78 } };
  this->createSerializer().ext(baseData, Ext{});
  Ptr baseRes{};
  this->createDeserializer().ext(baseRes, Ext{});

  auto* data = dynamic_cast<Derived*>(baseData.get());
  auto* res = dynamic_cast<Derived*>(baseRes.get());

  EXPECT_THAT(data, ::testing::NotNull());
  EXPECT_THAT(res, ::testing::NotNull());
  EXPECT_THAT(res->x, Eq(data->x));
  EXPECT_THAT(res->y, Eq(data->y));
}

TYPED_TEST(SerializeExtensionStdSmartPtrPolymorphicType,
           DataAndResultWithDifferentRuntimeTypes)
{
  using Ptr = typename TestFixture::template TPtr<Base>;
  using Ext = typename TestFixture::TExt;

  Ptr baseData{ new Derived{ 3, 78 } };
  this->createSerializer().ext(baseData, Ext{});
  Ptr baseRes{ new Base{} };
  this->createDeserializer().ext(baseRes, Ext{});

  auto* data = dynamic_cast<Derived*>(baseData.get());
  auto* res = dynamic_cast<Derived*>(baseRes.get());

  EXPECT_THAT(data, ::testing::NotNull());
  EXPECT_THAT(res, ::testing::NotNull());
  EXPECT_THAT(res->x, Eq(data->x));
  EXPECT_THAT(res->y, Eq(data->y));
}

class SerializeExtensionStdSmartSharedPtr : public testing::Test
{
public:
  using TContext = std::tuple<PointerLinkingContext,
                              InheritanceContext,
                              PolymorphicContext<StandardRTTI>>;
  using SerContext = BasicSerializationContext<TContext>;

  // this is useful for PolymorphicContext to bind classes to
  // serializer/deserializer
  using TSerializer = typename SerContext::TSerializer;
  using TDeserializer = typename SerContext::TDeserializer;

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

  size_t getBufferSize() const { return sctx.getBufferSize(); }

  bool isPointerContextValid() { return std::get<0>(plctx).isValid(); }

  void clearSharedState() { return std::get<0>(plctx).clearSharedState(); }
};

TEST_F(SerializeExtensionStdSmartSharedPtr, SameSharedObjectIsSerializedOnce)
{

  std::shared_ptr<Base> baseData1{ new Derived{ 3, 78 } };
  std::shared_ptr<Base> baseData2{ baseData1 };
  auto& ser = createSerializer();
  ser.ext(baseData1, StdSmartPtr{});
  ser.ext(baseData1, StdSmartPtr{});
  createDeserializer();

  // 1b linking context (for 1st time)
  // 1b dynamic type info
  // 2b Derived object
  // 1b linking context (for 2nd time)
  EXPECT_THAT(getBufferSize(), Eq(5));
  EXPECT_TRUE(isPointerContextValid());
}

TEST_F(SerializeExtensionStdSmartSharedPtr,
       PointerLinkingContextCorrectlyClearSharedState)
{

  std::shared_ptr<Base> baseData1{ new Derived{ 3, 78 } };

  auto& ser = createSerializer();
  ser.ext(baseData1, StdSmartPtr{});
  std::shared_ptr<Base> baseRes1{};
  auto& des = createDeserializer();
  des.ext(baseRes1, StdSmartPtr{});
  EXPECT_THAT(baseRes1.use_count(), Eq(2));
  clearSharedState();
  EXPECT_THAT(baseRes1.use_count(), Eq(1));
  EXPECT_TRUE(isPointerContextValid());
}

TEST_F(SerializeExtensionStdSmartSharedPtr, CorrectlyManagesSameSharedObject)
{

  std::shared_ptr<Base> baseData1{ new Derived{ 3, 78 } };
  std::shared_ptr<Base> baseData2{ new Derived{ 55, 11 } };
  std::shared_ptr<Base> baseData21{ baseData2 };
  auto& ser = createSerializer();
  ser.ext(baseData1, StdSmartPtr{});
  ser.ext(baseData2, StdSmartPtr{});
  ser.ext(baseData21, StdSmartPtr{});

  std::shared_ptr<Base> baseRes1{};
  std::shared_ptr<Base> baseRes2{};
  std::shared_ptr<Base> baseRes21{};
  auto& des = createDeserializer();
  des.ext(baseRes1, StdSmartPtr{});
  des.ext(baseRes2, StdSmartPtr{});
  des.ext(baseRes21, StdSmartPtr{});

  auto* data = dynamic_cast<Derived*>(baseRes1.get());
  EXPECT_THAT(data, ::testing::NotNull());

  clearSharedState();

  EXPECT_THAT(baseRes1.use_count(), Eq(1));
  EXPECT_THAT(baseRes2.use_count(), Eq(2));
  EXPECT_THAT(baseRes21.use_count(), Eq(2));
  baseRes2.reset();
  EXPECT_THAT(baseRes21.use_count(), Eq(1));
  EXPECT_TRUE(isPointerContextValid());
}

TEST_F(SerializeExtensionStdSmartSharedPtr, FirstSharedThenWeakPtr)
{

  std::shared_ptr<Base> baseData1{ new Derived{ 3, 78 } };
  std::weak_ptr<Base> baseData11{ baseData1 };
  std::weak_ptr<Base> baseData12{ baseData11 };
  auto& ser = createSerializer();
  ser.ext(baseData1, StdSmartPtr{});
  ser.ext(baseData11, StdSmartPtr{});
  ser.ext(baseData12, StdSmartPtr{});

  std::shared_ptr<Base> baseRes1{};
  std::weak_ptr<Base> baseRes11{};
  std::weak_ptr<Base> baseRes12{};
  auto& des = createDeserializer();
  des.ext(baseRes1, StdSmartPtr{});
  des.ext(baseRes11, StdSmartPtr{});
  des.ext(baseRes12, StdSmartPtr{});

  auto* data = dynamic_cast<Derived*>(baseRes1.get());
  EXPECT_THAT(data, ::testing::NotNull());

  clearSharedState();

  EXPECT_THAT(baseRes1.use_count(), Eq(1));
  EXPECT_THAT(baseRes11.use_count(), Eq(1));
  EXPECT_THAT(baseRes12.use_count(), Eq(1));
  baseRes1.reset();
  EXPECT_THAT(baseRes11.use_count(), Eq(0));
  EXPECT_TRUE(isPointerContextValid());
}

TEST_F(SerializeExtensionStdSmartSharedPtr, FirstWeakThenSharedPtr)
{

  std::shared_ptr<MyStruct1> baseData1{ new MyStruct1{ 3, 78 } };
  std::weak_ptr<MyStruct1> baseData11{ baseData1 };
  std::weak_ptr<MyStruct1> baseData2{};
  auto& ser = createSerializer();
  ser.ext(baseData2, StdSmartPtr{});
  ser.ext(baseData11, StdSmartPtr{});
  ser.ext(baseData1, StdSmartPtr{});

  std::shared_ptr<MyStruct1> baseRes1{};
  std::weak_ptr<MyStruct1> baseRes11{};
  std::weak_ptr<MyStruct1> baseRes2{};
  auto& des = createDeserializer();
  des.ext(baseRes2, StdSmartPtr{});
  des.ext(baseRes11, StdSmartPtr{});
  des.ext(baseRes1, StdSmartPtr{});

  clearSharedState();

  EXPECT_THAT(*baseData1, Eq(*baseRes1));
  EXPECT_THAT(baseRes1.use_count(), Eq(1));
  EXPECT_THAT(baseRes2.use_count(), Eq(0));
  EXPECT_THAT(baseRes11.use_count(), Eq(1));
  baseRes1.reset();
  EXPECT_THAT(baseRes11.use_count(), Eq(0));
  EXPECT_TRUE(isPointerContextValid());
}

TEST_F(SerializeExtensionStdSmartSharedPtr, WeakPtrFirstPolymorphicData0Result1)
{

  std::shared_ptr<Base> baseData1{};
  std::weak_ptr<Base> baseData2{};
  auto& ser = createSerializer();
  ser.ext(baseData2, StdSmartPtr{});
  ser.ext(baseData1, StdSmartPtr{});

  std::shared_ptr<Base> baseRes1{ new Base{} };
  std::weak_ptr<Base> baseRes2{ baseRes1 };
  auto& des = createDeserializer();
  des.ext(baseRes2, StdSmartPtr{});
  des.ext(baseRes1, StdSmartPtr{});

  clearSharedState();

  EXPECT_THAT(baseRes1.use_count(), Eq(0));
  EXPECT_THAT(baseRes2.use_count(), Eq(0));
  baseRes1.reset();
  EXPECT_TRUE(isPointerContextValid());
}

TEST_F(SerializeExtensionStdSmartSharedPtr,
       WeakPtrFirstNonPolymorphicData0Result1)
{

  std::shared_ptr<MyStruct2> baseData1{};
  std::weak_ptr<MyStruct2> baseData2{};
  auto& ser = createSerializer();
  ser.ext(baseData2, StdSmartPtr{});
  ser.ext(baseData1, StdSmartPtr{});

  std::shared_ptr<MyStruct2> baseRes1{ new MyStruct2{ MyStruct2::MyEnum::V4,
                                                      { 1, 87 } } };
  std::weak_ptr<MyStruct2> baseRes2{ baseRes1 };
  auto& des = createDeserializer();
  des.ext(baseRes2, StdSmartPtr{});
  des.ext(baseRes1, StdSmartPtr{});

  clearSharedState();

  EXPECT_THAT(baseRes1.use_count(), Eq(0));
  EXPECT_THAT(baseRes2.use_count(), Eq(0));
  baseRes1.reset();
  EXPECT_TRUE(isPointerContextValid());
}

TEST_F(SerializeExtensionStdSmartSharedPtr, FewPtrsAreEmpty)
{

  std::shared_ptr<Base> baseData1{ new Derived{ 3, 78 } };
  std::shared_ptr<Base> baseData2{};
  std::weak_ptr<Base> baseData3{};
  std::weak_ptr<Base> baseData11{ baseData1 };
  auto& ser = createSerializer();
  ser.ext(baseData1, StdSmartPtr{});
  ser.ext(baseData2, StdSmartPtr{});
  ser.ext(baseData3, StdSmartPtr{});
  ser.ext(baseData11, StdSmartPtr{});

  std::shared_ptr<Base> baseRes1{};
  std::shared_ptr<Base> baseRes2{ new Derived{ 3, 78 } };
  std::weak_ptr<Base> baseRes3{ baseRes2 };
  std::weak_ptr<Base> baseRes11{};
  auto& des = createDeserializer();
  des.ext(baseRes1, StdSmartPtr{});
  des.ext(baseRes2, StdSmartPtr{});
  des.ext(baseRes3, StdSmartPtr{});
  des.ext(baseRes11, StdSmartPtr{});

  clearSharedState();

  EXPECT_THAT(baseRes1.use_count(), Eq(1));
  EXPECT_THAT(baseRes2.use_count(), Eq(0));
  EXPECT_THAT(baseRes3.use_count(), Eq(0));
  EXPECT_THAT(baseRes11.use_count(), Eq(1));
  baseRes1.reset();
  EXPECT_THAT(baseRes11.use_count(), Eq(0));
  EXPECT_TRUE(isPointerContextValid());
}

TEST_F(SerializeExtensionStdSmartSharedPtr, WhenResultObjectExistsSameType)
{

  std::shared_ptr<Base> baseData1{ new Derived{ 3, 78 } };
  auto& ser = createSerializer();
  ser.ext(baseData1, StdSmartPtr{});

  std::shared_ptr<Base> baseRes1{ new Derived{ 0, 0 } };
  auto& des = createDeserializer();
  des.ext(baseRes1, StdSmartPtr{});

  clearSharedState();

  EXPECT_THAT(baseRes1.use_count(), Eq(1));
  EXPECT_THAT(baseRes1->x, Eq(baseData1->x));
  EXPECT_TRUE(isPointerContextValid());
}

TEST_F(SerializeExtensionStdSmartSharedPtr, WhenResultObjectExistsDifferentType)
{

  std::shared_ptr<Base> baseData1{ new Derived{ 3, 78 } };
  auto& ser = createSerializer();
  ser.ext(baseData1, StdSmartPtr{});

  std::shared_ptr<Base> baseRes1{ new Base{} };
  auto& des = createDeserializer();
  des.ext(baseRes1, StdSmartPtr{});

  clearSharedState();

  EXPECT_THAT(baseRes1.use_count(), Eq(1));
  EXPECT_THAT(baseRes1->x, Eq(baseData1->x));
  EXPECT_THAT(dynamic_cast<Derived*>(baseRes1.get()), ::testing::NotNull());
  EXPECT_TRUE(isPointerContextValid());
}

TEST_F(SerializeExtensionStdSmartSharedPtr,
       WhenOnlyWeakPtrIsSerializedThenPointerCointextIsInvalid)
{
  std::shared_ptr<Base> tmp{ new Derived{ 3, 78 } };
  std::weak_ptr<Base> baseData1{ tmp };
  auto& ser = createSerializer();
  ser.ext(baseData1, StdSmartPtr{});

  EXPECT_FALSE(isPointerContextValid());
}

TEST_F(SerializeExtensionStdSmartSharedPtr,
       WhenOnlyWeakPtrIsDeserializedThenPointerCointextIsInvalid)
{
  std::shared_ptr<Base> baseData1{ new Derived{ 3, 78 } };
  auto& ser = createSerializer();
  ser.ext(baseData1, StdSmartPtr{});

  std::weak_ptr<Base> baseRes1{};
  auto& des = createDeserializer();
  des.ext(baseRes1, StdSmartPtr{});

  EXPECT_FALSE(isPointerContextValid());

  EXPECT_THAT(baseRes1.use_count(), Eq(1));
  clearSharedState();
  EXPECT_THAT(baseRes1.use_count(), Eq(0));
}

struct TestSharedFromThis
  : public std::enable_shared_from_this<TestSharedFromThis>
{
  float x{};

  explicit TestSharedFromThis()
    : std::enable_shared_from_this<TestSharedFromThis>()
  {
  }

  template<typename S>
  void serialize(S& s)
  {
    s.value4b(x);
  }
};

TEST_F(SerializeExtensionStdSmartSharedPtr, EnableSharedFromThis)
{
  std::shared_ptr<TestSharedFromThis> dataPtr(new TestSharedFromThis{});
  std::shared_ptr<TestSharedFromThis> resPtr{};
  createSerializer().ext(dataPtr, StdSmartPtr{});
  createDeserializer().ext(resPtr, StdSmartPtr{});
  clearSharedState();
  auto resPtr2 = resPtr->shared_from_this();
  EXPECT_THAT(resPtr->x, Eq(dataPtr->x));
  EXPECT_THAT(resPtr2.use_count(), Eq(2));
}

struct CustomDeleter
{
  void operator()(Base* p) { delete p; }
};

class SerializeExtensionStdSmartUniquePtr
  : public SerializeExtensionStdSmartSharedPtr
{};

TEST_F(SerializeExtensionStdSmartUniquePtr, WithCustomDeleter)
{
  std::unique_ptr<Base, CustomDeleter> dataPtr(new Derived{ 87, 7 });
  std::unique_ptr<Base, CustomDeleter> resPtr{};
  createSerializer().ext(dataPtr, StdSmartPtr{});
  createDeserializer().ext(resPtr, StdSmartPtr{});
  clearSharedState();
  EXPECT_THAT(resPtr->x, Eq(dataPtr->x));
  EXPECT_THAT(dynamic_cast<Derived*>(resPtr.get()), ::testing::NotNull());
}
