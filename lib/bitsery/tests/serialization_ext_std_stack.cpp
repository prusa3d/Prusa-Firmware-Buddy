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
#include <bitsery/ext/std_stack.h>
#include <gmock/gmock.h>

using StdStack = bitsery::ext::StdStack;

using testing::Eq;

template<typename T>
void
test(SerializationContext& ctx, const T& v, T& r)
{
  ctx.createSerializer().ext4b(v, StdStack{ 10 });
  ctx.createDeserializer().ext4b(r, StdStack{ 10 });
}

TEST(SerializeExtensionStdStack, DefaultContainer)
{
  std::stack<int32_t> t1{};
  t1.push(3);
  t1.push(-4854);
  std::stack<int32_t> r1{};

  SerializationContext ctx1;
  test(ctx1, t1, r1);
  EXPECT_THAT(t1, Eq(r1));
}

TEST(SerializeExtensionStdStack, VectorContainer)
{
  std::stack<int32_t, std::vector<int32_t>> t1{};
  t1.push(3);
  t1.push(-4854);
  std::stack<int32_t, std::vector<int32_t>> r1{};

  SerializationContext ctx1;
  test(ctx1, t1, r1);
  EXPECT_THAT(t1, Eq(r1));
}
