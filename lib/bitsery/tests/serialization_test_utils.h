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

#ifndef BITSERY_SERIALIZER_TEST_UTILS_H
#define BITSERY_SERIALIZER_TEST_UTILS_H

#include <bitsery/adapter/buffer.h>
#include <bitsery/bitsery.h>
#include <bitsery/traits/vector.h>
#include <memory>

/*
 * define some types for testing
 */

struct MyStruct1
{
  MyStruct1(int32_t v1, int32_t v2)
    : i1{ v1 }
    , i2{ v2 }
  {
  }

  MyStruct1()
    : MyStruct1{ 0, 0 }
  {
  }

  int32_t i1;
  int32_t i2;

  bool operator==(const MyStruct1& rhs) const
  {
    return i1 == rhs.i1 && i2 == rhs.i2;
  }
  friend bool operator<(const MyStruct1& lhs, const MyStruct1& rhs)
  {
    return lhs.i1 < rhs.i1 || (lhs.i1 == rhs.i1 && lhs.i2 < rhs.i2);
  }

  static constexpr size_t SIZE = sizeof(MyStruct1::i1) + sizeof(MyStruct1::i2);
};

template<typename S>
void
serialize(S& s, MyStruct1& o)
{
  s.template value<sizeof(o.i1)>(o.i1);
  s.template value<sizeof(o.i2)>(o.i2);
}

enum class MyEnumClass : int32_t
{
  E1,
  E2,
  E3,
  E4,
  E5,
  E6
};

struct MyStruct2
{
  enum MyEnum
  {
    V1,
    V2,
    V3,
    V4,
    V5,
    V6
  };

  MyStruct2(MyEnum e, MyStruct1 s)
    : e1{ e }
    , s1{ s }
  {
  }

  MyStruct2()
    : MyStruct2{ V1, { 0, 0 } }
  {
  }

  MyEnum e1;
  MyStruct1 s1;

  bool operator==(const MyStruct2& rhs) const
  {
    return e1 == rhs.e1 && s1 == rhs.s1;
  }

  static constexpr size_t SIZE = MyStruct1::SIZE + sizeof(MyStruct2::e1);
};

template<typename S>
void
serialize(S& s, MyStruct2& o)
{
  s.template value<sizeof(o.e1)>(o.e1);
  s.object(o.s1);
}

using Buffer = std::vector<char>;
using Reader = bitsery::InputBufferAdapter<Buffer>;
using Writer = bitsery::OutputBufferAdapter<Buffer>;

template<typename Context>
class BasicSerializationContext
{
public:
  using TSerializer = bitsery::Serializer<Writer, Context>;
  using TDeserializer = bitsery::Deserializer<Reader, Context>;
  using TSerializerBPEnabled = typename TSerializer::BPEnabledType;
  using TDeserializerBPEnabled = typename TDeserializer::BPEnabledType;

  Buffer buf{};
  std::unique_ptr<TSerializer> ser{};
  std::unique_ptr<TDeserializer> des{};

  template<typename T = Context,
           typename std::enable_if<std::is_void<T>::value>::type* = nullptr>
  TSerializer& createSerializer()
  {
    if (!ser) {
      ser = std::unique_ptr<TSerializer>(new TSerializer{ buf });
    }
    return *ser;
  }

  template<typename T = Context>
  TSerializer& createSerializer(
    typename std::enable_if<!std::is_void<T>::value, T>::type& ctx)
  {
    if (!ser) {
      ser = std::unique_ptr<TSerializer>(new TSerializer{ ctx, buf });
    }
    return *ser;
  }

  template<typename T = Context,
           typename std::enable_if<std::is_void<T>::value>::type* = nullptr>
  TDeserializer& createDeserializer()
  {
    size_t writtenBytes = 0;
    if (ser) {
      ser->adapter().flush();
      writtenBytes = ser->adapter().writtenBytesCount();
    }
    if (!des) {
      des = std::unique_ptr<TDeserializer>(
        new TDeserializer{ buf.begin(), writtenBytes });
    }
    return *des;
  }

  template<typename T = Context>
  TDeserializer& createDeserializer(
    typename std::enable_if<!std::is_void<T>::value, T>::type& ctx)
  {
    size_t writtenBytes = 0;
    if (ser) {
      ser->adapter().flush();
      writtenBytes = ser->adapter().writtenBytesCount();
    }
    if (!des) {
      des = std::unique_ptr<TDeserializer>(
        new TDeserializer{ ctx, buf.begin(), writtenBytes });
    }
    return *des;
  }

  size_t getBufferSize() const { return ser->adapter().writtenBytesCount(); }

  // since all containers .size() method returns size_t, it cannot be directly
  // serialized, because size_t is platform dependant this function returns
  // number of bytes written to buffer, when reading/writing size of container
  static size_t containerSizeSerializedBytesCount(size_t elemsCount)
  {
    if (elemsCount < 0x80u)
      return 1;
    if (elemsCount < 0x4000u)
      return 2;
    return 4;
  }
};

// helper type
using SerializationContext = BasicSerializationContext<void>;

#endif // BITSERY_SERIALIZER_TEST_UTILS_H
