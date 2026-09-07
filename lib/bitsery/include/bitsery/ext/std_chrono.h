// MIT License
//
// Copyright (c) 2019 Mindaugas Vinkelis
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

#ifndef BITSERY_EXT_STD_CHRONO_H
#define BITSERY_EXT_STD_CHRONO_H

#include "../traits/core/traits.h"
#include <chrono>

namespace bitsery {
namespace ext {

class StdDuration
{
public:
  template<typename Ser, typename T, typename Period, typename Fnc>
  void serialize(Ser& ser,
                 const std::chrono::duration<T, Period>& obj,
                 Fnc&& fnc) const
  {
    auto res = obj.count();
    fnc(ser, res);
  }

  template<typename Des, typename T, typename Period, typename Fnc>
  void deserialize(Des& des,
                   std::chrono::duration<T, Period>& obj,
                   Fnc&& fnc) const
  {
    T res{};
    fnc(des, res);
    obj = std::chrono::duration<T, Period>{ res };
  }
};

class StdTimePoint
{
public:
  template<typename Ser,
           typename Clock,
           typename T,
           typename Period,
           typename Fnc>
  void serialize(
    Ser& ser,
    const std::chrono::time_point<Clock, std::chrono::duration<T, Period>>& obj,
    Fnc&& fnc) const
  {
    auto res = obj.time_since_epoch().count();
    fnc(ser, res);
  }

  template<typename Des,
           typename Clock,
           typename T,
           typename Period,
           typename Fnc>
  void deserialize(
    Des& des,
    std::chrono::time_point<Clock, std::chrono::duration<T, Period>>& obj,
    Fnc&& fnc) const
  {
    T res{};
    fnc(des, res);
    auto dur = std::chrono::duration<T, Period>{ res };
    obj =
      std::chrono::time_point<Clock, std::chrono::duration<T, Period>>{ dur };
  }
};

}

namespace traits {
template<typename Rep, typename Period>
struct ExtensionTraits<ext::StdDuration, std::chrono::duration<Rep, Period>>
{
  using TValue = Rep;
  static constexpr bool SupportValueOverload = true;
  static constexpr bool SupportObjectOverload = false;
  static constexpr bool SupportLambdaOverload = false;
};

template<typename Clock, typename Rep, typename Period>
struct ExtensionTraits<
  ext::StdTimePoint,
  std::chrono::time_point<Clock, std::chrono::duration<Rep, Period>>>
{
  using TValue = Rep;
  static constexpr bool SupportValueOverload = true;
  static constexpr bool SupportObjectOverload = false;
  static constexpr bool SupportLambdaOverload = false;
};

}

}

#endif // BITSERY_EXT_STD_CHRONO_H
