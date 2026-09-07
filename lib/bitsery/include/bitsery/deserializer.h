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

#ifndef BITSERY_DESERIALIZER_H
#define BITSERY_DESERIALIZER_H

#include "details/serialization_common.h"

namespace bitsery {

template<typename TInputAdapter, typename TContext = void>
class Deserializer
  : public details::AdapterAndContextRef<TInputAdapter, TContext>
{
public:
  // helper type, that always returns bit-packing enabled type, useful inside
  // deserialize function when enabling bitpacking
  using BPEnabledType =
    Deserializer<typename TInputAdapter::BitPackingEnabled, TContext>;
  using TConfig = typename TInputAdapter::TConfig;

  using details::AdapterAndContextRef<TInputAdapter,
                                      TContext>::AdapterAndContextRef;

  /*
   * object function
   */

  template<typename T>
  void object(T&& obj)
  {
    details::SerializeFunction<Deserializer, T>::invoke(*this,
                                                        std::forward<T>(obj));
  }

  template<typename T, typename Fnc>
  void object(T&& obj, Fnc&& fnc)
  {
    fnc(*this, std::forward<T>(obj));
  }

  /*
   * functionality, that enables simpler serialization syntax, by including
   * additional header
   */

  template<typename... TArgs>
  Deserializer& operator()(TArgs&&... args)
  {
    archive(std::forward<TArgs>(args)...);
    return *this;
  }

  /*
   * value
   */

  template<size_t VSIZE, typename T>
  void value(T& v)
  {
    static_assert(details::IsFundamentalType<T>::value,
                  "Value must be integral, float or enum type.");
    using TValue = typename details::IntegralFromFundamental<T>::TValue;
    this->_adapter.template readBytes<VSIZE>(reinterpret_cast<TValue&>(v));
  }

  /*
   * enable bit-packing
   */
  template<typename Fnc>
  void enableBitPacking(Fnc&& fnc)
  {
    procEnableBitPacking(
      std::forward<Fnc>(fnc),
      std::is_same<TInputAdapter, typename TInputAdapter::BitPackingEnabled>{});
  }

  /*
   * extension functions
   */

  template<typename T, typename Ext, typename Fnc>
  void ext(T& obj, const Ext& extension, Fnc&& fnc)
  {
    static_assert(details::IsExtensionTraitsDefined<Ext, T>::value,
                  "Please define ExtensionTraits");
    static_assert(traits::ExtensionTraits<Ext, T>::SupportLambdaOverload,
                  "extension doesn't support overload with lambda");
    extension.deserialize(*this, obj, std::forward<Fnc>(fnc));
  }

  template<size_t VSIZE, typename T, typename Ext>
  void ext(T& obj, const Ext& extension)
  {
    static_assert(details::IsExtensionTraitsDefined<Ext, T>::value,
                  "Please define ExtensionTraits");
    static_assert(traits::ExtensionTraits<Ext, T>::SupportValueOverload,
                  "extension doesn't support overload with `value<N>`");
    using ExtVType = typename traits::ExtensionTraits<Ext, T>::TValue;
    using VType = typename std::conditional<std::is_void<ExtVType>::value,
                                            details::DummyType,
                                            ExtVType>::type;
    extension.deserialize(
      *this, obj, [](Deserializer& s, VType& v) { s.value<VSIZE>(v); });
  }

  template<typename T, typename Ext>
  void ext(T& obj, const Ext& extension)
  {
    static_assert(details::IsExtensionTraitsDefined<Ext, T>::value,
                  "Please define ExtensionTraits");
    static_assert(traits::ExtensionTraits<Ext, T>::SupportObjectOverload,
                  "extension doesn't support overload with `object`");
    using ExtVType = typename traits::ExtensionTraits<Ext, T>::TValue;
    using VType = typename std::conditional<std::is_void<ExtVType>::value,
                                            details::DummyType,
                                            ExtVType>::type;
    extension.deserialize(
      *this, obj, [](Deserializer& s, VType& v) { s.object(v); });
  }

  /*
   * boolValue
   */
  void boolValue(bool& v)
  {
    procBoolValue(
      v,
      std::is_same<TInputAdapter, typename TInputAdapter::BitPackingEnabled>{},
      std::integral_constant<bool, TInputAdapter::TConfig::CheckDataErrors>{});
  }

  /*
   * text overloads
   */

  template<size_t VSIZE, typename T>
  void text(T& str, size_t maxSize)
  {
    static_assert(
      details::IsTextTraitsDefined<T>::value,
      "Please define TextTraits or include from <bitsery/traits/...>");
    static_assert(
      traits::ContainerTraits<T>::isResizable,
      "use text(T&) overload without `maxSize` for static containers");
    size_t length;
    readSize(length, maxSize);
    traits::ContainerTraits<T>::resize(
      str, length + (traits::TextTraits<T>::addNUL ? 1u : 0u));
    procText<VSIZE>(str, length);
  }

  template<size_t VSIZE, typename T>
  void text(T& str)
  {
    static_assert(
      details::IsTextTraitsDefined<T>::value,
      "Please define TextTraits or include from <bitsery/traits/...>");
    static_assert(
      !traits::ContainerTraits<T>::isResizable,
      "use text(T&, size_t) overload with `maxSize` for dynamic containers");
    size_t length;
    readSize(length, traits::ContainerTraits<T>::size(str));
    procText<VSIZE>(str, length);
  }

  /*
   * container overloads
   */

  // dynamic size containers

  template<typename T, typename Fnc>
  void container(T& obj, size_t maxSize, Fnc&& fnc)
  {
    static_assert(
      details::IsContainerTraitsDefined<T>::value,
      "Please define ContainerTraits or include from <bitsery/traits/...>");
    static_assert(
      traits::ContainerTraits<T>::isResizable,
      "use container(T&) overload without `maxSize` for static containers");
    size_t size{};
    readSize(size, maxSize);
    traits::ContainerTraits<T>::resize(obj, size);
    procContainer(std::begin(obj), std::end(obj), std::forward<Fnc>(fnc));
  }

  template<size_t VSIZE, typename T>
  void container(T& obj, size_t maxSize)
  {
    static_assert(
      details::IsContainerTraitsDefined<T>::value,
      "Please define ContainerTraits or include from <bitsery/traits/...>");
    static_assert(
      traits::ContainerTraits<T>::isResizable,
      "use container(T&) overload without `maxSize` for static containers");
    size_t size{};
    readSize(size, maxSize);
    traits::ContainerTraits<T>::resize(obj, size);
    procContainer<VSIZE>(
      std::begin(obj),
      std::end(obj),
      std::integral_constant<bool, traits::ContainerTraits<T>::isContiguous>{});
  }

  template<typename T>
  void container(T& obj, size_t maxSize)
  {
    static_assert(
      details::IsContainerTraitsDefined<T>::value,
      "Please define ContainerTraits or include from <bitsery/traits/...>");
    static_assert(
      traits::ContainerTraits<T>::isResizable,
      "use container(T&) overload without `maxSize` for static containers");
    size_t size{};
    readSize(size, maxSize);
    traits::ContainerTraits<T>::resize(obj, size);
    procContainer(std::begin(obj), std::end(obj));
  }
  // fixed size containers

  template<
    typename T,
    typename Fnc,
    typename std::enable_if<!std::is_integral<Fnc>::value>::type* = nullptr>
  void container(T& obj, Fnc&& fnc)
  {
    static_assert(
      details::IsContainerTraitsDefined<T>::value,
      "Please define ContainerTraits or include from <bitsery/traits/...>");
    static_assert(!traits::ContainerTraits<T>::isResizable,
                  "use container(T&, size_t, Fnc) overload with `maxSize` for "
                  "dynamic containers");
    procContainer(std::begin(obj), std::end(obj), std::forward<Fnc>(fnc));
  }

  template<size_t VSIZE, typename T>
  void container(T& obj)
  {
    static_assert(
      details::IsContainerTraitsDefined<T>::value,
      "Please define ContainerTraits or include from <bitsery/traits/...>");
    static_assert(!traits::ContainerTraits<T>::isResizable,
                  "use container(T&, size_t) overload with `maxSize` for "
                  "dynamic containers");
    static_assert(VSIZE > 0, "");
    procContainer<VSIZE>(
      std::begin(obj),
      std::end(obj),
      std::integral_constant<bool, traits::ContainerTraits<T>::isContiguous>{});
  }

  template<typename T>
  void container(T& obj)
  {
    static_assert(
      details::IsContainerTraitsDefined<T>::value,
      "Please define ContainerTraits or include from <bitsery/traits/...>");
    static_assert(!traits::ContainerTraits<T>::isResizable,
                  "use container(T&, size_t) overload with `maxSize` for "
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
  void ext1b(T& v, Ext&& extension)
  {
    ext<1, T, Ext>(v, std::forward<Ext>(extension));
  }

  template<typename T, typename Ext>
  void ext2b(T& v, Ext&& extension)
  {
    ext<2, T, Ext>(v, std::forward<Ext>(extension));
  }

  template<typename T, typename Ext>
  void ext4b(T& v, Ext&& extension)
  {
    ext<4, T, Ext>(v, std::forward<Ext>(extension));
  }

  template<typename T, typename Ext>
  void ext8b(T& v, Ext&& extension)
  {
    ext<8, T, Ext>(v, std::forward<Ext>(extension));
  }

  template<typename T, typename Ext>
  void ext16b(T& v, Ext&& extension)
  {
    ext<16, T, Ext>(v, std::forward<Ext>(extension));
  }

  template<typename T>
  void text1b(T& str, size_t maxSize)
  {
    text<1>(str, maxSize);
  }

  template<typename T>
  void text2b(T& str, size_t maxSize)
  {
    text<2>(str, maxSize);
  }

  template<typename T>
  void text4b(T& str, size_t maxSize)
  {
    text<4>(str, maxSize);
  }

  template<typename T>
  void text1b(T& str)
  {
    text<1>(str);
  }

  template<typename T>
  void text2b(T& str)
  {
    text<2>(str);
  }

  template<typename T>
  void text4b(T& str)
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
  void readSize(size_t& size, size_t maxSize)
  {
    details::readSize(
      this->_adapter,
      size,
      maxSize,
      std::integral_constant<bool, TInputAdapter::TConfig::CheckDataErrors>{});
  }

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
    if (first != last) {
      const auto distance = std::distance(first, last);
      this->_adapter.template readBuffer<VSIZE>(
        reinterpret_cast<TIntegral*>(&(*first)), static_cast<size_t>(distance));
    }
  }

  // process by calling functions
  template<typename It, typename Fnc>
  void procContainer(It first, It last, Fnc fnc)
  {
    for (; first != last; ++first)
      fnc(*this, *first);
  }

  // process object types
  template<typename It>
  void procContainer(It first, It last)
  {
    for (; first != last; ++first)
      object(*first);
  }

  template<size_t VSIZE, typename T>
  void procText(T& str, size_t length)
  {
    auto begin = std::begin(str);
    // end of string, not end of container
    using diff_t =
      typename std::iterator_traits<decltype(begin)>::difference_type;
    auto end = std::next(begin, static_cast<diff_t>(length));
    procContainer<VSIZE>(
      begin,
      end,
      std::integral_constant<bool, traits::ContainerTraits<T>::isContiguous>{});
    // null terminated character at the end
    if (traits::TextTraits<T>::addNUL)
      *end = {};
  }

  // proc bool writing bit or byte, depending on if BitPackingEnabled or not
  template<typename HandleDataErrors>
  void procBoolValue(bool& v, std::true_type, HandleDataErrors)
  {
    uint8_t tmp{};
    this->_adapter.readBits(tmp, 1);
    v = tmp == 1;
  }

  void procBoolValue(bool& v, std::false_type, std::true_type)
  {
    uint8_t tmp{};
    this->_adapter.template readBytes<1>(tmp);
    if (tmp > 1)
      this->_adapter.error(ReaderError::InvalidData);
    v = tmp == 1;
  }

  void procBoolValue(bool& v, std::false_type, std::false_type)
  {
    uint8_t tmp{};
    this->_adapter.template readBytes<1>(tmp);
    v = tmp > 0;
  }

  // enable bit-packing or do nothing if it is already enabled
  template<typename Fnc>
  void procEnableBitPacking(const Fnc& fnc, std::true_type)
  {
    fnc(*this);
  }

  template<typename Fnc>
  void procEnableBitPacking(const Fnc& fnc, std::false_type)
  {
    auto des = createWithContext(
      std::integral_constant<bool, Deserializer::HasContext>{});
    fnc(des);
  }

  BPEnabledType createWithContext(std::true_type)
  {
    return BPEnabledType{ this->_context, this->_adapter };
  }

  BPEnabledType createWithContext(std::false_type)
  {
    return BPEnabledType{ this->_adapter };
  }

  // these are dummy functions for extensions that have TValue = void
  void object(details::DummyType&) {}

  template<size_t VSIZE>
  void value(details::DummyType&)
  {
  }

  template<typename T, typename... TArgs>
  void archive(T&& head, TArgs&&... tail)
  {
    // serialize object
    details::BriefSyntaxFunction<Deserializer, T>::invoke(
      *this, std::forward<T>(head));
    // expand other elements
    archive(std::forward<TArgs>(tail)...);
  }

  // dummy function, that stops archive variadic arguments expansion
  void archive() {}
};

// helper type

// helper function that set ups all the basic steps and after deserialziation
// returns status
template<typename InputAdapter, typename T>
std::pair<ReaderError, bool>
quickDeserialization(InputAdapter adapter, T& value)
{
  Deserializer<InputAdapter> des{ std::move(adapter) };
  des.object(value);
  return { des.adapter().error(), des.adapter().isCompletedSuccessfully() };
}

template<typename Context, typename InputAdapter, typename T>
std::pair<ReaderError, bool>
quickDeserialization(Context& ctx, InputAdapter adapter, T& value)
{
  Deserializer<InputAdapter, Context> des{ ctx, std::move(adapter) };
  des.object(value);
  return { des.adapter().error(), des.adapter().isCompletedSuccessfully() };
}

}

#endif // BITSERY_DESERIALIZER_H
