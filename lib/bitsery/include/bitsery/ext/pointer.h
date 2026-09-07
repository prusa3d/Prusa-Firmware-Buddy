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

#ifndef BITSERY_EXT_POINTER_H
#define BITSERY_EXT_POINTER_H

#include "../traits/core/traits.h"
#include "utils/pointer_utils.h"
#include "utils/polymorphism_utils.h"
#include "utils/rtti_utils.h"
#include <cassert>

namespace bitsery {

namespace ext {

namespace pointer_details {

template<typename T>
struct PtrOwnerManager
{
  static_assert(std::is_pointer<T>::value, "");

  using TElement = typename std::remove_pointer<T>::type;

  static TElement* getPtr(T& obj) { return obj; }

  static constexpr PointerOwnershipType getOwnership()
  {
    return PointerOwnershipType::Owner;
  }

  static void create(T& obj,
                     pointer_utils::PolyAllocWithTypeId alloc,
                     size_t typeId)
  {
    obj = alloc.newObject<TElement>(typeId);
  }

  static void createPolymorphic(
    T& obj,
    pointer_utils::PolyAllocWithTypeId alloc,
    const std::shared_ptr<PolymorphicHandlerBase>& handler)
  {
    obj = static_cast<TElement*>(handler->create(alloc));
  }

  static void destroy(T& obj,
                      pointer_utils::PolyAllocWithTypeId alloc,
                      size_t typeId)
  {
    alloc.deleteObject(obj, typeId);
    obj = nullptr;
  }

  static void destroyPolymorphic(
    T& obj,
    pointer_utils::PolyAllocWithTypeId alloc,
    const std::shared_ptr<PolymorphicHandlerBase>& handler)
  {
    handler->destroy(alloc, obj);
    obj = nullptr;
  }
};

template<typename T>
struct PtrObserverManager
{
  static_assert(std::is_pointer<T>::value, "");

  using TElement = typename std::remove_pointer<T>::type;

  static TElement* getPtr(T& obj) { return obj; }

  static constexpr PointerOwnershipType getOwnership()
  {
    return PointerOwnershipType::Observer;
  }

  // pure observer doesn't have create/createPolymorphic methods, but instead
  // returns reference to pointer which gets updated later
  static TElement*& getPtrRef(T& obj) { return obj; }

  static void destroy(T& obj, MemResourceBase*, size_t) { obj = nullptr; }

  static void destroyPolymorphic(T& obj,
                                 MemResourceBase*,
                                 PolymorphicHandlerBase&)
  {
    obj = nullptr;
  }
};

template<typename T>
struct NonPtrManager
{

  static_assert(!std::is_pointer<T>::value, "");

  using TElement = T;

  static TElement* getPtr(T& obj) { return &obj; }

  static constexpr PointerOwnershipType getOwnership()
  {
    return PointerOwnershipType::Owner;
  }

  // this code is unreachable for reference type, but is necessary to compile
  // LCOV_EXCL_START

  static void create(T&, MemResourceBase*, size_t) {}

  static void createPolymorphic(T&, MemResourceBase*, PolymorphicHandlerBase&)
  {
  }

  static void destroy(T&, MemResourceBase*, size_t) {}

  static void destroyPolymorphic(T&, MemResourceBase*, PolymorphicHandlerBase&)
  {
  }
  // LCOV_EXCL_STOP
};

// this class is used by NonPtrManager
struct NoRTTI
{
  template<typename TBase>
  static size_t get(TBase&)
  {
    return 0;
  }

  template<typename TBase>
  static constexpr size_t get()
  {
    return 0;
  }

  template<typename TBase, typename TDerived>
  static constexpr TDerived* cast(TBase* obj)
  {
    static_assert(!std::is_pointer<TDerived>::value, "");
    return dynamic_cast<TDerived*>(obj);
  }

  template<typename TBase>
  static constexpr bool isPolymorphic()
  {
    return false;
  }
};

}

template<typename RTTI>
using PointerOwnerBase =
  pointer_utils::PointerObjectExtensionBase<pointer_details::PtrOwnerManager,
                                            PolymorphicContext,
                                            RTTI>;

using PointerOwner = PointerOwnerBase<StandardRTTI>;

using PointerObserver =
  pointer_utils::PointerObjectExtensionBase<pointer_details::PtrObserverManager,
                                            PolymorphicContext,
                                            pointer_details::NoRTTI>;

// inherit from PointerObjectExtensionBase in order to specify
// PointerType::NotNull
class ReferencedByPointer
  : public pointer_utils::PointerObjectExtensionBase<
      pointer_details::NonPtrManager,
      PolymorphicContext,
      pointer_details::NoRTTI>
{
public:
  ReferencedByPointer()
    : pointer_utils::PointerObjectExtensionBase<pointer_details::NonPtrManager,
                                                PolymorphicContext,
                                                pointer_details::NoRTTI>(
        PointerType::NotNull)
  {
  }
};

}

namespace traits {

template<typename T, typename RTTI>
struct ExtensionTraits<ext::PointerOwnerBase<RTTI>, T*>
{
  using TValue = T;
  static constexpr bool SupportValueOverload = true;
  static constexpr bool SupportObjectOverload = true;
  // if underlying type is not polymorphic, then we can enable lambda syntax
  static constexpr bool SupportLambdaOverload =
    !RTTI::template isPolymorphic<TValue>();
};

template<typename T>
struct ExtensionTraits<ext::PointerObserver, T*>
{
  // although pointer observer doesn't serialize anything, but we still add
  // value overload support to be consistent with pointer owners observer only
  // writes/reads pointer id from pointer linking context
  using TValue = T;
  static constexpr bool SupportValueOverload = true;
  static constexpr bool SupportObjectOverload = true;
  static constexpr bool SupportLambdaOverload = false;
};

template<typename T>
struct ExtensionTraits<ext::ReferencedByPointer, T>
{
  // allow everything, because it is serialized as regular type, except it also
  // creates pointerId that is required by NonOwningPointer to work
  using TValue = T;
  static constexpr bool SupportValueOverload = true;
  static constexpr bool SupportObjectOverload = true;
  static constexpr bool SupportLambdaOverload = true;
};
}

}

#endif // BITSERY_EXT_POINTER_H
