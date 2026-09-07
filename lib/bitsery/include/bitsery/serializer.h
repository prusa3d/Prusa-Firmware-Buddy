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

#ifndef BITSERY_SERIALIZER_H
#define BITSERY_SERIALIZER_H

#include "details/serialization_common.h"
#include <cassert>

namespace bitsery {

template<typename TOutputAdapter, typename TContext = void>
class Serializer
  : public details::AdapterAndContextRef<TOutputAdapter, TContext>
{
public:
  // helper type, that always returns bit-packing enabled type, useful inside
  // serialize function when enabling bitpacking
  using BPEnabledType =
    Serializer<typename TOutputAdapter::BitPackingEnabled, TContext>;
  using TConfig = typename TOutputAdapter::TConfig;

  using details::AdapterAndContextRef<TOutputAdapter,
                                      TContext>::AdapterAndContextRef;

  /*
   * object function
   */
  template<typename T>
  void object(const T& obj)
  {
    details::SerializeFunction<Serializer, T>::invoke(*this,
                                                      const_cast<T&>(obj));
  }

  template<typename T, typename Fnc>
  void object(const T& obj, Fnc&& fnc)
  {
    fnc(*this, const_cast<T&>(obj));
  }

  /*
   * functionality, that enables simpler serialization syntax, by including
   * additional header
   */

  template<typename... TArgs>
  Serializer& operator()(TArgs&&... args)
  {
    archive(std::forward<TArgs>(args)...);
    return *this;
  }

  /*
   * value overloads
   */

  template<size_t VSIZE, typename T>
  void value(const T& v)
  {
    static_assert(details::IsFundamentalType<T>::value,
                  "Value must be integral, float or enum type.");
    using TValue = typename details::IntegralFromFundamental<T>::TValue;
    this->_adapter.template writeBytes<VSIZE>(
      reinterpret_cast<const TValue&>(v));
  }

  /*
   * enable bit-packing
   */
  template<typename Fnc>
  void enableBitPacking(Fnc&& fnc)
  {
    procEnableBitPacking(
      std::forward<Fnc>(fnc),
      std::is_same<TOutputAdapter,
                   typename TOutputAdapter::BitPackingEnabled>{},
      std::integral_constant<bool, Serializer::HasContext>{});
  }

  /*
   * extension functions
   */

  template<typename T, typename Ext, typename Fnc>
  void ext(const T& obj, const Ext& extension, Fnc&& fnc)
  {
    static_assert(details::IsExtensionTraitsDefined<Ext, T>::value,
                  "Please define ExtensionTraits");
    static_assert(traits::ExtensionTraits<Ext, T>::SupportLambdaOverload,
                  "extension doesn't support overload with lambda");
    extension.serialize(*this, obj, std::forward<Fnc>(fnc));
  }

  template<size_t VSIZE, typename T, typename Ext>
  void ext(const T& obj, const Ext& extension)
  {
    static_assert(details::IsExtensionTraitsDefined<Ext, T>::value,
                  "Please define ExtensionTraits");
    static_assert(traits::ExtensionTraits<Ext, T>::SupportValueOverload,
                  "extension doesn't support overload with `value<N>`");
    using ExtVType = typename traits::ExtensionTraits<Ext, T>::TValue;
    using VType = typename std::conditional<std::is_void<ExtVType>::value,
                                            details::DummyType,
                                            ExtVType>::type;
    extension.serialize(
      *this, obj, [](Serializer& s, VType& v) { s.value<VSIZE>(v); });
  }

  template<typename T, typename Ext>
  void ext(const T& obj, const Ext& extension)
  {
    static_assert(details::IsExtensionTraitsDefined<Ext, T>::value,
                  "Please define ExtensionTraits");
    static_assert(traits::ExtensionTraits<Ext, T>::SupportObjectOverload,
                  "extension doesn't support overload with `object`");
    using ExtVType = typename traits::ExtensionTraits<Ext, T>::TValue;
    using VType = typename std::conditional<std::is_void<ExtVType>::value,
                                            details::DummyType,
                                            ExtVType>::type;
    extension.serialize(
      *this, obj, [](Serializer& s, VType& v) { s.object(v); });
  }

  /*
   * boolValue
   */

  void boolValue(bool v)
  {
    procBoolValue(v,
                  std::is_same<TOutputAdapter,
                               typename TOutputAdapter::BitPackingEnabled>{});
  }

  /*
   * text overloads
   */

  template<size_t VSIZE, typename T>
  void text(const T& str, size_t maxSize)
  {
    static_assert(
      details::IsTextTraitsDefined<T>::value,
      "Please define TextTraits or include from <bitsery/traits/...>");
    static_assert(
      traits::ContainerTraits<T>::isResizable,
      "use text(const T&) overload without `maxSize` for static container");
    procText<VSIZE>(str, maxSize);
  }

  template<size_t VSIZE, typename T>
  void text(const T& str)
  {
    static_assert(
      details::IsTextTraitsDefined<T>::value,
      "Please define TextTraits or include from <bitsery/traits/...>");
    static_assert(!traits::ContainerTraits<T>::isResizable,
                  "use text(const T&, size_t) overload with `maxSize` for "
                  "dynamic containers");
    procText<VSIZE>(str, traits::ContainerTraits<T>::size(str));
  }

  /*
   * container overloads
   */

  // dynamic size containers

  template<typename T, typename Fnc>
  void container(const T& obj, size_t maxSize, Fnc&& fnc)
  {
    static_assert(
      details::IsContainerTraitsDefined<T>::value,
      "Please define ContainerTraits or include from <bitsery/traits/...>");
    static_assert(traits::ContainerTraits<T>::isResizable,
                  "use container(const T&, Fnc) overload without `maxSize` for "
                  "static containers");
    auto size = traits::ContainerTraits<T>::size(obj);
    (void)maxSize; // unused in release
    assert(size <= maxSize);
    details::writeSize(this->_adapter, size);
    procContainer(std::begin(obj), std::end(obj), std::forward<Fnc>(fnc));
  }

  template<size_t VSIZE, typename T>
  void container(const T& obj, size_t maxSize)
  {
    static_assert(
      details::IsContainerTraitsDefined<T>::value,
      "Please define ContainerTraits or include from <bitsery/traits/...>");
    static_assert(traits::ContainerTraits<T>::isResizable,
                  "use container(const T&) overload without `maxSize` for "
                  "static containers");
    static_assert(VSIZE > 0, "");
    auto size = traits::ContainerTraits<T>::size(obj);
    (void)maxSize; // unused in release
    assert(size <= maxSize);
    details::writeSize(this->_adapter, size);

    procContainer<VSIZE>(
      std::begin(obj),
      std::end(obj),
      std::integral_constant<bool, traits::ContainerTraits<T>::isContiguous>{});
  }

  template<typename T>
  void container(const T& obj, size_t maxSize)
  {
    static_assert(
      details::IsContainerTraitsDefined<T>::value,
      "Please define ContainerTraits or include from <bitsery/traits/...>");
    static_assert(traits::ContainerTraits<T>::isResizable,
                  "use container(const T&) overload without `maxSize` for "
                  "static containers");
    auto size = traits::ContainerTraits<T>::size(obj);
    (void)maxSize; // unused in release
    assert(size <= maxSize);
    details::writeSize(this->_adapter, size);
    procContainer(std::begin(obj), std::end(obj));
  }

  // fixed size containers

  template<
    typename T,
    typename Fnc,
    typename std::enable_if<!std::is_integral<Fnc>::value>::type* = nullptr>
  void container(const T& obj, Fnc&& fnc)
  {
    static_assert(
      details::IsContainerTraitsDefined<T>::value,
      "Please define ContainerTraits or include from <bitsery/traits/...>");
    static_assert(!traits::ContainerTraits<T>::isResizable,
                  "use container(const T&, size_t, Fnc) overload with "
                  "`maxSize` for dynamic containers");
    procContainer(std::begin(obj), std::end(obj), std::forward<Fnc>(fnc));
  }

  template<size_t VSIZE, typename T>
  void container(const T& obj)
  {
    static_assert(
      details::IsContainerTraitsDefined<T>::value,
      "Please define ContainerTraits or include from <bitsery/traits/...>");
    static_assert(!traits::ContainerTraits<T>::isResizable,
                  "use container(const T&, size_t) overload with `maxSize` for "
                  "dynamic containers");
    static_assert(VSIZE > 0, "");
    procContainer<VSIZE>(
      std::begin(obj),
      std::end(obj),
      std::integral_constant<bool, traits::ContainerTraits<T>::isContiguous>{});
  }

  template<typename T>
  void container(const T& obj)
  {
    static_assert(
      details::IsContainerTraitsDefined<T>::value,
      "Please define ContainerTraits or include from <bitsery/traits/...>");
    static_assert(!traits::ContainerTraits<T>::isResizable,
                  "use container(const T&, size_t) overload with `maxSize` for "
                  "dynamic containers");
    procContainer(std::begin(obj), std::end(obj));
  }

  // overloads for functions with explicit type size

  template<typename T>
  void value1b(T&& v)
  {
    value<1>(std::forward<T>(v));
  }

  template<typename T>
  void value2b(T&& v)
  {
    value<2>(std::forward<T>(v));
  }

  template<typename T>
  void value4b(T&& v)
  {
    value<4>(std::forward<T>(v));
  }

  template<typename T>
  void value8b(T&& v)
  {
    value<8>(std::forward<T>(v));
  }

  template<typename T>
  void value16b(T&& v)
  {
    value<16>(std::forward<T>(v));
  }

  template<typename T, typename Ext>
  void ext1b(const T& v, Ext&& extension)
  {
    ext<1, T, Ext>(v, std::forward<Ext>(extension));
  }

  template<typename T, typename Ext>
  void ext2b(const T& v, Ext&& extension)
  {
    ext<2, T, Ext>(v, std::forward<Ext>(extension));
  }

  template<typename T, typename Ext>
  void ext4b(const T& v, Ext&& extension)
  {
    ext<4, T, Ext>(v, std::forward<Ext>(extension));
  }

  template<typename T, typename Ext>
  void ext8b(const T& v, Ext&& extension)
  {
    ext<8, T, Ext>(v, std::forward<Ext>(extension));
  }

  template<typename T, typename Ext>
  void ext16b(const T& v, Ext&& extension)
  {
    ext<16, T, Ext>(v, std::forward<Ext>(extension));
  }

  template<typename T>
  void text1b(const T& str, size_t maxSize)
  {
    text<1>(str, maxSize);
  }

  template<typename T>
  void text2b(const T& str, size_t maxSize)
  {
    text<2>(str, maxSize);
  }

  template<typename T>
  void text4b(const T& str, size_t maxSize)
  {
    text<4>(str, maxSize);
  }

  template<typename T>
  void text1b(const T& str)
  {
    text<1>(str);
  }

  template<typename T>
  void text2b(const T& str)
  {
    text<2>(str);
  }

  template<typename T>
  void text4b(const T& str)
  {
    text<4>(str);
  }

  template<typename T>
  void container1b(T&& obj, size_t maxSize)
  {
    container<1>(std::forward<T>(obj), maxSize);
  }

  template<typename T>
  void container2b(T&& obj, size_t maxSize)
  {
    container<2>(std::forward<T>(obj), maxSize);
  }

  template<typename T>
  void container4b(T&& obj, size_t maxSize)
  {
    container<4>(std::forward<T>(obj), maxSize);
  }

  template<typename T>
  void container8b(T&& obj, size_t maxSize)
  {
    container<8>(std::forward<T>(obj), maxSize);
  }

  template<typename T>
  void container16b(T&& obj, size_t maxSize)
  {
    container<16>(std::forward<T>(obj), maxSize);
  }

  template<typename T>
  void container1b(T&& obj)
  {
    container<1>(std::forward<T>(obj));
  }

  template<typename T>
  void container2b(T&& obj)
  {
    container<2>(std::forward<T>(obj));
  }

  template<typename T>
  void container4b(T&& obj)
  {
    container<4>(std::forward<T>(obj));
  }

  template<typename T>
  void container8b(T&& obj)
  {
    container<8>(std::forward<T>(obj));
  }

  template<typename T>
  void container16b(T&& obj)
  {
    container<16>(std::forward<T>(obj));
  }

private:
  // process value types
  // false_type means that we must process all elements individually
  template<size_t VSIZE, typename It>
  void procContainer(It first, It last, std::false_type)
  {
    for (; first != last; ++first)
      value<VSIZE>(*first);
  }

  // process value types
  // true_type means, that we can copy whole buffer
  template<size_t VSIZE, typename It>
  void procContainer(It first, It last, std::true_type)
  {
    using TValue = typename std::decay<decltype(*first)>::type;
    using TIntegral = typename details::IntegralFromFundamental<TValue>::TValue;
    if (first != last)
      this->_adapter.template writeBuffer<VSIZE>(
        reinterpret_cast<const TIntegral*>(&(*first)),
        static_cast<size_t>(std::distance(first, last)));
  }

  // process by calling functions
  template<typename It, typename Fnc>
  void procContainer(It first, It last, Fnc fnc)
  {
    using TValue = typename std::decay<decltype(*first)>::type;
    for (; first != last; ++first) {
      fnc(*this, const_cast<TValue&>(*first));
    }
  }

  // process text,
  template<size_t VSIZE, typename T>
  void procText(const T& str, size_t maxSize)
  {
    const size_t length = traits::TextTraits<T>::length(str);
    (void)maxSize; // unused in release
    assert((length + (traits::TextTraits<T>::addNUL ? 1u : 0u)) <= maxSize);
    details::writeSize(this->_adapter, length);
    auto begin = std::begin(str);
    using diff_t =
      typename std::iterator_traits<decltype(begin)>::difference_type;
    procContainer<VSIZE>(
      begin,
      std::next(begin, static_cast<diff_t>(length)),
      std::integral_constant<bool, traits::ContainerTraits<T>::isContiguous>{});
  }

  // process object types
  template<typename It>
  void procContainer(It first, It last)
  {
    for (; first != last; ++first)
      object(*first);
  }

  // proc bool writing bit or byte, depending on if BitPackingEnabled or not
  void procBoolValue(bool v, std::true_type)
  {
    this->_adapter.writeBits(static_cast<unsigned char>(v ? 1 : 0), 1);
  }

  void procBoolValue(bool v, std::false_type)
  {
    this->_adapter.template writeBytes<1>(
      static_cast<unsigned char>(v ? 1 : 0));
  }

  // enable bit-packing or do nothing if it is already enabled
  template<typename Fnc, typename HasContext>
  void procEnableBitPacking(const Fnc& fnc, std::true_type, HasContext)
  {
    fnc(*this);
  }

  template<typename Fnc>
  void procEnableBitPacking(const Fnc& fnc, std::false_type, std::true_type)
  {
    BPEnabledType ser{ this->_context, this->_adapter };
    fnc(ser);
  }

  template<typename Fnc>
  void procEnableBitPacking(const Fnc& fnc, std::false_type, std::false_type)
  {
    BPEnabledType ser{ this->_adapter };
    fnc(ser);
  }

  // these are dummy functions for extensions that have TValue = void
  void object(const details::DummyType&) {}

  template<size_t VSIZE>
  void value(const details::DummyType&)
  {
  }

  template<typename T, typename... TArgs>
  void archive(T&& head, TArgs&&... tail)
  {
    // serialize object
    details::BriefSyntaxFunction<Serializer, T>::invoke(*this,
                                                        std::forward<T>(head));
    // expand other elements
    archive(std::forward<TArgs>(tail)...);
  }
  // dummy function, that stops archive variadic arguments expansion
  void archive() {}
};

// helper function that set ups all the basic steps and after serialziation
// returns serialized bytes count
template<typename OutputAdapter, typename T>
size_t
quickSerialization(OutputAdapter adapter, const T& value)
{
  Serializer<OutputAdapter> ser{ std::move(adapter) };
  ser.object(value);
  ser.adapter().flush();
  return ser.adapter().writtenBytesCount();
}

template<typename Context, typename OutputAdapter, typename T>
size_t
quickSerialization(Context& ctx, OutputAdapter adapter, const T& value)
{
  Serializer<OutputAdapter, Context> ser{ ctx, std::move(adapter) };
  ser.object(value);
  ser.adapter().flush();
  return ser.adapter().writtenBytesCount();
}

}
#endif // BITSERY_SERIALIZER_H
