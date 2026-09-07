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

#include <bitsery/ext/inheritance.h>
#include <bitsery/ext/pointer.h>
#include <bitsery/ext/std_smart_ptr.h>

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
using bitsery::ext::StdSmartPtr;

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
  Base() = default;

  explicit Base(uint64_t v)
    : x{ v }
  {
  }

  uint64_t x{};

  virtual ~Base() = default;
};

template<typename S>
void
serialize(S& s, Base& o)
{
  s.value8b(o.x);
}

struct Derived1 : Base
{
  Derived1() = default;

  Derived1(uint64_t x_, uint64_t y_)
    : Base{ x_ }
    , y1{ y_ }
  {
  }

  friend bool operator==(const Derived1& lhs, const Derived1& rhs)
  {
    return lhs.x == rhs.x && lhs.y1 == rhs.y1;
  }

  uint64_t y1{};
};

template<typename S>
void
serialize(S& s, Derived1& o)
{
  s.ext(o, BaseClass<Base>{});
  s.value8b(o.y1);
}

struct Derived2 : Base
{
  uint64_t y1{};
  uint64_t y2{};
};

template<typename S>
void
serialize(S& s, Derived2& o)
{
  s.ext(o, BaseClass<Base>{});
  s.value8b(o.y1);
  s.value8b(o.y2);
}

// polymorphic structure that contains polymorphic pointer, to test memory
// resource propagation
struct PolyPtrWithPolyPtrBase
{
  std::unique_ptr<Base> ptr{};

  virtual ~PolyPtrWithPolyPtrBase() = default;
};

template<typename S>
void
serialize(S& s, PolyPtrWithPolyPtrBase& o)
{
  s.ext(o.ptr, StdSmartPtr{});
}

struct DerivedPolyPtrWithPolyPtr : PolyPtrWithPolyPtrBase
{};

template<typename S>
void
serialize(S& s, DerivedPolyPtrWithPolyPtr& o)
{
  s.ext(o.ptr, StdSmartPtr{});
}

// define relationships between base class and derived classes for runtime
// polymorphism

namespace bitsery {
namespace ext {

template<>
struct PolymorphicBaseClass<Base>
  : PolymorphicDerivedClasses<Derived1, Derived2>
{};

template<>
struct PolymorphicBaseClass<PolyPtrWithPolyPtrBase>
  : PolymorphicDerivedClasses<DerivedPolyPtrWithPolyPtr>
{};

}
}

// this class is for testing
struct TestAllocInfo
{
  void* ptr;
  size_t bytes;
  size_t alignment;
  size_t typeId;

  friend bool operator==(const TestAllocInfo& lhs, const TestAllocInfo& rhs)
  {
    return std::tie(lhs.ptr, lhs.bytes, lhs.alignment, lhs.typeId) ==
           std::tie(rhs.ptr, rhs.bytes, rhs.alignment, rhs.typeId);
  }
};

struct MemResourceForTest : public bitsery::ext::MemResourceBase
{

  void* allocate(size_t bytes, size_t alignment, size_t typeId) override
  {
    const auto res =
      bitsery::ext::MemResourceNewDelete{}.allocate(bytes, alignment, typeId);
    allocs.push_back({ res, bytes, alignment, typeId });
    return res;
  }

  void deallocate(void* ptr,
                  size_t bytes,
                  size_t alignment,
                  size_t typeId) noexcept override
  {
    deallocs.push_back({ ptr, bytes, alignment, typeId });
    bitsery::ext::MemResourceNewDelete{}.deallocate(
      ptr, bytes, alignment, typeId);
  }

  std::vector<TestAllocInfo> allocs{};
  std::vector<TestAllocInfo> deallocs{};
};

class SerializeExtensionPointerWithAllocator : public testing::Test
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
      bitsery::ext::PolymorphicClassesList<Base, PolyPtrWithPolyPtrBase>{});
    return res;
  }

  typename SerContext::TDeserializer& createDeserializer()
  {
    auto& res = sctx.createDeserializer(plctx);
    std::get<2>(plctx).clear();
    // bind deserializer with classes
    std::get<2>(plctx).registerBasesList<SerContext::TDeserializer>(
      bitsery::ext::PolymorphicClassesList<Base, PolyPtrWithPolyPtrBase>{});
    return res;
  }

  bool isPointerContextValid() { return std::get<0>(plctx).isValid(); }

  virtual void TearDown() override { EXPECT_TRUE(isPointerContextValid()); }
};

TEST_F(SerializeExtensionPointerWithAllocator,
       CanSetDefaultMemoryResourceInPointerLinkingContext)
{

  MemResourceForTest memRes{};
  std::get<0>(plctx).setMemResource(&memRes);

  Base* baseData = new Derived1{ 2, 1 };
  createSerializer().ext(baseData, PointerOwner{});
  Base* baseRes = nullptr;
  createDeserializer().ext(baseRes, PointerOwner{});

  auto dData = dynamic_cast<Derived1*>(baseData);
  auto dRes = dynamic_cast<Derived1*>(baseRes);

  EXPECT_THAT(dRes, ::testing::NotNull());
  EXPECT_THAT(*dData, *dRes);
  EXPECT_THAT(memRes.allocs.size(), Eq(1u));
  EXPECT_THAT(memRes.allocs[0].bytes, Eq(sizeof(Derived1)));
  EXPECT_THAT(memRes.allocs[0].alignment, Eq(alignof(Derived1)));
  EXPECT_THAT(memRes.allocs[0].typeId,
              Eq(bitsery::ext::StandardRTTI::get<Derived1>()));
  EXPECT_THAT(memRes.deallocs.size(), Eq(0u));
  delete dData;
  delete dRes;
}

TEST_F(SerializeExtensionPointerWithAllocator,
       CorrectlyDeallocatesPreviousInstance)
{
  MemResourceForTest memRes{};
  std::get<0>(plctx).setMemResource(&memRes);

  Base* baseData = new Derived1{ 2, 1 };
  createSerializer().ext(baseData, PointerOwner{});
  Base* baseRes = new Derived2;
  createDeserializer().ext(baseRes, PointerOwner{});

  auto dData = dynamic_cast<Derived1*>(baseData);
  auto dRes = dynamic_cast<Derived1*>(baseRes);

  EXPECT_THAT(dRes, ::testing::NotNull());
  EXPECT_THAT(*dData, *dRes);
  EXPECT_THAT(memRes.allocs.size(), Eq(1u));
  EXPECT_THAT(memRes.allocs[0].bytes, Eq(sizeof(Derived1)));
  EXPECT_THAT(memRes.allocs[0].alignment, Eq(alignof(Derived1)));
  EXPECT_THAT(memRes.allocs[0].typeId,
              Eq(bitsery::ext::StandardRTTI::get<Derived1>()));
  EXPECT_THAT(memRes.deallocs.size(), Eq(1u));
  EXPECT_THAT(memRes.deallocs[0].bytes, Eq(sizeof(Derived2)));
  EXPECT_THAT(memRes.deallocs[0].alignment, Eq(alignof(Derived2)));
  EXPECT_THAT(memRes.deallocs[0].typeId,
              Eq(bitsery::ext::StandardRTTI::get<Derived2>()));

  delete dData;
  delete dRes;
}

TEST_F(SerializeExtensionPointerWithAllocator,
       DefaultDeleterIsNotUsedForStdUniquePtr)
{
  MemResourceForTest memRes{};
  std::get<0>(plctx).setMemResource(&memRes);

  std::unique_ptr<Base> baseData{};
  createSerializer().ext(baseData, StdSmartPtr{});
  auto baseRes = std::unique_ptr<Base>(new Derived1{ 45, 64 });
  createDeserializer().ext(baseRes, StdSmartPtr{});

  EXPECT_THAT(memRes.allocs.size(), Eq(0u));
  EXPECT_THAT(memRes.deallocs.size(), Eq(1u));
  EXPECT_THAT(memRes.deallocs[0].bytes, Eq(sizeof(Derived1)));
  EXPECT_THAT(memRes.deallocs[0].alignment, Eq(alignof(Derived1)));
  EXPECT_THAT(memRes.deallocs[0].typeId,
              Eq(bitsery::ext::StandardRTTI::get<Derived1>()));
}

struct CustomBaseDeleter
{
  void operator()(Base* obj) { delete obj; }
};

TEST_F(SerializeExtensionPointerWithAllocator,
       CustomDeleterIsNotUsedForStdUniquePtr)
{
  MemResourceForTest memRes{};
  std::get<0>(plctx).setMemResource(&memRes);

  std::unique_ptr<Base, CustomBaseDeleter> baseData{};
  createSerializer().ext(baseData, StdSmartPtr{});
  auto baseRes =
    std::unique_ptr<Base, CustomBaseDeleter>(new Derived1{ 45, 64 });
  createDeserializer().ext(baseRes, StdSmartPtr{});

  EXPECT_THAT(memRes.allocs.size(), Eq(0u));
  EXPECT_THAT(memRes.deallocs.size(), Eq(1u));
  EXPECT_THAT(memRes.deallocs[0].bytes, Eq(sizeof(Derived1)));
  EXPECT_THAT(memRes.deallocs[0].alignment, Eq(alignof(Derived1)));
  EXPECT_THAT(memRes.deallocs[0].typeId,
              Eq(bitsery::ext::StandardRTTI::get<Derived1>()));
}

TEST_F(SerializeExtensionPointerWithAllocator, CanSetMemResourcePerPointer)
{
  MemResourceForTest memRes1{};
  MemResourceForTest memRes2{};
  std::get<0>(plctx).setMemResource(&memRes1);

  Base* baseData = new Derived1{ 2, 1 };
  createSerializer().ext(
    baseData, PointerOwner{ bitsery::ext::PointerType::Nullable, &memRes2 });
  Base* baseRes = new Derived2;
  createDeserializer().ext(
    baseRes, PointerOwner{ bitsery::ext::PointerType::Nullable, &memRes2 });

  auto dData = dynamic_cast<Derived1*>(baseData);
  auto dRes = dynamic_cast<Derived1*>(baseRes);

  EXPECT_THAT(dRes, ::testing::NotNull());
  EXPECT_THAT(*dData, *dRes);
  EXPECT_THAT(memRes1.allocs.size(), Eq(0u));
  EXPECT_THAT(memRes1.deallocs.size(), Eq(0u));
  EXPECT_THAT(memRes2.allocs.size(), Eq(1u));
  EXPECT_THAT(memRes2.allocs[0].bytes, Eq(sizeof(Derived1)));
  EXPECT_THAT(memRes2.allocs[0].alignment, Eq(alignof(Derived1)));
  EXPECT_THAT(memRes2.allocs[0].typeId,
              Eq(bitsery::ext::StandardRTTI::get<Derived1>()));
  EXPECT_THAT(memRes2.deallocs.size(), Eq(1u));
  EXPECT_THAT(memRes2.deallocs[0].bytes, Eq(sizeof(Derived2)));
  EXPECT_THAT(memRes2.deallocs[0].alignment, Eq(alignof(Derived2)));
  EXPECT_THAT(memRes2.deallocs[0].typeId,
              Eq(bitsery::ext::StandardRTTI::get<Derived2>()));

  delete dData;
  delete dRes;
}

TEST_F(SerializeExtensionPointerWithAllocator,
       MemResourceSetPerPointerByDefaultDoNotPropagate)
{
  MemResourceForTest memRes1{};
  MemResourceForTest memRes2{};
  std::get<0>(plctx).setMemResource(&memRes1);

  auto data =
    std::unique_ptr<PolyPtrWithPolyPtrBase>(new PolyPtrWithPolyPtrBase{});
  data->ptr = std::unique_ptr<Base>(new Derived1{ 5, 6 });
  createSerializer().ext(
    data, StdSmartPtr{ bitsery::ext::PointerType::Nullable, &memRes2 });

  auto res =
    std::unique_ptr<PolyPtrWithPolyPtrBase>(new DerivedPolyPtrWithPolyPtr{});
  res->ptr = std::unique_ptr<Base>(new Derived2{});
  createDeserializer().ext(
    res, StdSmartPtr{ bitsery::ext::PointerType::Nullable, &memRes2 });

  EXPECT_THAT(memRes1.allocs.size(), Eq(1u));
  // Base* was destroyed by unique_ptr on PolyPtrWithPolyPtrBase destructor,
  // hence == 0
  EXPECT_THAT(memRes1.deallocs.size(), Eq(0u));

  EXPECT_THAT(memRes2.allocs.size(), Eq(1u));
  EXPECT_THAT(memRes2.deallocs.size(), Eq(1u));
}

TEST_F(SerializeExtensionPointerWithAllocator,
       MemResourceSetPerPointerCanPropagate)
{
  MemResourceForTest memRes1{};
  MemResourceForTest memRes2{};
  std::get<0>(plctx).setMemResource(&memRes1);

  auto data =
    std::unique_ptr<PolyPtrWithPolyPtrBase>(new PolyPtrWithPolyPtrBase{});
  data->ptr = std::unique_ptr<Base>(new Derived1{ 5, 6 });
  createSerializer().ext(
    data, StdSmartPtr{ bitsery::ext::PointerType::Nullable, &memRes2, true });

  auto res =
    std::unique_ptr<PolyPtrWithPolyPtrBase>(new DerivedPolyPtrWithPolyPtr{});
  res->ptr = std::unique_ptr<Base>(new Derived2{});
  createDeserializer().ext(
    res, StdSmartPtr{ bitsery::ext::PointerType::Nullable, &memRes2, true });

  EXPECT_THAT(memRes1.allocs.size(), Eq(0u));
  EXPECT_THAT(memRes1.deallocs.size(), Eq(0u));
  EXPECT_THAT(memRes2.allocs.size(), Eq(2u));
  // deallocates are actually == 1, because when we destroy
  // PolyPtrWithPolyPtrBase it also destroys Base because it is managed by
  // unique_ptr. in order to do it correctly we should always use custom deleter
  // for structures with nested pointers
  EXPECT_THAT(memRes2.deallocs.size(), Eq(1u));
}
