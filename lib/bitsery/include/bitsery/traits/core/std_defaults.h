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

#ifndef BITSERY_TRAITS_CORE_STD_DEFAULTS_H
#define BITSERY_TRAITS_CORE_STD_DEFAULTS_H

#include "../../bitsery.h"
#include "../../details/serialization_common.h"
#include "traits.h"

namespace bitsery {
namespace traits {

/*
 * these are helper types, to easier write specializations for std types
 */

template<typename T, bool Resizable, bool Contiguous>
struct StdContainer
{
  using TValue = typename T::value_type;
  static constexpr bool isResizable = Resizable;
  static constexpr bool isContiguous = Contiguous;
  static size_t size(const T& container) { return container.size(); }
};

// specialization for resizable
template<typename T, bool Contiguous>
struct StdContainer<T, true, Contiguous>
{
  using TValue = typename T::value_type;
  static constexpr bool isResizable = true;
  static constexpr bool isContiguous = Contiguous;
  static size_t size(const T& container) { return container.size(); }
  static void resize(T& container, size_t size)
  {
    resizeImpl(container, size, std::is_default_constructible<TValue>{});
  }

private:
  using diff_t = typename T::difference_type;

  static void resizeImpl(T& container, size_t size, std::true_type)
  {
    container.resize(size);
  }
  static void resizeImpl(T& container, size_t newSize, std::false_type)
  {
    const auto oldSize = size(container);
    for (auto it = oldSize; it < newSize; ++it) {
      container.push_back(::bitsery::Access::create<TValue>());
    }
    if (oldSize > newSize) {
      container.erase(
        std::next(std::begin(container), static_cast<diff_t>(newSize)),
        std::end(container));
    }
  }
};

template<typename T, bool Resizable = ContainerTraits<T>::isResizable>
struct StdContainerForBufferAdapter
{
  using TIterator = typename T::iterator;
  using TConstIterator = typename T::const_iterator;
  using TValue = typename ContainerTraits<T>::TValue;
};

// specialization for resizable buffers
template<typename T>
struct StdContainerForBufferAdapter<T, true>
{
  using TIterator = typename T::iterator;
  using TConstIterator = typename T::const_iterator;
  using TValue = typename ContainerTraits<T>::TValue;

  static void increaseBufferSize(T& container,
                                 size_t /*currSize*/,
                                 size_t minSize)
  {
    // since we're writing to buffer use different resize strategy than default
    // implementation when small size grow faster, to avoid thouse 2/4/8/16...
    // byte allocations
    auto newSize =
      static_cast<size_t>(static_cast<double>(container.size()) * 1.5) + 128;
    // make data cache friendly
    newSize -= newSize % 64; // 64 is cache line size
    auto resize =
      (std::max)(newSize > minSize ? newSize : minSize, container.capacity());
    BITSERY_ASSUME(resize >= container.size());
    BITSERY_ASSUME(resize >= container.capacity());
    container.resize(resize);
  }
};

}
}

#endif // BITSERY_TRAITS_CORE_STD_DEFAULTS_H
