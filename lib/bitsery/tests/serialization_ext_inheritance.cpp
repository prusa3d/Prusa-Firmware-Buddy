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
#include <bitsery/ext/inheritance.h>
#include <gmock/gmock.h>

using bitsery::ext::BaseClass;
using bitsery::ext::VirtualBaseClass;

using SerContext = BasicSerializationContext<bitsery::ext::InheritanceContext>;

using testing::Eq;

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

/*
 * non virtual inheritance from base
 */
struct Derive1NonVirtually : Base
{
  uint8_t y1{};
};

template<typename S>
void
serialize(S& s, Derive1NonVirtually& o)
{
  s.ext(o, BaseClass<Base>{});
  s.value1b(o.y1);
}

struct Derive2NonVirtually : Base
{
  uint8_t y2{};
};

template<typename S>
void
serialize(S& s, Derive2NonVirtually& o)
{
  // use lambda to serialize base
  s.ext(o, BaseClass<Base>{}, [](S& s, Base& b) { s.object(b); });
  s.value1b(o.y2);
}

struct MultipleInheritanceNonVirtualBase
  : Derive1NonVirtually
  , Derive2NonVirtually
{
  uint8_t z{};
};

template<typename S>
void
serialize(S& s, MultipleInheritanceNonVirtualBase& o)
{
  s.ext(o, BaseClass<Derive1NonVirtually>{});
  s.ext(o, BaseClass<Derive2NonVirtually>{});
  s.value1b(o.z);
}

/*
 * virtual inheritance from base
 */
struct Derive1Virtually : virtual Base
{
  uint8_t y1{};
};

template<typename S>
void
serialize(S& s, Derive1Virtually& o)
{
  s.ext(o, VirtualBaseClass<Base>{});
  s.value1b(o.y1);
}

struct Derive2Virtually : virtual Base
{
  uint8_t y2{};
};

template<typename S>
void
serialize(S& s, Derive2Virtually& o)
{
  s.ext(o, VirtualBaseClass<Base>{});
  s.value1b(o.y2);
}

struct MultipleInheritanceVirtualBase
  : Derive1Virtually
  , Derive2Virtually
{
  uint8_t z{};
  MultipleInheritanceVirtualBase() = default;

  MultipleInheritanceVirtualBase(uint8_t x_,
                                 uint8_t y1_,
                                 uint8_t y2_,
                                 uint8_t z_)
  {
    x = x_;
    y1 = y1_;
    y2 = y2_;
    z = z_;
  }

  template<typename S>
  void serialize(S& s)
  {
    s.ext(*this, BaseClass<Derive1Virtually>{});
    s.ext(*this, BaseClass<Derive2Virtually>{});
    s.value1b(z);
  }
};

bool
operator==(const MultipleInheritanceVirtualBase& lhs,
           const MultipleInheritanceVirtualBase& rhs)
{
  return std::tie(lhs.x, lhs.y1, lhs.y2, lhs.z) ==
         std::tie(rhs.x, rhs.y1, rhs.y2, rhs.z);
}

TEST(SerializeExtensionInheritance, BaseClass)
{

  Derive1NonVirtually d1{};
  d1.x = 187;
  d1.y1 = 74;
  Derive1NonVirtually rd1{};

  SerContext ctx{};
  bitsery::ext::InheritanceContext inherCtxSer{};
  bitsery::ext::InheritanceContext inherCtxDes{};
  ctx.createSerializer(inherCtxSer).object(d1);
  ctx.createDeserializer(inherCtxDes).object(rd1);

  EXPECT_THAT(rd1.x, Eq(d1.x));
  EXPECT_THAT(rd1.y1, Eq(d1.y1));
  EXPECT_THAT(ctx.getBufferSize(), Eq(2));
}

TEST(SerializeExtensionInheritance, VirtualBaseClass)
{
  Derive1Virtually d1{};
  d1.x = 15;
  d1.y1 = 87;
  Derive1Virtually rd1{};

  SerContext ctx{};
  bitsery::ext::InheritanceContext inherCtxSer{};
  bitsery::ext::InheritanceContext inherCtxDes{};
  ctx.createSerializer(inherCtxSer).object(d1);
  ctx.createDeserializer(inherCtxDes).object(rd1);

  EXPECT_THAT(rd1.x, Eq(d1.x));
  EXPECT_THAT(rd1.y1, Eq(d1.y1));
  EXPECT_THAT(ctx.getBufferSize(), Eq(2));
}

TEST(SerializeExtensionInheritance, MultipleBasesWithoutVirtualInheritance)
{
  MultipleInheritanceNonVirtualBase md{};
  // x is ambiguous because we don't derive virtually
  static_cast<Derive1NonVirtually&>(md).x = 1;
  static_cast<Derive2NonVirtually&>(md).x = 2;
  md.y1 = 4;
  md.z = 5;
  md.y2 = 6;
  MultipleInheritanceNonVirtualBase res{};

  SerContext ctx{};
  bitsery::ext::InheritanceContext inherCtxSer{};
  bitsery::ext::InheritanceContext inherCtxDes{};
  ctx.createSerializer(inherCtxSer).object(md);
  ctx.createDeserializer(inherCtxDes).object(res);

  EXPECT_THAT(static_cast<Derive1NonVirtually&>(res).x,
              Eq(static_cast<Derive1NonVirtually&>(md).x));
  EXPECT_THAT(static_cast<Derive2NonVirtually&>(res).x,
              Eq(static_cast<Derive2NonVirtually&>(md).x));
  EXPECT_THAT(res.y1, Eq(md.y1));
  EXPECT_THAT(res.y2, Eq(md.y2));
  EXPECT_THAT(res.z, Eq(md.z));
  EXPECT_THAT(ctx.getBufferSize(), Eq(5)); // 5 because two bases
}

TEST(SerializeExtensionInheritance,
     WhenNoVirtualInheritanceExistsThenInheritanceContextIsNotRequired)
{
  MultipleInheritanceNonVirtualBase md{};
  // x is ambiguous because we don't derive virtually
  static_cast<Derive1NonVirtually&>(md).x = 1;
  static_cast<Derive2NonVirtually&>(md).x = 2;
  md.y1 = 4;
  md.z = 5;
  md.y2 = 6;
  MultipleInheritanceNonVirtualBase res{};

  // without InheritanceContext
  SerializationContext ctx{};
  ctx.createSerializer().object(md);
  ctx.createDeserializer().object(res);

  EXPECT_THAT(static_cast<Derive1NonVirtually&>(res).x,
              Eq(static_cast<Derive1NonVirtually&>(md).x));
  EXPECT_THAT(static_cast<Derive2NonVirtually&>(res).x,
              Eq(static_cast<Derive2NonVirtually&>(md).x));
  EXPECT_THAT(res.y1, Eq(md.y1));
  EXPECT_THAT(res.y2, Eq(md.y2));
  EXPECT_THAT(res.z, Eq(md.z));
  EXPECT_THAT(ctx.getBufferSize(), Eq(5)); // 5 because two bases
}

TEST(SerializeExtensionInheritance, MultipleBasesWithVirtualInheritance)
{
  MultipleInheritanceVirtualBase md{ 3, 7, 5, 15 };
  MultipleInheritanceVirtualBase res{};

  SerContext ctx{};
  bitsery::ext::InheritanceContext inherCtxSer{};
  bitsery::ext::InheritanceContext inherCtxDes{};
  ctx.createSerializer(inherCtxSer).object(md);
  ctx.createDeserializer(inherCtxDes).object(res);
  EXPECT_THAT(res, Eq(md));
  EXPECT_THAT(ctx.getBufferSize(), Eq(4)); // 4 because virtual base
}

TEST(SerializeExtensionInheritance,
     MultipleBasesWithVirtualInheritanceMultipleObjects)
{
  std::vector<MultipleInheritanceVirtualBase> data;
  data.emplace_back(4, 8, 7, 9);
  data.emplace_back(1, 2, 3, 4);
  data.emplace_back(8, 7, 15, 97);
  data.emplace_back(54, 132, 45, 84);
  data.emplace_back(27, 85, 41, 2);
  std::vector<MultipleInheritanceVirtualBase> res{};

  SerContext ctx{};
  bitsery::ext::InheritanceContext inherCtxSer{};
  bitsery::ext::InheritanceContext inherCtxDes{};
  ctx.createSerializer(inherCtxSer).container(data, 10);
  ctx.createDeserializer(inherCtxDes).container(res, 10);
  EXPECT_THAT(res, ::testing::ContainerEq(data));
  EXPECT_THAT(
    ctx.getBufferSize(),
    Eq(1 +
       4 *
         data.size())); // 1 container size + 4 because virtual base * elements
}

//
class BasePrivateSerialize
{
public:
  explicit BasePrivateSerialize(uint8_t v)
    : _v{ v }
  {
  }
  uint8_t getX() const { return _v; }

private:
  uint8_t _v;

  friend bitsery::Access;
  template<typename S>
  void serialize(S& s)
  {
    s.value1b(_v);
  }
};

class DerivedPrivateBase : public BasePrivateSerialize
{
public:
  explicit DerivedPrivateBase(uint8_t v)
    : BasePrivateSerialize(v)
  {
  }
  uint8_t z{};
};

template<typename S>
void
serialize(S& s, DerivedPrivateBase& o)
{
  // use lambda for base serialization
  s.ext(o,
        BaseClass<BasePrivateSerialize>{},
        [](S& s, BasePrivateSerialize& b) { s.object(b); });
  s.value1b(o.z);
}

struct BaseNonMemberSerialize
{
  uint8_t x{};
};

template<typename S>
void
serialize(S& s, BaseNonMemberSerialize& o)
{
  s.value1b(o.x);
}

struct DerivedMemberSerialize : public BaseNonMemberSerialize
{
  uint8_t z{};
  template<typename S>
  void serialize(S& s)
  {
    s.ext(*this, BaseClass<BaseNonMemberSerialize>{});
    s.value1b(z);
  }
};

// explicitly select serialize functions, for types that has ambiguous serialize
// functions
namespace bitsery {
template<>
struct SelectSerializeFnc<DerivedPrivateBase> : UseNonMemberFnc
{};

template<>
struct SelectSerializeFnc<DerivedMemberSerialize> : UseMemberFnc
{};
}

TEST(
  SerializeExtensionInheritance,
  WhenDerivedClassHasAmbiguousSerializeFunctionThenExplicitlySelectSpecialization)
{
  DerivedPrivateBase data1{ 43 };
  data1.z = 87;
  DerivedMemberSerialize data2{};
  data2.x = 71;
  data2.z = 22;
  DerivedPrivateBase res1{ 0 };
  DerivedMemberSerialize res2{};

  SerContext ctx{};
  bitsery::ext::InheritanceContext inherCtxSer{};
  bitsery::ext::InheritanceContext inherCtxDes{};
  ctx.createSerializer(inherCtxSer).object(data1);
  ctx.createSerializer(inherCtxSer).object(data2);
  ctx.createDeserializer(inherCtxDes).object(res1);
  ctx.createDeserializer(inherCtxDes).object(res2);
  EXPECT_THAT(res1.getX(), Eq(data1.getX()));
  EXPECT_THAT(res1.z, Eq(data1.z));
  EXPECT_THAT(res2.x, Eq(data2.x));
  EXPECT_THAT(res2.z, Eq(data2.z));
}

struct AbstractBase
{
  uint8_t x{};
  virtual void exec() = 0;
  virtual ~AbstractBase() = default;

  template<typename S>
  void serialize(S& s)
  {
    s.value1b(x);
  }
};

struct ImplementedBase : AbstractBase
{
  uint8_t y{};
  void exec() override {}

  template<typename S>
  void serialize(S& s)
  {
    s.ext(*this, BaseClass<AbstractBase>{});
    s.value1b(y);
  }
};

TEST(SerializeExtensionInheritance, CanSerializeAbstractClass)
{
  ImplementedBase data{};
  data.x = 4;
  data.y = 2;
  data.exec();
  ImplementedBase res{};
  SerContext ctx{};
  bitsery::ext::InheritanceContext inherCtxSer{};
  bitsery::ext::InheritanceContext inherCtxDes{};
  ctx.createSerializer(inherCtxSer).object(data);
  ctx.createDeserializer(inherCtxDes).object(res);
  EXPECT_THAT(res.x, Eq(data.x));
  EXPECT_THAT(res.y, Eq(data.y));
}
