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
#include <bitsery/ext/pointer.h>
#include <gmock/gmock.h>

using bitsery::ext::PointerLinkingContext;
using bitsery::ext::PointerObserver;
using bitsery::ext::PointerOwner;
using bitsery::ext::PointerType;
using bitsery::ext::ReferencedByPointer;

using testing::Eq;

using SerContext = BasicSerializationContext<PointerLinkingContext>;

class SerializeExtensionPointerSerialization : public testing::Test
{
public:
  // data used for serialization
  int16_t d1{ 1597 };
  int16_t* pd1 = &d1;
  MyEnumClass d2{ MyEnumClass::E2 };
  MyEnumClass* pd2 = &d2;
  MyStruct1 d3{ 184, 897 };
  MyStruct1* pd3 = &d3;

  // data used for deserialization
  int16_t r1{ -84 };
  int16_t* pr1 = &r1;
  MyEnumClass r2{ MyEnumClass::E4 };
  MyEnumClass* pr2 = &r2;
  MyStruct1 r3{ -4984, -14597 };
  MyStruct1* pr3 = &r3;

  // null pointers
  int16_t* p1null = nullptr;
  MyEnumClass* p2null = nullptr;
  MyStruct1* p3null = nullptr;

  PointerLinkingContext plctx1{};
  SerContext sctx1{};

  typename SerContext::TSerializer& createSerializer()
  {
    return sctx1.createSerializer(plctx1);
  }

  typename SerContext::TDeserializer& createDeserializer()
  {
    return sctx1.createDeserializer(plctx1);
  }

  bool isPointerContextValid() { return plctx1.isValid(); }
};

TEST(SerializeExtensionPointer, RequiresPointerLinkingContext)
{
  MyStruct1* data = nullptr;
  // linking context
  PointerLinkingContext plctx1{};
  SerContext sctx1;
  sctx1.createSerializer(plctx1).ext(data, PointerOwner{});
  sctx1.createDeserializer(plctx1).ext(data, PointerOwner{});

  // linking context in tuple
  using ContextInTuple = std::tuple<int, PointerLinkingContext, float, char>;
  ContextInTuple plctx2(0, PointerLinkingContext{}, 0.0f, 'a');
  BasicSerializationContext<ContextInTuple> sctx2;
  sctx2.createSerializer(plctx2).ext(data, PointerObserver{});
  sctx2.createDeserializer(plctx2).ext(data, PointerObserver{});
}

TEST(SerializeExtensionPointer,
     PointerLinkingContextAcceptsMultipleSharedOwnersAndReturnSameId)
{
  MyStruct1 data{};
  // pretend that this is shared ptr
  MyStruct1* sharedPtr = &data;
  // linking context
  PointerLinkingContext plctx1{};
  EXPECT_THAT(
    plctx1
      .getInfoByPtr(sharedPtr, bitsery::ext::PointerOwnershipType::SharedOwner)
      .id,
    Eq(1));
  EXPECT_THAT(plctx1
                .getInfoByPtr(
                  sharedPtr, bitsery::ext::PointerOwnershipType::SharedObserver)
                .id,
              Eq(1));
  EXPECT_THAT(
    plctx1
      .getInfoByPtr(sharedPtr, bitsery::ext::PointerOwnershipType::SharedOwner)
      .id,
    Eq(1));
}

TEST(SerializeExtensionPointer,
     WhenOnlySharedObserverThenPointerLinkingContextIsInvalid)
{
  MyStruct1 data1{};
  MyStruct1 data2{};
  // pretend that this is shared ptr
  MyStruct1* sharedPtr1 = &data1;
  MyStruct1* sharedPtr2 = &data2;
  // linking context
  PointerLinkingContext plctx1{};
  EXPECT_THAT(
    plctx1
      .getInfoByPtr(sharedPtr1,
                    bitsery::ext::PointerOwnershipType::SharedObserver)
      .id,
    Eq(1));
  EXPECT_THAT(
    plctx1
      .getInfoByPtr(sharedPtr2,
                    bitsery::ext::PointerOwnershipType::SharedObserver)
      .id,
    Eq(2));
  EXPECT_THAT(
    plctx1
      .getInfoByPtr(sharedPtr1,
                    bitsery::ext::PointerOwnershipType::SharedObserver)
      .id,
    Eq(1));
  EXPECT_FALSE(plctx1.isValid());
  EXPECT_THAT(
    plctx1
      .getInfoByPtr(sharedPtr1, bitsery::ext::PointerOwnershipType::SharedOwner)
      .id,
    Eq(1));
  EXPECT_THAT(
    plctx1
      .getInfoByPtr(sharedPtr2, bitsery::ext::PointerOwnershipType::SharedOwner)
      .id,
    Eq(2));
  EXPECT_TRUE(plctx1.isValid());
}

TEST_F(SerializeExtensionPointerSerialization, WhenPointersAreNullThenIsValid)
{

  auto& ser = createSerializer();
  ser.ext2b(p1null, PointerOwner{});
  ser.ext2b(p1null, PointerObserver{});
  ser.ext(p3null, PointerOwner{});
  ser.ext(p3null, PointerObserver{});
  createDeserializer();
  EXPECT_THAT(sctx1.ser->adapter().writtenBytesCount(), Eq(4));

  EXPECT_THAT(plctx1.isValid(), Eq(true));
}

#ifndef NDEBUG

TEST_F(SerializeExtensionPointerSerialization,
       WhenPointerOwnerIsNotUniqueThenAssert)
{

  auto& ser = createSerializer();
  ser.ext2b(p1null, PointerOwner{});
  ser.ext2b(pd1, PointerOwner{});
  ser.ext4b(pd2, PointerOwner{});
  ser.ext2b(p1null, PointerOwner{});
  // dublicating pointer
  EXPECT_DEATH(ser.ext2b(pd1, PointerOwner{}), "");
}

TEST_F(SerializeExtensionPointerSerialization,
       WhenRererencedByPointerIsSameAsPointerOwnerThenAssert1)
{
  auto& ser1 = createSerializer();
  ser1.ext4b(pd2, PointerOwner{});
  ser1.ext(d3, ReferencedByPointer{});

  EXPECT_DEATH(ser1.ext(pd3, PointerOwner{}), "");
}

TEST_F(SerializeExtensionPointerSerialization,
       WhenRererencedByPointerIsSameAsPointerOwnerThenAssert2)
{
  auto& ser1 = createSerializer();
  ser1.ext2b(pd1, PointerOwner{});
  ser1.ext4b(d2, ReferencedByPointer{});
  EXPECT_DEATH(ser1.ext2b(d1, ReferencedByPointer{}), "");
}

TEST_F(SerializeExtensionPointerSerialization,
       WhenNonNullPointerIsNullThenAssert)
{
  auto& ser1 = createSerializer();
  EXPECT_DEATH(ser1.ext2b(p1null, PointerOwner{ PointerType::NotNull }), "");
  EXPECT_DEATH(ser1.ext2b(p1null, PointerObserver{ PointerType::NotNull }), "");
}

#endif

TEST_F(SerializeExtensionPointerSerialization,
       WhenPointerObserverPointsToOwnerThenIsValid)
{
  auto& ser1 = createSerializer();
  ser1.ext2b(pd1, PointerOwner{});
  ser1.ext2b(p1null, PointerObserver{});
  EXPECT_THAT(plctx1.isValid(), Eq(true));
  ser1.ext4b(
    pd2,
    PointerObserver{}); // points to d2, and d2 is not still marked as owner
  EXPECT_THAT(plctx1.isValid(), Eq(false));
  ser1.ext4b(pd2, PointerOwner{});    // now d2 is owning pointer
  ser1.ext4b(pd2, PointerObserver{}); // points to d2, but this time d2 has
                                      // owner
  ser1.ext2b(p1null, PointerObserver{});
  EXPECT_THAT(plctx1.isValid(), Eq(true));
}

TEST_F(SerializeExtensionPointerSerialization,
       ReferenceTypeCanAlsoBeReferencedByPointerObservers)
{
  auto& ser1 = createSerializer();
  ser1.ext2b(p1null, PointerObserver{});
  EXPECT_THAT(plctx1.isValid(), Eq(true));
  ser1.ext4b(
    pd2,
    PointerObserver{}); // points to d2, and d2 is not still marked as owner
  EXPECT_THAT(plctx1.isValid(), Eq(false));
  ser1.ext4b(
    d2, ReferencedByPointer{}); // now d2 is marked by marked as owning pointer
  ser1.ext4b(pd2, PointerObserver{}); // points to d2, but this time d2 has
                                      // owner
  ser1.ext(p3null, PointerObserver{});
  EXPECT_THAT(plctx1.isValid(), Eq(true));
}

TEST_F(SerializeExtensionPointerSerialization,
       WhenPointerIsNullThenPointerIdIsZero)
{
  auto& ser1 = createSerializer();
  ser1.ext(p3null, PointerOwner{});
  ser1.ext2b(p1null, PointerObserver{});
  createDeserializer();
  EXPECT_THAT(sctx1.ser->adapter().writtenBytesCount(), Eq(2));
  size_t res;
  bitsery::details::readSize(sctx1.des->adapter(), res, 0, std::false_type{});
  EXPECT_THAT(res, Eq(0));
  bitsery::details::readSize(sctx1.des->adapter(), res, 0, std::false_type{});
  EXPECT_THAT(res, Eq(0));
}

TEST_F(SerializeExtensionPointerSerialization, PointerIdsStartsFromOne)
{
  auto& ser1 = createSerializer();
  ser1.ext2b(pd1, PointerObserver{});
  ser1.ext4b(pd2, PointerObserver{});
  ser1.ext4b(pd2, PointerObserver{});
  ser1.ext2b(p1null, PointerObserver{});
  createDeserializer();
  EXPECT_THAT(sctx1.ser->adapter().writtenBytesCount(), Eq(4));
  size_t res;
  bitsery::details::readSize(sctx1.des->adapter(), res, 0, std::false_type{});
  EXPECT_THAT(res, Eq(1));
  bitsery::details::readSize(sctx1.des->adapter(), res, 0, std::false_type{});
  EXPECT_THAT(res, Eq(2));
  bitsery::details::readSize(sctx1.des->adapter(), res, 0, std::false_type{});
  EXPECT_THAT(res, Eq(2));
  bitsery::details::readSize(sctx1.des->adapter(), res, 0, std::false_type{});
  EXPECT_THAT(res, Eq(0));
}

TEST_F(SerializeExtensionPointerSerialization,
       PointerObserversDoesntSerializeObject)
{
  auto& ser1 = createSerializer();
  ser1.ext2b(pd1, PointerObserver{});
  ser1.ext4b(pd2, PointerObserver{});
  ser1.ext4b(pd2, PointerObserver{});
  createDeserializer();
  EXPECT_THAT(sctx1.ser->adapter().writtenBytesCount(), Eq(3));
}

TEST_F(SerializeExtensionPointerSerialization,
       ReferencedByPointerSerializesIdAndObject)
{
  auto& ser1 = createSerializer();
  ser1.ext2b(d1, ReferencedByPointer{});
  ser1.ext4b(d2, ReferencedByPointer{});
  ser1.ext4b(pd2, PointerObserver{});
  auto& des = createDeserializer();
  EXPECT_THAT(sctx1.ser->adapter().writtenBytesCount(), Eq(3 + 6));
  size_t id{};
  bitsery::details::readSize(sctx1.des->adapter(), id, 0, std::false_type{});
  EXPECT_THAT(id, Eq(1));
  des.value2b(r1);
  EXPECT_THAT(r1, Eq(d1));
  bitsery::details::readSize(sctx1.des->adapter(), id, 0, std::false_type{});
  EXPECT_THAT(id, Eq(2));
  des.value4b(r2);
  EXPECT_THAT(r2, Eq(d2));
  bitsery::details::readSize(sctx1.des->adapter(), id, 0, std::false_type{});
  EXPECT_THAT(id, Eq(2));
}

TEST_F(SerializeExtensionPointerSerialization,
       PointerOwnerSerializesIdAndObject)
{
  auto& ser1 = createSerializer();
  ser1.ext4b(pd2, PointerOwner{});
  ser1.ext(pd3, PointerOwner{});
  auto& des1 = createDeserializer();
  // 2x ids + int32_t + MyStruct1
  EXPECT_THAT(sctx1.ser->adapter().writtenBytesCount(),
              Eq(2 + 4 + MyStruct1::SIZE));
  size_t id;
  bitsery::details::readSize(sctx1.des->adapter(), id, 0, std::false_type{});
  des1.value4b(r2);
  EXPECT_THAT(r2, Eq(*pd2));
  bitsery::details::readSize(sctx1.des->adapter(), id, 0, std::false_type{});
  des1.object(r3);
  EXPECT_THAT(r3, Eq(*pd3));
}

class SerializeExtensionPointerDeserialization
  : public SerializeExtensionPointerSerialization
{
public:
};

TEST_F(SerializeExtensionPointerDeserialization, ReferencedByPointer)
{
  auto& ser = createSerializer();
  ser.ext2b(d1, ReferencedByPointer{});
  ser.ext4b(d2, ReferencedByPointer{});
  ser.ext(d3, ReferencedByPointer{});
  auto& des = createDeserializer();
  des.ext2b(r1, ReferencedByPointer{});
  des.ext4b(r2, ReferencedByPointer{});
  des.ext(r3, ReferencedByPointer{});

  EXPECT_THAT(r1, Eq(d1));
  EXPECT_THAT(r2, Eq(d2));
  EXPECT_THAT(r3, Eq(d3));
}

TEST_F(SerializeExtensionPointerDeserialization,
       WhenReferencedByPointerReadsNullPointerThenInvalidPointerError)
{
  auto& ser = createSerializer();
  bitsery::details::writeSize(sctx1.ser->adapter(), 0u);
  ser.ext2b(d1, ReferencedByPointer{});
  auto& des = createDeserializer();
  des.ext2b(r1, ReferencedByPointer{});
  EXPECT_THAT(sctx1.des->adapter().error(),
              Eq(bitsery::ReaderError::InvalidPointer));
}

TEST_F(SerializeExtensionPointerDeserialization,
       WhenNonNullPointerIsNullThenInvalidPointerError)
{
  createSerializer();
  bitsery::details::writeSize(sctx1.ser->adapter(), 0u);
  auto& des1 = createDeserializer();
  des1.ext2b(p1null, PointerOwner{ PointerType::NotNull });
  EXPECT_THAT(sctx1.des->adapter().error(),
              Eq(bitsery::ReaderError::InvalidPointer));

  auto& des2 = createDeserializer();
  des2.ext2b(p1null, PointerObserver{ PointerType::NotNull });
  EXPECT_THAT(sctx1.des->adapter().error(),
              Eq(bitsery::ReaderError::InvalidPointer));
}

TEST_F(SerializeExtensionPointerDeserialization, PointerOwnerCreatesObjects)
{
  auto& ser = createSerializer();
  ser.ext2b(pd1, PointerOwner{});
  ser.ext4b(pd2, PointerOwner{});
  ser.ext(pd3, PointerOwner{});
  auto& des = createDeserializer();
  des.ext2b(p1null, PointerOwner{});
  des.ext4b(p2null, PointerOwner{});
  des.ext(p3null, PointerOwner{});

  EXPECT_THAT(isPointerContextValid(), Eq(true));
  EXPECT_THAT(p1null, ::testing::NotNull());
  EXPECT_THAT(p2null, ::testing::NotNull());
  EXPECT_THAT(p3null, ::testing::NotNull());
  EXPECT_THAT(*p1null, Eq(*pd1));
  EXPECT_THAT(*p2null, Eq(*pd2));
  EXPECT_THAT(*p3null, Eq(*pd3));
}

TEST_F(SerializeExtensionPointerDeserialization, PointerOwnerDestroysObjects)
{
  auto& ser = createSerializer();
  ser.ext2b(p1null, PointerOwner{});
  ser.ext4b(p2null, PointerOwner{});
  ser.ext(p3null, PointerOwner{});
  auto& des = createDeserializer();
  // pr cannot link to local variables, need to allocate them separately
  pr1 = new int16_t{};
  pr2 = new MyEnumClass{};
  pr3 = new MyStruct1{ 3, 4 };
  des.ext2b(pr1, PointerOwner{});
  des.ext4b(pr2, PointerOwner{});
  des.ext(pr3, PointerOwner{});

  EXPECT_THAT(isPointerContextValid(), Eq(true));
  EXPECT_THAT(pr1, ::testing::IsNull());
  EXPECT_THAT(pr2, ::testing::IsNull());
  EXPECT_THAT(pr3, ::testing::IsNull());
}

TEST_F(SerializeExtensionPointerDeserialization, PointerObserver)
{
  auto& ser = createSerializer();
  // first owner, than observer
  ser.ext4b(d2, ReferencedByPointer{});
  ser.ext2b(p1null, PointerObserver{});
  ser.ext4b(pd2, PointerObserver{});
  // first observer, than owner
  ser.ext(pd3, PointerObserver{});
  ser.ext(pd3, PointerOwner{});
  auto& des = createDeserializer();
  des.ext4b(r2, ReferencedByPointer{});
  des.ext2b(pr1, PointerObserver{});
  des.ext4b(p2null, PointerObserver{});
  des.ext(pr3, PointerObserver{});
  des.ext(pr3, PointerOwner{});

  EXPECT_THAT(isPointerContextValid(), Eq(true));
  // serialize null, override non-null
  EXPECT_THAT(pr1, Eq(p1null));
  // serialize non-null, override null
  EXPECT_THAT(*p2null, Eq(*pd2));
  EXPECT_THAT(p2null, Eq(&r2));
  // serialize non-null override non-null
  EXPECT_THAT(*pr3, Eq(*pd3));
  EXPECT_THAT(pr3, Eq(&r3));
}

struct Test1Data
{
  std::vector<MyStruct1> vdata;
  std::vector<MyStruct1*> vptr;
  MyStruct1 o1;
  MyStruct1* po1;
  int32_t i1;
  int32_t* pi1;

  template<typename S>
  void serialize(S& s)
  {
    // set container elements to be candidates for non-owning pointers
    s.container(
      vdata, 100, [](S& s, MyStruct1& d) { s.ext(d, ReferencedByPointer{}); });
    // contains non owning pointers
    //
    // IMPORTANT !!!
    //  ALWAYS ACCEPT BY REFERENCE like this: T* (&obj)
    //
    s.container(
      vptr, 100, [](S& s, MyStruct1*(&d)) { s.ext(d, PointerObserver{}); });
    // just a regular fields
    s.object(o1);
    s.value4b(i1);
    // observer
    s.ext(po1, PointerObserver{});
    // owner
    s.ext4b(pi1, PointerOwner{});
  }
};

TEST(SerializeExtensionPointer, IntegrationTest)
{

  Test1Data data{};
  data.vdata.push_back({ 165, -45 });
  data.vdata.push_back({ 7895, -1576 });
  data.vdata.push_back({ 5987, -798 });
  // container of non owning pointers (observers)
  data.vptr.push_back(nullptr);
  data.vptr.push_back(std::addressof(data.vdata[0]));
  data.vptr.push_back(std::addressof(data.vdata[2]));
  // regular fields
  data.o1 = MyStruct1{ 145, 948 };
  data.i1 = 945415;
  // observer
  data.po1 = std::addressof(data.vdata[1]);
  // owning pointer
  data.pi1 = new int32_t{};

  Test1Data res{};

  PointerLinkingContext plctx1{};
  SerContext sctx1;
  sctx1.createSerializer(plctx1).object(data);
  sctx1.createDeserializer(plctx1).object(res);

  EXPECT_THAT(plctx1.isValid(), Eq(true));
  // check regular fields
  EXPECT_THAT(res.i1, Eq(data.i1));
  EXPECT_THAT(res.o1, Eq(data.o1));
  // check data container
  EXPECT_THAT(res.vdata, ::testing::ContainerEq(data.vdata));
  // check owning pointers
  EXPECT_THAT(*res.pi1, Eq(*data.pi1));
  EXPECT_THAT(res.pi1, ::testing::Ne(data.pi1));
  // check if observers points to correct data
  EXPECT_THAT(res.po1, Eq(std::addressof(res.vdata[1])));
  EXPECT_THAT(res.vptr[0], ::testing::IsNull());
  EXPECT_THAT(res.vptr[1], Eq(std::addressof(res.vdata[0])));
  EXPECT_THAT(res.vptr[2], Eq(std::addressof(res.vdata[2])));

  // free owning raw pointers
  delete data.pi1;
  delete res.pi1;
}

TEST(SerializeExtensionPointer,
     PointerOwnerWithNonPolymorphicTypeCanUseLambdaOverload)
{
  const int32_t NEW_VALUE = 2;
  const int32_t OLD_VALUE = 1;
  MyStruct1* data = new MyStruct1{ NEW_VALUE, NEW_VALUE };
  MyStruct1* res = new MyStruct1{ OLD_VALUE, OLD_VALUE };
  // linking context
  PointerLinkingContext plctx1{};
  SerContext sctx1;
  auto& ser = sctx1.createSerializer(plctx1);
  ser.ext(data, PointerOwner{}, [](decltype(ser)& ser, MyStruct1& o) {
    // serialize only one field
    ser.value4b(o.i1);
  });
  auto& des = sctx1.createDeserializer(plctx1);
  des.ext(res, PointerOwner{}, [](decltype(des)& des, MyStruct1& o) {
    // deserialize only one field
    des.value4b(o.i1);
  });

  EXPECT_THAT(res->i1, Eq(NEW_VALUE));
  EXPECT_THAT(res->i2, Eq(OLD_VALUE)); // we didn't serialized that

  delete data;
  delete res;
}

TEST(SerializeExtensionPointer, ReferencedByPointerCanUseLambdaOverload)
{
  const int32_t NEW_VALUE = 2;
  const int32_t OLD_VALUE = 1;
  MyStruct1 data = MyStruct1{ NEW_VALUE, NEW_VALUE };
  MyStruct1 res = MyStruct1{ OLD_VALUE, OLD_VALUE };
  // linking context
  PointerLinkingContext plctx1{};
  SerContext sctx1;
  auto& ser = sctx1.createSerializer(plctx1);
  ser.ext(data, ReferencedByPointer{}, [](decltype(ser)& ser, MyStruct1& o) {
    // serialize only one field
    ser.value4b(o.i1);
  });
  auto& des = sctx1.createDeserializer(plctx1);
  des.ext(res, ReferencedByPointer{}, [](decltype(des)& des, MyStruct1& o) {
    // deserialize only one field
    des.value4b(o.i1);
  });

  EXPECT_THAT(res.i1, Eq(NEW_VALUE));
  EXPECT_THAT(res.i2, Eq(OLD_VALUE)); // we didn't serialized that
}

TEST(SerializeExtensionPointer, PointerOwnerCanUseValueOverload)
{
  auto* data = new int64_t{ 49845894 };
  auto* res = new int64_t{ -78548415 };

  PointerLinkingContext plctx1{};
  SerContext sctx1;
  sctx1.createSerializer(plctx1).ext8b(data, PointerOwner{});
  sctx1.createDeserializer(plctx1).ext8b(res, PointerOwner{});

  EXPECT_THAT(*res, Eq(*data));

  delete data;
  delete res;
}

TEST(SerializeExtensionPointer, ReferencedByPointerCanUseValueOverload)
{
  int64_t data{ 49845894 };
  int64_t res{ -78548415 };

  PointerLinkingContext plctx1{};
  SerContext sctx1;
  sctx1.createSerializer(plctx1).ext8b(data, ReferencedByPointer{});
  sctx1.createDeserializer(plctx1).ext8b(res, ReferencedByPointer{});

  EXPECT_THAT(res, Eq(data));
}
