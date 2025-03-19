//
// Copyright (c) 2022 Slaven Falandys
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#ifndef SFL_DETAIL_SCOPE_GUARD_HPP_INCLUDED
#define SFL_DETAIL_SCOPE_GUARD_HPP_INCLUDED

#include <utility> // forward, move

namespace sfl
{

namespace dtl
{

template <typename Lambda>
class scope_guard
{
private:

    mutable bool dismissed_;

    Lambda lambda_;

public:

    scope_guard(Lambda&& lambda)
        : dismissed_(false)
        , lambda_(std::forward<Lambda>(lambda))
    {}

    scope_guard(const scope_guard& other) = delete;

    scope_guard(scope_guard&& other)
        : dismissed_(other.dismissed_)
        , lambda_(std::move(other.lambda_))
    {
        other.dismissed_ = true;
    }

    scope_guard& operator=(const scope_guard& other) = delete;

    scope_guard& operator=(scope_guard&& other)
    {
        dismissed_ = other.dismissed_;
        lambda_ = std::move(other.lambda_);
        other.dismissed_ = true;
    }

    ~scope_guard()
    {
        if (!dismissed_)
        {
            lambda_();
        }
    }

    void dismiss() const noexcept
    {
        dismissed_ = true;
    }
};

template <typename Lambda>
scope_guard<Lambda> make_scope_guard(Lambda&& lambda)
{
    return scope_guard<Lambda>(std::forward<Lambda>(lambda));
}

} // namespace dtl

} // namespace sfl

#endif // SFL_DETAIL_SCOPE_GUARD_HPP_INCLUDED
