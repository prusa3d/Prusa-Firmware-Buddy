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

#include <bitsery/details/serialization_common.h>
#include <bitsery/traits/array.h>

#include "serialization_test_utils.h"
#include <gmock/gmock.h>

using bitsery::EndiannessType;
using testing::ContainerEq;
using testing::Eq;

template<typename BufType>
class DataWriting : public testing::Test
{
public:
  using TWriter = bitsery::OutputBufferAdapter<BufType>;
  using TBuffer = BufType;
};

using NonFixedContainer = std::vector<uint8_t>;
using FixedContainer = std::array<uint8_t, 100>;

using ContainerTypes = ::testing::Types<FixedContainer, NonFixedContainer>;

TYPED_TEST_SUITE(DataWriting, ContainerTypes, );

static constexpr size_t DATA_SIZE = 14u;

template<typename BW>
void
writeData(BW& bw)
{
  uint16_t tmp1{ 45 }, tmp2{ 6543 }, tmp3{ 46533 };
  uint32_t tmp4{ 8979445 }, tmp5{ 7987564 };
  bw.template writeBytes<2>(tmp1);
  bw.template writeBytes<2>(tmp2);
  bw.template writeBytes<2>(tmp3);
  bw.template writeBytes<4>(tmp4);
  bw.template writeBytes<4>(tmp5);
}

TYPED_TEST(DataWriting, GetWrittenBytesCountReturnsActualBytesWritten)
{
  using TWriter = typename TestFixture::TWriter;
  using TBuffer = typename TestFixture::TBuffer;
  TBuffer buf{};
  TWriter bw{ buf };
  writeData(bw);
  bw.flush();
  auto writtenSize = bw.writtenBytesCount();
  EXPECT_THAT(writtenSize, DATA_SIZE);
  EXPECT_THAT(buf.size(), ::testing::Ge(DATA_SIZE));
}

TYPED_TEST(DataWriting, WhenWritingBitsThenMustFlushWriter)
{
  using TWriter = typename TestFixture::TWriter;
  using TBuffer = typename TestFixture::TBuffer;
  TBuffer buf{};
  TWriter bw{ buf };
  bitsery::details::OutputAdapterBitPackingWrapper<TWriter> bpw{ bw };
  bpw.writeBits(3u, 2);
  auto writtenSize1 = bpw.writtenBytesCount();
  bpw.flush();
  auto writtenSize2 = bpw.writtenBytesCount();
  EXPECT_THAT(writtenSize1, Eq(0));
  EXPECT_THAT(writtenSize2, Eq(1));
}

TYPED_TEST(DataWriting, WhenDataAlignedThenFlushHasNoEffect)
{
  using TWriter = typename TestFixture::TWriter;
  using TBuffer = typename TestFixture::TBuffer;
  TBuffer buf{};
  TWriter bw{ buf };
  bitsery::details::OutputAdapterBitPackingWrapper<TWriter> bpw{ bw };
  bpw.writeBits(3u, 2);
  bpw.align();
  auto writtenSize1 = bpw.writtenBytesCount();
  bpw.flush();
  auto writtenSize2 = bpw.writtenBytesCount();
  EXPECT_THAT(writtenSize1, Eq(1));
  EXPECT_THAT(writtenSize2, Eq(1));
}

TEST(DataWritingNonFixedBufferContainer, ContainerIsAlwaysResizedToCapacity)
{
  NonFixedContainer buf{};
  bitsery::OutputBufferAdapter<NonFixedContainer> bw{ buf };
  for (auto i = 0; i < 5; ++i) {
    uint32_t tmp{};
    bw.writeBytes<4>(tmp);
    bw.writeBytes<4>(tmp);
    EXPECT_TRUE(buf.size() == buf.capacity());
  }
}
