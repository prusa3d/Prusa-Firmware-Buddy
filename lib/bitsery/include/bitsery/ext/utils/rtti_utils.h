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

#ifndef BITSERY_RTTI_UTILS_H
#define BITSERY_RTTI_UTILS_H

#include <cstddef>
#include <type_traits>
#include <typeinfo>

namespace bitsery {
namespace ext {

struct StandardRTTI
{

  template<typename TBase>
  static size_t get(TBase& obj)
  {
    return typeid(obj).hash_code();
  }

  template<typename TBase>
  static constexpr size_t get()
  {
    return typeid(TBase).hash_code();
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
    return std::is_polymorphic<TBase>::value;
  }
};

}
}

#endif // BITSERY_RTTI_UTILS_H
