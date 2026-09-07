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

#include <bitsery/adapter/buffer.h>
#include <bitsery/adapter/measure_size.h>
#include <bitsery/adapter/stream.h>
#include <bitsery/deserializer.h>
#include <bitsery/ext/value_range.h>
#include <bitsery/serializer.h>
#include <bitsery/traits/array.h>
#include <bitsery/traits/string.h>
#include <bitsery/traits/vector.h>

#include <gmock/gmock.h>

// some helper types
using Buffer = std::vector<char>;
using OutputAdapter = bitsery::OutputBufferAdapter<Buffer>;
using InputAdapter = bitsery::InputBufferAdapter<Buffer>;

using bitsery::ReaderError;

using testing::Eq;
using testing::Ge;

struct DisableAdapterErrorsConfig
{
  static constexpr bitsery::EndiannessType Endianness =
    bitsery::DefaultConfig::Endianness;
  static constexpr bool CheckAdapterErrors = false;
  static constexpr bool CheckDataErrors = true;
};

TEST(OutputBuffer, WhenSetWritePositionThenResizeUnderlyingBufferIfRequired)
{
  // setup data
  Buffer buf{};
  OutputAdapter w{ buf };
  const auto initialSize = buf.size();
  EXPECT_THAT(buf.size(), Eq(initialSize));
  EXPECT_THAT(w.currentWritePos(), Eq(0));
  w.currentWritePos(initialSize + 10);
  EXPECT_THAT(w.currentWritePos(), Eq(initialSize + 10));
  EXPECT_THAT(buf.size(), Ge(initialSize + 10));
}

TEST(
  OutputBuffer,
  WhenSettingCurrentPositionBeforeBufferEndThenWrittenBytesCountIsNotAffected)
{
  // setup data
  Buffer buf{};
  OutputAdapter w{ buf };
  const auto initialSize = buf.size();
  EXPECT_THAT(buf.size(), Eq(initialSize));
  EXPECT_THAT(w.writtenBytesCount(), Eq(0));
  w.currentWritePos(initialSize + 10);
  w.writeBytes<8>(static_cast<uint64_t>(1));
  EXPECT_THAT(w.writtenBytesCount(), Eq(initialSize + 10 + 8));
  w.currentWritePos(0);
  EXPECT_THAT(w.writtenBytesCount(), Eq(initialSize + 10 + 8));
}

TEST(OutputBuffer, CanWorkWithFixedSizeBuffer)
{
  // setup data
  std::array<uint8_t, 10> buf{};
  bitsery::OutputBufferAdapter<std::array<uint8_t, 10>> w{ buf };
  const auto initialSize = buf.size();
  EXPECT_THAT(buf.size(), Eq(initialSize));
  EXPECT_THAT(w.currentWritePos(), Eq(0));
  w.currentWritePos(5);
  EXPECT_THAT(w.currentWritePos(), Eq(5));
}

TEST(InputBuffer, CorrectlySetsAndGetsCurrentReadPosition)
{

  Buffer buf{};
  buf.resize(100);
  InputAdapter r{ buf.begin(), 10 };
  r.currentReadPos(5);
  EXPECT_THAT(r.currentReadPos(), Eq(5));
  r.currentReadPos(0);
  EXPECT_THAT(r.currentReadPos(), Eq(0));
  uint8_t tmp;
  r.readBytes<1>(tmp);
  EXPECT_THAT(r.currentReadPos(), Eq(1));
}

TEST(InputBuffer, WhenSetReadPositionOutOfRangeThenDataOverflow)
{

  Buffer buf{};
  buf.resize(100);
  InputAdapter r{ buf.begin(), 10 };
  r.currentReadPos(10);
  EXPECT_THAT(r.error(), Eq(ReaderError::NoError));
  r.currentReadPos(11);
  EXPECT_THAT(r.error(), Eq(ReaderError::DataOverflow));
}

TEST(InputBuffer, WhenSetReadEndPositionOutOfRangeThenDataOverflow)
{
  Buffer buf{};
  buf.resize(100);
  InputAdapter r{ buf.begin(), 10 };
  r.currentReadEndPos(11);
  EXPECT_THAT(r.error(), Eq(ReaderError::DataOverflow));
}

TEST(InputBuffer, WhenReadEndPositionIsNotSetThenReturnZeroAsBufferEndPosition)
{
  Buffer buf{};
  buf.resize(100);
  InputAdapter r{ buf.begin(), 10 };
  EXPECT_THAT(r.currentReadEndPos(), Eq(0));
  r.currentReadEndPos(5);
  EXPECT_THAT(r.currentReadEndPos(), Eq(5));
  EXPECT_THAT(r.error(), Eq(ReaderError::NoError));
}

TEST(InputBuffer,
     WhenReadEndPositionIsNotZeroThenDataOverflowErrorWillBeIgnored)
{
  Buffer buf{};
  buf.resize(100);
  InputAdapter r{ buf.begin(), 1 };
  r.currentReadEndPos(1);
  uint32_t tmp{};
  r.readBytes<4>(tmp);
  EXPECT_THAT(tmp, Eq(0));
  EXPECT_THAT(r.error(), Eq(ReaderError::NoError));
  r.currentReadEndPos(0);
  r.readBytes<4>(tmp);
  EXPECT_THAT(tmp, Eq(0));
  EXPECT_THAT(r.error(), Eq(ReaderError::DataOverflow));
}

TEST(InputBuffer,
     WhenReadingPastReadEndPositionOrBufferEndThenReadPositionDoesntChange)
{
  Buffer buf{};
  buf.resize(10);
  InputAdapter r{ buf.begin(), 3 };
  uint32_t tmp{};
  r.currentReadEndPos(2);
  r.readBytes<4>(tmp);
  EXPECT_THAT(r.currentReadPos(), Eq(0));
  EXPECT_THAT(r.error(), Eq(ReaderError::NoError));
  EXPECT_THAT(tmp, Eq(0));
  r.currentReadEndPos(0);
  r.readBytes<4>(tmp);
  EXPECT_THAT(r.currentReadPos(), Eq(0));
  EXPECT_THAT(r.error(), Eq(ReaderError::DataOverflow));
  EXPECT_THAT(tmp, Eq(0));
}

TEST(
  InputBuffer,
  WhenReaderHasErrorsThenSettingReadPosAndReadEndPosIsIgnoredAndGettingAlwaysReturnsZero)
{
  Buffer buf{};
  buf.resize(10);
  InputAdapter r{ buf.begin(), 10 };
  uint32_t tmp{};
  r.readBytes<4>(tmp);
  r.currentReadEndPos(5);
  EXPECT_THAT(r.currentReadPos(), Eq(4));
  EXPECT_THAT(r.currentReadEndPos(), Eq(5));
  EXPECT_THAT(r.error(), Eq(ReaderError::NoError));
  r.currentReadEndPos(11);
  EXPECT_THAT(r.error(), Eq(ReaderError::DataOverflow));
  EXPECT_THAT(r.currentReadPos(), Eq(0));
  EXPECT_THAT(r.currentReadEndPos(), Eq(0));
  r.currentReadPos(1);
  r.currentReadEndPos(1);
  EXPECT_THAT(r.currentReadPos(), Eq(0));
  EXPECT_THAT(r.currentReadEndPos(), Eq(0));
}

TEST(InputBuffer, ConstDataForBufferAllAdapters)
{
  // create and write to buffer
  uint16_t data = 7549;
  Buffer bufWrite{};
  OutputAdapter bw{ bufWrite };
  bw.writeBytes<2>(data);
  bw.flush();
  const Buffer buf{ bufWrite };

  // read from buffer

  bitsery::InputBufferAdapter<const Buffer> r1{ buf.begin(), buf.end() };

  uint16_t res1{};
  r1.readBytes<2>(res1);
  EXPECT_THAT(res1, Eq(data));
}

#ifndef NDEBUG
TEST(InputBuffer,
     WhenAdapterErrorsIsDisabledThenCanChangeAnyReadPositionAndReadsAsserts)
{
  // create and write to buffer
  uint64_t data = 0x1122334455667788;
  Buffer buf{};
  OutputAdapter bw{ buf };
  bw.writeBytes<8>(data);
  bw.flush();

  bitsery::InputBufferAdapter<Buffer, DisableAdapterErrorsConfig> r1{
    buf.begin(), 2
  };
  uint16_t res1{};
  r1.readBytes<2>(res1);
  EXPECT_THAT(res1, Eq(0x7788)); // default config is little endian
  EXPECT_THAT(r1.currentReadPos(), Eq(2));
  r1.currentReadPos(4);
  EXPECT_THAT(r1.currentReadPos(), Eq(4));
  EXPECT_DEATH(r1.readBytes<2>(res1), ""); // default config is little endian
}
#endif

TEST(
  InputStream,
  WhenAdapterErrorsIsDisabledThenReadingPastEndDoesntSetErrorAndDoesntReturnZero)
{
  // create and write to buffer
  std::stringstream ss{};
  bitsery::OutputStreamAdapter bw{ ss };
  uint32_t data = 0x12345678;
  bw.writeBytes<4>(data);
  bw.flush();

  bitsery::BasicInputStreamAdapter<char,
                                   DisableAdapterErrorsConfig,
                                   std::char_traits<char>>
    br{ ss };
  uint32_t res{};
  br.readBytes<4>(res);
  EXPECT_THAT(res, Eq(data));
  br.readBytes<4>(res);
  EXPECT_THAT(res, Eq(data));
  EXPECT_THAT(br.isCompletedSuccessfully(), Eq(true));
}

template<template<typename...> class TAdapter>
struct InBufferConfig
{
  using Data = std::vector<char>;
  using Adapter = TAdapter<Data>;

  Data data{};
  Adapter createReader(const std::vector<char>& buffer)
  {
    data = buffer;
    return Adapter{ data.begin(), data.size() };
  }
};

template<typename TAdapter>
struct InStreamConfig
{
  using Data = std::stringstream;
  using Adapter = TAdapter;

  Data data{};
  Adapter createReader(const std::vector<char>& buffer)
  {
    std::string str(buffer.begin(), buffer.end());
    data = std::stringstream{ str };
    return Adapter{ data };
  }
};

template<typename TAdapterWithData>
class AdapterConfig : public testing::Test
{
public:
  TAdapterWithData config{};
};

using AdapterInputTypes =
  ::testing::Types<InBufferConfig<bitsery::InputBufferAdapter>,
                   InStreamConfig<bitsery::InputStreamAdapter>>;

template<typename TConfig>
class InputAll : public AdapterConfig<TConfig>
{};

TYPED_TEST_SUITE(InputAll, AdapterInputTypes, );

TYPED_TEST(InputAll, SettingMultipleErrorsAlwaysReturnsFirstError)
{
  auto r = this->config.createReader({ 0, 0, 0, 0 });
  EXPECT_THAT(r.error(), Eq(ReaderError::NoError));
  r.error(ReaderError::InvalidPointer);
  EXPECT_THAT(r.error(), Eq(ReaderError::InvalidPointer));
  r.error(ReaderError::DataOverflow);
  EXPECT_THAT(r.error(), Eq(ReaderError::InvalidPointer));
  r.error(ReaderError::NoError);
  EXPECT_THAT(r.error(), Eq(ReaderError::InvalidPointer));
}

TYPED_TEST(InputAll, CanBeMoveConstructedAndMoveAssigned)
{
  auto r = this->config.createReader({ 1, 2, 3 });
  uint8_t res{};
  r.template readBytes<1>(res);
  EXPECT_THAT(res, Eq(1));
  // move construct
  auto r1 = std::move(r);
  r1.template readBytes<1>(res);
  EXPECT_THAT(res, Eq(2));
  // move assign
  r = std::move(r1);
  r.template readBytes<1>(res);
  EXPECT_THAT(res, Eq(3));
}

TYPED_TEST(InputAll, WhenAlignHasNonZerosThenInvalidDataError)
{

  auto r = this->config.createReader({ 0x7F });
  bitsery::details::InputAdapterBitPackingWrapper<decltype(r)> bpr{ r };

  uint8_t tmp{ 0xFF };
  bpr.readBits(tmp, 3);
  bpr.align();
  EXPECT_THAT(bpr.error(), Eq(ReaderError::InvalidData));
}

TYPED_TEST(InputAll,
           WhenAllBytesAreReadWithoutErrorsThenIsCompletedSuccessfully)
{
  // setup data

  uint32_t tb = 94545646;
  int16_t tc = -8778;
  uint8_t td = 200;

  // create and write to buffer
  Buffer buf{};
  OutputAdapter bw{ buf };

  bw.writeBytes<4>(tb);
  bw.writeBytes<2>(tc);
  bw.writeBytes<1>(td);
  bw.flush();
  buf.resize(bw.writtenBytesCount());

  auto br = this->config.createReader(buf);

  uint32_t rb = 94545646;
  int16_t rc = -8778;
  uint8_t rd = 200;

  br.template readBytes<4>(rb);
  EXPECT_THAT(br.error(), Eq(bitsery::ReaderError::NoError));
  br.template readBytes<2>(rc);
  EXPECT_THAT(br.error(), Eq(bitsery::ReaderError::NoError));
  EXPECT_THAT(br.isCompletedSuccessfully(), Eq(false));
  br.template readBytes<1>(rd);
  EXPECT_THAT(br.error(), Eq(bitsery::ReaderError::NoError));
  EXPECT_THAT(br.isCompletedSuccessfully(), Eq(true));

  EXPECT_THAT(rb, Eq(tb));
  EXPECT_THAT(rc, Eq(tc));
  EXPECT_THAT(rd, Eq(td));
}

TYPED_TEST(InputAll, WhenReadingMoreThanAvailableThenDataOverflow)
{
  // setup data
  uint8_t t1 = 111;

  Buffer buf{};
  OutputAdapter w{ buf };
  w.writeBytes<1>(t1);
  w.flush();
  buf.resize(w.writtenBytesCount());

  auto r = this->config.createReader(buf);

  uint8_t r1{};
  EXPECT_THAT(r.isCompletedSuccessfully(), Eq(false));
  EXPECT_THAT(r.error(), Eq(ReaderError::NoError));
  r.template readBytes<1>(r1);
  EXPECT_THAT(r.isCompletedSuccessfully(), Eq(true));
  EXPECT_THAT(r.error(), Eq(ReaderError::NoError));
  EXPECT_THAT(r1, Eq(t1));
  r.template readBytes<1>(r1);
  r.template readBytes<1>(r1);
  EXPECT_THAT(r1, Eq(0));
  EXPECT_THAT(r.isCompletedSuccessfully(), Eq(false));
  EXPECT_THAT(r.error(), Eq(ReaderError::DataOverflow));
}

TYPED_TEST(InputAll, WhenReaderHasErrorsAllThenReadsReturnZero)
{
  // setup data
  uint8_t t1 = 111;

  Buffer buf{};
  OutputAdapter w{ buf };
  w.writeBytes<1>(t1);
  w.writeBytes<1>(t1);
  w.flush();
  buf.resize(w.writtenBytesCount());

  auto r = this->config.createReader(buf);

  uint8_t r1{};
  r.template readBytes<1>(r1);
  EXPECT_THAT(r1, Eq(t1));
  r.error(ReaderError::InvalidPointer);
  r.template readBytes<1>(r1);
  EXPECT_THAT(r1, Eq(0));
}

template<template<typename...> class TAdapter>
struct OutBufferConfig
{
  using Data = std::vector<char>;
  using Adapter = TAdapter<Data>;

  Data data{};
  Adapter createWriter() { return Adapter{ data }; }

  bitsery::InputBufferAdapter<Data> getReader()
  {
    return bitsery::InputBufferAdapter<Data>{ data.begin(), data.end() };
  }
};

template<typename TAdapter>
struct OutStreamConfig
{
  using Data = std::stringstream;
  using Adapter = TAdapter;

  Data data{};
  Adapter createWriter() { return Adapter{ data }; }

  bitsery::InputStreamAdapter getReader()
  {
    return bitsery::InputStreamAdapter{ data };
  }
};

using AdapterOutputTypes =
  ::testing::Types<OutBufferConfig<bitsery::OutputBufferAdapter>,
                   OutStreamConfig<bitsery::OutputStreamAdapter>,
                   OutStreamConfig<bitsery::OutputBufferedStreamAdapter>>;

template<typename TConfig>
class OutputAll : public AdapterConfig<TConfig>
{};

TYPED_TEST_SUITE(OutputAll, AdapterOutputTypes, );

TYPED_TEST(OutputAll, CanBeMoveConstructedAndMoveAssigned)
{
  auto w = this->config.createWriter();
  uint8_t data{ 1 };
  w.template writeBytes<1>(data);
  // move construct
  auto w1 = std::move(w);
  data = 2;
  w1.template writeBytes<1>(data);
  // move assignment
  w = std::move(w1);
  data = 3;
  w.template writeBytes<1>(data);
  w.flush();

  auto r = this->config.getReader();
  r.template readBytes<1>(data);
  EXPECT_THAT(data, Eq(1));
  r.template readBytes<1>(data);
  EXPECT_THAT(data, Eq(2));
  r.template readBytes<1>(data);
  EXPECT_THAT(data, Eq(3));
}

template<typename T>
class OutputStreamBuffered : public testing::Test
{
public:
  using Buffer = T;
  using Adapter =
    bitsery::BasicBufferedOutputStreamAdapter<char,
                                              bitsery::DefaultConfig,
                                              std::char_traits<char>,
                                              Buffer>;

  static constexpr size_t InternalBufferSize = 128;

  std::stringstream stream{};

  Adapter writer{ stream, 128 };
};

using BufferedAdapterInternalBufferTypes =
  ::testing::Types<std::vector<char>, std::array<char, 128>, std::string>;

TYPED_TEST_SUITE(OutputStreamBuffered, BufferedAdapterInternalBufferTypes, );

TYPED_TEST(OutputStreamBuffered,
           WhenInternalBufferIsFullThenWriteBufferAndValueToStream)
{
  uint8_t x{};
  for (auto i = 0u; i < TestFixture::InternalBufferSize; ++i)
    this->writer.template writeBytes<1>(x);
  EXPECT_TRUE(this->stream.str().empty());
  this->writer.template writeBytes<1>(x);
  EXPECT_THAT(this->stream.str().size(),
              Eq(TestFixture::InternalBufferSize + 1));
}

TYPED_TEST(OutputStreamBuffered, WhenFlushThenWriteImmediately)
{
  uint8_t x{};
  this->writer.template writeBytes<1>(x);
  EXPECT_THAT(this->stream.str().size(), Eq(0));
  this->writer.flush();
  EXPECT_THAT(this->stream.str().size(), Eq(1));
  this->writer.flush();
  EXPECT_THAT(this->stream.str().size(), Eq(1));
}

TYPED_TEST(OutputStreamBuffered,
           WhenBufferIsStackAllocatedThenBufferSizeViaCtorHasNoEffect)
{

  // create writer with half the internal buffer size
  // for std::vector it should overflow, and for std::array it should have no
  // effect
  typename TestFixture::Adapter w{ this->stream,
                                   TestFixture::InternalBufferSize / 2 };

  uint8_t x{};
  for (auto i = 0u; i < TestFixture::InternalBufferSize; ++i)
    w.template writeBytes<1>(x);
  static constexpr bool ShouldWriteToStream =
    bitsery::traits::ContainerTraits<typename TestFixture::Buffer>::isResizable;
  EXPECT_THAT(this->stream.str().empty(), ::testing::Ne(ShouldWriteToStream));
}

struct TestData
{
  uint32_t b4;
  std::vector<uint16_t> vb2;
};

template<typename S>
void
serialize(S& s, TestData& o)
{
  s.value4b(o.b4);
  s.enableBitPacking([&o](typename S::BPEnabledType& sbp) {
    sbp.ext(o.b4, bitsery::ext::ValueRange<uint32_t>{ 0, 1023 }); // 10 bits
    sbp.value4b(o.b4);
    sbp.container(
      o.vb2, 10, [](typename S::BPEnabledType& sbp, uint16_t& data) {
        sbp.ext(data, bitsery::ext::ValueRange<uint16_t>{ 0, 200 }); // 7 bits
      });
  });
  s.container2b(o.vb2, 10);
}

TEST(AdapterWriterMeasureSize, CorrectlyMeasuresBytesAndBitsSize)
{
  TestData data{ 456, { 45, 98, 189, 4 } };

  Buffer buf{};
  auto measuredSize = bitsery::quickSerialization(bitsery::MeasureSize{}, data);
  auto writtenSize = bitsery::quickSerialization(OutputAdapter{ buf }, data);
  EXPECT_THAT(measuredSize, Eq(24));
  EXPECT_THAT(measuredSize, Eq(writtenSize));
}
