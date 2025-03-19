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

#ifndef SFL_MULTIMAP_HPP_INCLUDED
#define SFL_MULTIMAP_HPP_INCLUDED

#include <sfl/detail/allocator_traits.hpp>
#include <sfl/detail/container_compatible_range.hpp>
#include <sfl/detail/cpp.hpp>
#include <sfl/detail/functional.hpp>
#include <sfl/detail/rb_tree.hpp>
#include <sfl/detail/tags.hpp>
#include <sfl/detail/type_traits.hpp>

#include <cstddef>          // size_t
#include <functional>       // equal_to, less
#include <initializer_list> // initializer_list
#include <memory>           // allocator
#include <type_traits>      // is_same, is_nothrow_xxxxx
#include <utility>          // forward, move, pair

namespace sfl
{

template < typename Key,
           typename T,
           typename Compare = std::less<Key>,
           typename Allocator = std::allocator<std::pair<const Key, T>> >
class multimap
{
    static_assert
    (
        std::is_same<typename Allocator::value_type, std::pair<const Key, T>>::value,
        "Allocator::value_type must be std::pair<const Key, T>."
    );

public:

    using allocator_type = Allocator;
    using key_type       = Key;
    using mapped_type    = T;
    using value_type     = std::pair<const Key, T>;
    using key_compare    = Compare;

    class value_compare : protected key_compare
    {
        friend class multimap;

    private:

        value_compare(const key_compare& c) : key_compare(c)
        {}

    public:

        bool operator()(const value_type& x, const value_type& y) const
        {
            return key_compare::operator()(x.first, y.first);
        }
    };

private:

    using tree_type = sfl::dtl::rb_tree
    <
        key_type,
        value_type,
        sfl::dtl::first,
        key_compare,
        typename sfl::dtl::allocator_traits<allocator_type>::template rebind_alloc<value_type>,
        multimap
    >;

    tree_type tree_;

public:

    using size_type              = typename sfl::dtl::allocator_traits<typename tree_type::allocator_type>::size_type;
    using difference_type        = typename sfl::dtl::allocator_traits<typename tree_type::allocator_type>::difference_type;
    using reference              = value_type&;
    using const_reference        = const value_type&;
    using pointer                = typename sfl::dtl::allocator_traits<typename tree_type::allocator_type>::pointer;
    using const_pointer          = typename sfl::dtl::allocator_traits<typename tree_type::allocator_type>::const_pointer;
    using iterator               = typename tree_type::iterator;
    using const_iterator         = typename tree_type::const_iterator;
    using reverse_iterator       = typename tree_type::reverse_iterator;
    using const_reverse_iterator = typename tree_type::const_reverse_iterator;

public:

    //
    // ---- CONSTRUCTION AND DESTRUCTION --------------------------------------
    //

    multimap() noexcept
    (
        std::is_nothrow_default_constructible<Allocator>::value &&
        std::is_nothrow_default_constructible<Compare>::value
    )
        : tree_()
    {}

    explicit multimap(const Compare& comp) noexcept
    (
        std::is_nothrow_default_constructible<Allocator>::value &&
        std::is_nothrow_copy_constructible<Compare>::value
    )
        : tree_(comp)
    {}

    explicit multimap(const Allocator& alloc) noexcept
    (
        std::is_nothrow_copy_constructible<Allocator>::value &&
        std::is_nothrow_default_constructible<Compare>::value
    )
        : tree_(alloc)
    {}

    explicit multimap(const Compare& comp, const Allocator& alloc) noexcept
    (
        std::is_nothrow_copy_constructible<Allocator>::value &&
        std::is_nothrow_copy_constructible<Compare>::value
    )
        : tree_(comp, alloc)
    {}

    template <typename InputIt,
              sfl::dtl::enable_if_t<sfl::dtl::is_input_iterator<InputIt>::value>* = nullptr>
    multimap(InputIt first, InputIt last)
        : tree_()
    {
        insert(first, last);
    }

    template <typename InputIt,
              sfl::dtl::enable_if_t<sfl::dtl::is_input_iterator<InputIt>::value>* = nullptr>
    multimap(InputIt first, InputIt last, const Compare& comp)
        : tree_(comp)
    {
        insert(first, last);
    }

    template <typename InputIt,
              sfl::dtl::enable_if_t<sfl::dtl::is_input_iterator<InputIt>::value>* = nullptr>
    multimap(InputIt first, InputIt last, const Allocator& alloc)
        : tree_(alloc)
    {
        insert(first, last);
    }

    template <typename InputIt,
              sfl::dtl::enable_if_t<sfl::dtl::is_input_iterator<InputIt>::value>* = nullptr>
    multimap(InputIt first, InputIt last, const Compare& comp, const Allocator& alloc)
        : tree_(comp, alloc)
    {
        insert(first, last);
    }

    multimap(std::initializer_list<value_type> ilist)
        : multimap(ilist.begin(), ilist.end())
    {}

    multimap(std::initializer_list<value_type> ilist, const Compare& comp)
        : multimap(ilist.begin(), ilist.end(), comp)
    {}

    multimap(std::initializer_list<value_type> ilist, const Allocator& alloc)
        : multimap(ilist.begin(), ilist.end(), alloc)
    {}

    multimap(std::initializer_list<value_type> ilist, const Compare& comp, const Allocator& alloc)
        : multimap(ilist.begin(), ilist.end(), comp, alloc)
    {}

    multimap(const multimap& other)
        : tree_(other.tree_)
    {}

    multimap(const multimap& other, const Allocator& alloc)
        : tree_(other.tree_, alloc)
    {}

    multimap(multimap&& other)
        : tree_(std::move(other.tree_))
    {}

    multimap(multimap&& other, const Allocator& alloc)
        : tree_(std::move(other.tree_), alloc)
    {}

#if SFL_CPP_VERSION >= SFL_CPP_20

    template <sfl::dtl::container_compatible_range<value_type> Range>
    multimap(sfl::from_range_t, Range&& range)
        : tree_()
    {
        insert_range(std::forward<Range>(range));
    }

    template <sfl::dtl::container_compatible_range<value_type> Range>
    multimap(sfl::from_range_t, Range&& range, const Compare& comp)
        : tree_(comp)
    {
        insert_range(std::forward<Range>(range));
    }

    template <sfl::dtl::container_compatible_range<value_type> Range>
    multimap(sfl::from_range_t, Range&& range, const Allocator& alloc)
        : tree_(alloc)
    {
        insert_range(std::forward<Range>(range));
    }

    template <sfl::dtl::container_compatible_range<value_type> Range>
    multimap(sfl::from_range_t, Range&& range, const Compare& comp, const Allocator& alloc)
        : tree_(comp, alloc)
    {
        insert_range(std::forward<Range>(range));
    }

#else // before C++20

    template <typename Range>
    multimap(sfl::from_range_t, Range&& range)
        : tree_()
    {
        insert_range(std::forward<Range>(range));
    }

    template <typename Range>
    multimap(sfl::from_range_t, Range&& range, const Compare& comp)
        : tree_(comp)
    {
        insert_range(std::forward<Range>(range));
    }

    template <typename Range>
    multimap(sfl::from_range_t, Range&& range, const Allocator& alloc)
        : tree_(alloc)
    {
        insert_range(std::forward<Range>(range));
    }

    template <typename Range>
    multimap(sfl::from_range_t, Range&& range, const Compare& comp, const Allocator& alloc)
        : tree_(comp, alloc)
    {
        insert_range(std::forward<Range>(range));
    }

#endif // before C++20

    ~multimap()
    {}

    //
    // ---- ASSIGNMENT --------------------------------------------------------
    //

    multimap& operator=(const multimap& other)
    {
        tree_.operator=(other.tree_);
        return *this;
    }

    multimap& operator=(multimap&& other)
    {
        tree_.operator=(std::move(other.tree_));
        return *this;
    }

    multimap& operator=(std::initializer_list<value_type> ilist)
    {
        tree_.assign_range_equal(ilist.begin(), ilist.end());
        return *this;
    }

    //
    // ---- ALLOCATOR ---------------------------------------------------------
    //

    SFL_NODISCARD
    allocator_type get_allocator() const noexcept
    {
        return allocator_type(tree_.ref_to_node_alloc());
    }

    //
    // ---- KEY COMPARE -------------------------------------------------------
    //

    SFL_NODISCARD
    key_compare key_comp() const
    {
        return key_compare(tree_.ref_to_comp());
    }

    //
    // ---- VALUE COMPARE -----------------------------------------------------
    //

    SFL_NODISCARD
    value_compare value_comp() const
    {
        return value_compare(tree_.ref_to_comp());
    }

    //
    // ---- ITERATORS ---------------------------------------------------------
    //

    SFL_NODISCARD
    iterator begin() noexcept
    {
        return tree_.begin();
    }

    SFL_NODISCARD
    const_iterator begin() const noexcept
    {
        return tree_.begin();
    }

    SFL_NODISCARD
    const_iterator cbegin() const noexcept
    {
        return tree_.cbegin();
    }

    SFL_NODISCARD
    iterator end() noexcept
    {
        return tree_.end();
    }

    SFL_NODISCARD
    const_iterator end() const noexcept
    {
        return tree_.end();
    }

    SFL_NODISCARD
    const_iterator cend() const noexcept
    {
        return tree_.cend();
    }

    SFL_NODISCARD
    reverse_iterator rbegin() noexcept
    {
        return tree_.rbegin();
    }

    SFL_NODISCARD
    const_reverse_iterator rbegin() const noexcept
    {
        return tree_.rbegin();
    }

    SFL_NODISCARD
    const_reverse_iterator crbegin() const noexcept
    {
        return tree_.crbegin();
    }

    SFL_NODISCARD
    reverse_iterator rend() noexcept
    {
        return tree_.rend();
    }

    SFL_NODISCARD
    const_reverse_iterator rend() const noexcept
    {
        return tree_.rend();
    }

    SFL_NODISCARD
    const_reverse_iterator crend() const noexcept
    {
        return tree_.crend();
    }

    //
    // ---- SIZE AND CAPACITY -------------------------------------------------
    //

    SFL_NODISCARD
    bool empty() const noexcept
    {
        return tree_.empty();
    }

    SFL_NODISCARD
    size_type size() const noexcept
    {
        return tree_.size();
    }

    SFL_NODISCARD
    size_type max_size() const noexcept
    {
        return tree_.max_size();
    }

    //
    // ---- MODIFIERS ---------------------------------------------------------
    //

    void clear()
    {
        tree_.clear();
    }

    template <typename... Args>
    iterator emplace(Args&&... args)
    {
        return tree_.emplace_equal(std::forward<Args>(args)...);
    }

    template <typename... Args>
    iterator emplace_hint(const_iterator hint, Args&&... args)
    {
        return tree_.emplace_hint_equal(hint, std::forward<Args>(args)...);
    }

    iterator insert(const value_type& value)
    {
        return tree_.insert_equal(value);
    }

    iterator insert(value_type&& value)
    {
        return tree_.insert_equal(std::move(value));
    }

    template <typename P,
              sfl::dtl::enable_if_t<std::is_constructible<value_type, P&&>::value>* = nullptr>
    iterator insert(P&& value)
    {
        return tree_.insert_equal(std::forward<P>(value));
    }

    iterator insert(const_iterator hint, const value_type& value)
    {
        return tree_.insert_hint_equal(hint, value);
    }

    iterator insert(const_iterator hint, value_type&& value)
    {
        return tree_.insert_hint_equal(hint, std::move(value));
    }

    template <typename P,
              sfl::dtl::enable_if_t<std::is_constructible<value_type, P>::value>* = nullptr>
    iterator insert(const_iterator hint, P&& value)
    {
        return tree_.insert_hint_equal(hint, std::forward<P>(value));
    }

    template <typename InputIt,
              sfl::dtl::enable_if_t<sfl::dtl::is_input_iterator<InputIt>::value>* = nullptr>
    void insert(InputIt first, InputIt last)
    {
        insert_range_aux(first, last);
    }

    void insert(std::initializer_list<value_type> ilist)
    {
        insert_range_aux(ilist.begin(), ilist.end());
    }

#if SFL_CPP_VERSION >= SFL_CPP_20

    template <sfl::dtl::container_compatible_range<value_type> Range>
    void insert_range(Range&& range)
    {
        insert_range_aux(std::ranges::begin(range), std::ranges::end(range));
    }

#else // before C++20

    template <typename Range>
    void insert_range(Range&& range)
    {
        using std::begin;
        using std::end;
        insert_range_aux(begin(range), end(range));
    }

#endif // before C++20

    iterator erase(iterator pos)
    {
        return tree_.erase(pos);
    }

    iterator erase(const_iterator pos)
    {
        return tree_.erase(pos);
    }

    iterator erase(const_iterator first, const_iterator last)
    {
        return tree_.erase(first, last);
    }

    size_type erase(const Key& key)
    {
        return tree_.erase(key);
    }

    template <typename K,
              sfl::dtl::enable_if_t<sfl::dtl::has_is_transparent<Compare, K>::value>* = nullptr>
    size_type erase(K&& x)
    {
        return tree_.erase(x);
    }

    void swap(multimap& other)
    {
        tree_.swap(other.tree_);
    }

    //
    // ---- LOOKUP ------------------------------------------------------------
    //

    SFL_NODISCARD
    iterator lower_bound(const Key& key)
    {
        return tree_.lower_bound(key);
    }

    SFL_NODISCARD
    const_iterator lower_bound(const Key& key) const
    {
        return tree_.lower_bound(key);
    }

    template <typename K,
              sfl::dtl::enable_if_t<sfl::dtl::has_is_transparent<Compare, K>::value>* = nullptr>
    SFL_NODISCARD
    iterator lower_bound(const K& x)
    {
        return tree_.lower_bound(x);
    }

    template <typename K,
              sfl::dtl::enable_if_t<sfl::dtl::has_is_transparent<Compare, K>::value>* = nullptr>
    SFL_NODISCARD
    const_iterator lower_bound(const K& x) const
    {
        return tree_.lower_bound(x);
    }

    SFL_NODISCARD
    iterator upper_bound(const Key& key)
    {
        return tree_.upper_bound(key);
    }

    SFL_NODISCARD
    const_iterator upper_bound(const Key& key) const
    {
        return tree_.upper_bound(key);
    }

    template <typename K,
              sfl::dtl::enable_if_t<sfl::dtl::has_is_transparent<Compare, K>::value>* = nullptr>
    SFL_NODISCARD
    iterator upper_bound(const K& x)
    {
        return tree_.upper_bound(x);
    }

    template <typename K,
              sfl::dtl::enable_if_t<sfl::dtl::has_is_transparent<Compare, K>::value>* = nullptr>
    SFL_NODISCARD
    const_iterator upper_bound(const K& x) const
    {
        return tree_.upper_bound(x);
    }

    SFL_NODISCARD
    std::pair<iterator, iterator> equal_range(const Key& key)
    {
        return tree_.equal_range(key);
    }

    SFL_NODISCARD
    std::pair<const_iterator, const_iterator> equal_range(const Key& key) const
    {
        return tree_.equal_range(key);
    }

    template <typename K,
              sfl::dtl::enable_if_t<sfl::dtl::has_is_transparent<Compare, K>::value>* = nullptr>
    SFL_NODISCARD
    std::pair<iterator, iterator> equal_range(const K& x)
    {
        return tree_.equal_range(x);
    }

    template <typename K,
              sfl::dtl::enable_if_t<sfl::dtl::has_is_transparent<Compare, K>::value>* = nullptr>
    SFL_NODISCARD
    std::pair<const_iterator, const_iterator> equal_range(const K& x) const
    {
        return tree_.equal_range(x);
    }

    SFL_NODISCARD
    iterator find(const Key& key)
    {
        return tree_.find(key);
    }

    SFL_NODISCARD
    const_iterator find(const Key& key) const
    {
        return tree_.find(key);
    }

    template <typename K,
              sfl::dtl::enable_if_t<sfl::dtl::has_is_transparent<Compare, K>::value>* = nullptr>
    SFL_NODISCARD
    iterator find(const K& x)
    {
        return tree_.find(x);
    }

    template <typename K,
              sfl::dtl::enable_if_t<sfl::dtl::has_is_transparent<Compare, K>::value>* = nullptr>
    SFL_NODISCARD
    const_iterator find(const K& x) const
    {
        return tree_.find(x);
    }

    SFL_NODISCARD
    size_type count(const Key& key) const
    {
        return tree_.count(key);
    }

    template <typename K,
              sfl::dtl::enable_if_t<sfl::dtl::has_is_transparent<Compare, K>::value>* = nullptr>
    SFL_NODISCARD
    size_type count(const K& x) const
    {
        return tree_.count(x);
    }

    SFL_NODISCARD
    bool contains(const Key& key) const
    {
        return tree_.contains(key);
    }

    template <typename K,
              sfl::dtl::enable_if_t<sfl::dtl::has_is_transparent<Compare, K>::value>* = nullptr>
    SFL_NODISCARD
    bool contains(const K& x) const
    {
        return tree_.contains(x);
    }

private:

    template <typename InputIt, typename Sentinel>
    void insert_range_aux(InputIt first, Sentinel last)
    {
        while (first != last)
        {
            insert(*first);
            ++first;
        }
    }
};

//
// ---- NON-MEMBER FUNCTIONS --------------------------------------------------
//

template <typename K, typename T, typename C, typename A>
SFL_NODISCARD
bool operator==
(
    const multimap<K, T, C, A>& x,
    const multimap<K, T, C, A>& y
)
{
    return x.size() == y.size() && std::equal(x.begin(), x.end(), y.begin());
}

template <typename K, typename T, typename C, typename A>
SFL_NODISCARD
bool operator!=
(
    const multimap<K, T, C, A>& x,
    const multimap<K, T, C, A>& y
)
{
    return !(x == y);
}

template <typename K, typename T, typename C, typename A>
SFL_NODISCARD
bool operator<
(
    const multimap<K, T, C, A>& x,
    const multimap<K, T, C, A>& y
)
{
    return std::lexicographical_compare(x.begin(), x.end(), y.begin(), y.end());
}

template <typename K, typename T, typename C, typename A>
SFL_NODISCARD
bool operator>
(
    const multimap<K, T, C, A>& x,
    const multimap<K, T, C, A>& y
)
{
    return y < x;
}

template <typename K, typename T, typename C, typename A>
SFL_NODISCARD
bool operator<=
(
    const multimap<K, T, C, A>& x,
    const multimap<K, T, C, A>& y
)
{
    return !(y < x);
}

template <typename K, typename T, typename C, typename A>
SFL_NODISCARD
bool operator>=
(
    const multimap<K, T, C, A>& x,
    const multimap<K, T, C, A>& y
)
{
    return !(x < y);
}

template <typename K, typename T, typename C, typename A>
void swap
(
    multimap<K, T, C, A>& x,
    multimap<K, T, C, A>& y
)
{
    x.swap(y);
}

template <typename K, typename T, typename C, typename A, typename Predicate>
typename multimap<K, T, C, A>::size_type
    erase_if(multimap<K, T, C, A>& c, Predicate pred)
{
    auto old_size = c.size();

    for (auto it = c.begin(); it != c.end(); )
    {
        if (pred(*it))
        {
            it = c.erase(it);
        }
        else
        {
            ++it;
        }
    }

    return old_size - c.size();
}

} // namespace sfl

#endif // SFL_MULTIMAP_HPP_INCLUDED
