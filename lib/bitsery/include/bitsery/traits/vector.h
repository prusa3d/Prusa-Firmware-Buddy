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

#ifndef BITSERY_TRAITS_STD_VECTOR_H
#define BITSERY_TRAITS_STD_VECTOR_H

#include "core/std_defaults.h"
#include <vector>

namespace bitsery {

namespace traits {
template<typename T, typename Allocator>
struct ContainerTraits<std::vector<T, Allocator>>
  : public StdContainer<std::vector<T, Allocator>, true, true>
{};

// bool vector is not contiguous, do not copy it directly to buffer
template<typename Allocator>
struct ContainerTraits<std::vector<bool, Allocator>>
  : public StdContainer<std::vector<bool, Allocator>, true, false>
{};

template<typename T, typename Allocator>
struct BufferAdapterTraits<std::vector<T, Allocator>>
  : public StdContainerForBufferAdapter<std::vector<T, Allocator>>
{};

}

}

#endif // BITSERY_TRAITS_STD_VECTOR_H
