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

#ifndef BITSERY_EXT_INHERITANCE_H
#define BITSERY_EXT_INHERITANCE_H

#include "../ext/utils/memory_resource.h"
#include "../traits/core/traits.h"
#include <unordered_set>

namespace bitsery {

namespace ext {

// required when virtual inheritance (ext::VirtualBaseClass) exists in
// serialization flow. for standard inheritance (ext::BaseClass) it is optional.
class InheritanceContext
{
public:
  explicit InheritanceContext(MemResourceBase* memResource = nullptr)
    : _virtualBases{ pointer_utils::StdPolyAlloc<const void*>{ memResource } }
  {
  }
  InheritanceContext(const InheritanceContext&) = delete;
  InheritanceContext& operator=(const InheritanceContext&) = delete;
  InheritanceContext(InheritanceContext&&) = default;
  InheritanceContext& operator=(InheritanceContext&&) = default;

  template<typename TDerived, typename TBase>
  void beginBase(const TDerived& derived, const TBase&)
  {
    if (_depth == 0) {
      const void* ptr = std::addressof(derived);
      if (_parentPtr != ptr)
        _virtualBases.clear();
      _parentPtr = ptr;
    }
    ++_depth;
  }

  template<typename TDerived, typename TBase>
  bool beginVirtualBase(const TDerived& derived, const TBase& base)
  {
    beginBase(derived, base);
    return _virtualBases.emplace(std::addressof(base)).second;
  }

  void end() { --_depth; }

private:
  // these members are required to know when we can clear _virtualBases
  size_t _depth{};
  const void* _parentPtr{};
  // add virtual bases to the list, as long as we're on the same parent
  std::unordered_set<const void*,
                     std::hash<const void*>,
                     std::equal_to<const void*>,
                     pointer_utils::StdPolyAlloc<const void*>>
    _virtualBases;
};

template<typename TBase>
class BaseClass
{
public:
  template<typename Ser, typename T, typename Fnc>
  void serialize(Ser& ser, const T& obj, Fnc&& fnc) const
  {
    auto& resObj = static_cast<const TBase&>(obj);
    if (auto ctx = ser.template contextOrNull<InheritanceContext>()) {
      ctx->beginBase(obj, resObj);
      fnc(ser, const_cast<TBase&>(resObj));
      ctx->end();
    } else {
      fnc(ser, const_cast<TBase&>(resObj));
    }
  }

  template<typename Des, typename T, typename Fnc>
  void deserialize(Des& des, T& obj, Fnc&& fnc) const
  {
    auto& resObj = static_cast<TBase&>(obj);
    if (auto ctx = des.template contextOrNull<InheritanceContext>()) {
      ctx->beginBase(obj, resObj);
      fnc(des, resObj);
      ctx->end();
    } else {
      fnc(des, resObj);
    }
  }
};

// requires InheritanceContext
template<typename TBase>
class VirtualBaseClass
{
public:
  template<typename Ser, typename T, typename Fnc>
  void serialize(Ser& ser, const T& obj, Fnc&& fnc) const
  {
    auto& ctx = ser.template context<InheritanceContext>();
    auto& resObj = static_cast<const TBase&>(obj);
    if (ctx.beginVirtualBase(obj, resObj))
      fnc(ser, const_cast<TBase&>(resObj));
    ctx.end();
  }

  template<typename Des, typename T, typename Fnc>
  void deserialize(Des& des, T& obj, Fnc&& fnc) const
  {
    auto& ctx = des.template context<InheritanceContext>();
    auto& resObj = static_cast<TBase&>(obj);
    if (ctx.beginVirtualBase(obj, resObj))
      fnc(des, resObj);
    ctx.end();
  }
};

}

namespace traits {
template<typename TBase, typename T>
struct ExtensionTraits<ext::BaseClass<TBase>, T>
{
  static_assert(std::is_base_of<TBase, T>::value, "Invalid base class");

  using TValue = TBase;
  static constexpr bool SupportValueOverload = false;
  static constexpr bool SupportObjectOverload = true;
  static constexpr bool SupportLambdaOverload = true;
};

template<typename TBase, typename T>
struct ExtensionTraits<ext::VirtualBaseClass<TBase>, T>
{
  static_assert(std::is_base_of<TBase, T>::value, "Invalid base class");

  using TValue = TBase;
  static constexpr bool SupportValueOverload = false;
  static constexpr bool SupportObjectOverload = true;
  // disable lambda overload, when serializing virtually inherited base class.
  // Only one instance of virtual base will be serialized, when using multiple
  // inheritance and it will be undefined behaviour if derived classes would
  // have different virtual base class serialization flow.
  static constexpr bool SupportLambdaOverload = false;
};
}
}

#endif // BITSERY_EXT_INHERITANCE_H
