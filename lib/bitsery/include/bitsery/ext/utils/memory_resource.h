// MIT License
//
// Copyright (c) 2018 Mindaugas Vinkelis
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

#ifndef BITSERY_EXT_MEMORY_RESOURCE_H
#define BITSERY_EXT_MEMORY_RESOURCE_H

#include "../../details/serialization_common.h"
#include <new>

namespace bitsery {
namespace ext {
// these are very similar to c++17 polymorphic allocator and memory resource
// classes but i don't want to enforce users to use c++17 if they want to use
// pointers plus this has additional information from RTTI about runtime type
// information, might be useful working with polymorphic types. The same memory
// resource is used to allocate internal data in various contexts, (typeId is
// always 0 for internal data allocation in contexts).

class MemResourceBase
{
public:
  virtual void* allocate(size_t bytes, size_t alignment, size_t typeId) = 0;

  virtual void deallocate(void* ptr,
                          size_t bytes,
                          size_t alignment,
                          size_t typeId) noexcept = 0;

  virtual ~MemResourceBase() noexcept = default;
};

// default implementation for MemResourceBase using new and delete
class MemResourceNewDelete final : public MemResourceBase
{
public:
  inline void* allocate(size_t bytes,
                        size_t /*alignment*/,
                        size_t /*typeId*/) final
  {
    return (::operator new(bytes));
  }

  inline void deallocate(void* ptr,
                         size_t /*bytes*/,
                         size_t /*alignment*/,
                         size_t /*typeId*/) noexcept final
  {
    (::operator delete(ptr));
  }

  ~MemResourceNewDelete() noexcept final = default;
};

// these classes are used internally by bitsery extensions and and pointer utils
namespace pointer_utils {
// this is helper class that stores memory resource and knows how to
// construct/destroy objects capture this by value for custom deleters, because
// during deserialization mem resource can be changed
class PolyAllocWithTypeId final
{
public:
  constexpr PolyAllocWithTypeId(MemResourceBase* memResource = nullptr)
    : _resource{ memResource }
  {
  }

  template<typename T>
  T* allocate(size_t n, size_t typeId) const
  {
    const auto bytes = sizeof(T) * n;
    constexpr auto alignment = std::alignment_of<T>::value;
    void* ptr =
      _resource
        ? _resource->allocate(bytes, alignment, typeId)
        : ext::MemResourceNewDelete{}.allocate(bytes, alignment, typeId);
    return static_cast<T*>(ptr);
  }

  template<typename T>
  void deallocate(T* ptr, size_t n, size_t typeId) const noexcept
  {
    const auto bytes = sizeof(T) * n;
    constexpr auto alignment = std::alignment_of<T>::value;
    _resource
      ? _resource->deallocate(ptr, bytes, alignment, typeId)
      : ext::MemResourceNewDelete{}.deallocate(ptr, bytes, alignment, typeId);
  }

  template<typename T>
  T* newObject(size_t typeId) const
  {
    auto ptr = allocate<T>(1, typeId);
    return ::bitsery::Access::create<T>(ptr);
  }

  template<typename T>
  void deleteObject(T* obj, size_t typeId) const
  {
    obj->~T();
    deallocate(obj, 1, typeId);
  }

  void setMemResource(ext::MemResourceBase* resource) { _resource = resource; }

  ext::MemResourceBase* getMemResource() const { return _resource; }

  bool operator==(const PolyAllocWithTypeId& rhs) const noexcept
  {
    return _resource == rhs._resource;
  }

  bool operator!=(const PolyAllocWithTypeId& rhs) const noexcept
  {
    return !(*this == rhs);
  }

private:
  ext::MemResourceBase* _resource;
};

// this is very similar to c++17 PolymorphicAllocator
// it just wraps our PolyAllocWithTypeId and pass 0 as typeId
// and defines core functions for c++ Allocator concept,
template<class T>
class StdPolyAlloc
{
public:
  using value_type = T;

  explicit constexpr StdPolyAlloc(MemResourceBase* memResource)
    : _alloc{ memResource }
  {
  }
  explicit constexpr StdPolyAlloc(PolyAllocWithTypeId alloc)
    : _alloc{ alloc }
  {
  }

  template<typename U>
  friend class StdPolyAlloc;

  template<class U>
  constexpr explicit StdPolyAlloc(const StdPolyAlloc<U>& other) noexcept
    : _alloc{ other._alloc }
  {
  }

  T* allocate(std::size_t n) { return _alloc.allocate<T>(n, 0); }

  void deallocate(T* p, std::size_t n) noexcept
  {
    return _alloc.deallocate(p, n, 0);
  }

  template<class U>
  friend bool operator==(const StdPolyAlloc<T>& lhs,
                         const StdPolyAlloc<U>& rhs) noexcept
  {
    return lhs._alloc == rhs._alloc;
  }

  template<class U>
  friend bool operator!=(const StdPolyAlloc<T>& lhs,
                         const StdPolyAlloc<U>& rhs) noexcept
  {
    return !(lhs == rhs);
  }

private:
  PolyAllocWithTypeId _alloc;
};

}
}

}
#endif // BITSERY_EXT_MEMORY_RESOURCE_H
