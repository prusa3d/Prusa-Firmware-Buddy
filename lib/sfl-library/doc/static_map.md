# sfl::static_map

<details>

<summary>Table of Contents</summary>

* [Summary](#summary)
* [Template Parameters](#template-parameters)
* [Public Member Types](#public-member-types)
* [Public Member Classes](#public-member-classes)
  * [value\_compare](#value_compare)
* [Public Data Members](#public-data-members)
  * [static\_capacity](#static_capacity)
* [Public Member Functions](#public-member-functions)
  * [(constructor)](#constructor)
  * [(destructor)](#destructor)
  * [operator=](#operator)
  * [key\_comp](#key_comp)
  * [value\_comp](#value_comp)
  * [begin, cbegin](#begin-cbegin)
  * [end, cend](#end-cend)
  * [rbegin, crbegin](#rbegin-crbegin)
  * [rend, crend](#rend-crend)
  * [empty](#empty)
  * [full](#full)
  * [size](#size)
  * [max\_size](#max_size)
  * [capacity](#capacity)
  * [available](#available)
  * [clear](#clear)
  * [emplace](#emplace)
  * [emplace\_hint](#emplace_hint)
  * [insert](#insert)
  * [insert\_range](#insert_range)
  * [insert\_or\_assign](#insert_or_assign)
  * [try\_emplace](#try_emplace)
  * [erase](#erase)
  * [swap](#swap)
  * [lower\_bound](#lower_bound)
  * [upper\_bound](#upper_bound)
  * [equal\_range](#equal_range)
  * [find](#find)
  * [count](#count)
  * [contains](#contains)
  * [at](#at)
  * [operator\[\]](#operator-1)
* [Non-member Functions](#non-member-functions)
  * [operator==](#operator-2)
  * [operator!=](#operator-3)
  * [operator\<](#operator-4)
  * [operator\>](#operator-5)
  * [operator\<=](#operator-6)
  * [operator\>=](#operator-7)
  * [swap](#swap-1)
  * [erase\_if](#erase_if)

</details>



## Summary

Defined in header `sfl/static_map.hpp`:

```
namespace sfl
{
    template < typename Key,
               typename T,
               std::size_t N,
               typename Compare = std::less<Key> >
    class static_map;
}
```

`sfl::static_map` is an associative container similar to [`std::map`](https://en.cppreference.com/w/cpp/container/map), but with the different storage model.

This container internally holds statically allocated array of size `N` and stores elements into this array, which avoids dynamic memory allocation and deallocation. This container **never** uses dynamic memory management. The number of elements in this container **cannot** be greater than `N`. Attempting to insert more than `N` elements into this container results in **undefined behavior**.

Underlying storage is implemented as **red-black tree**.

Complexity of search, insert and remove operations is O(log N).

Iterators to elements are bidirectional iterators and they meet the requirements of [*LegacyBidirectionalIterator*](https://en.cppreference.com/w/cpp/named_req/BidirectionalIterator).

`sfl::static_map` meets the requirements of [*Container*](https://en.cppreference.com/w/cpp/named_req/Container), [*ReversibleContainer*](https://en.cppreference.com/w/cpp/named_req/ReversibleContainer) and [*AssociativeContainer*](https://en.cppreference.com/w/cpp/named_req/AssociativeContainer).

This container is convenient for bare-metal embedded software development.

<br><br>



## Template Parameters

1.  ```
    typename Key
    ```

    Key type.

2.  ```
    typename T
    ```

    Value type.

3.  ```
    std::size_t N
    ```

    Size of the internal statically allocated array, i.e. the maximal number of elements that this container can contain.

4.  ```
    typename Compare
    ```

    Ordering function for keys.

<br><br>



## Public Member Types

| Member Type               | Definition |
| :------------------------ | :--------- |
| `key_type`                | `Key` |
| `mapped_type`             | `T` |
| `value_type`              | `std::pair<const Key, T>` |
| `size_type`               | Unsigned integer type |
| `difference_type`         | Signed integer type |
| `key_compare`             | `Compare` |
| `reference`               | `value_type&` |
| `const_reference`         | `const value_type&` |
| `pointer`                 | Pointer to `value_type` |
| `const_pointer`           | Pointer to `const value_type` |
| `iterator`                | [*LegacyBidirectionalIterator*](https://en.cppreference.com/w/cpp/named_req/BidirectionalIterator) to `value_type` |
| `const_iterator`          | [*LegacyBidirectionalIterator*](https://en.cppreference.com/w/cpp/named_req/BidirectionalIterator) to `const value_type` |
| `reverse_iterator`        | Reverse [*LegacyBidirectionalIterator*](https://en.cppreference.com/w/cpp/named_req/BidirectionalIterator) to `value_type` |
| `const_reverse_iterator`  | Reverse [*LegacyBidirectionalIterator*](https://en.cppreference.com/w/cpp/named_req/BidirectionalIterator) to `const value_type` |

<br><br>



## Public Member Classes

### value_compare

```
class value_compare
{
public:
    bool operator()(const value_type& x, const value_type& y) const;
};
```

<br><br>



## Public Data Members

### static_capacity

```
static constexpr size_type static_capacity = N;
```

<br><br>



## Public Member Functions

### (constructor)

1.  ```
    static_map() noexcept(std::is_nothrow_default_constructible<Compare>::value);
    ```
2.  ```
    explicit static_map(const Compare& comp) noexcept(std::is_nothrow_copy_constructible<Compare>::value);
    ```

    **Effects:**
    Constructs an empty container.

    **Complexity:**
    Constant.

    <br><br>



3.  ```
    template <typename InputIt>
    static_map(InputIt first, InputIt last);
    ```
4.  ```
    template <typename InputIt>
    static_map(InputIt first, InputIt last, const Compare& comp);
    ```

    **Preconditions:**
    `std::distance(first, last) <= capacity()`

    **Effects:**
    Constructs the container with the contents of the range `[first, last)`.

    If multiple elements in the range have keys that compare equivalent, then the first element is inserted.

    **Note:**
    These overloads participate in overload resolution only if `InputIt` satisfies requirements of [*LegacyInputIterator*](https://en.cppreference.com/w/cpp/named_req/InputIterator).

    <br><br>



5.  ```
    static_map(std::initializer_list<value_type> ilist);
    ```
6.  ```
    static_map(std::initializer_list<value_type> ilist, const Compare& comp);
    ```

    **Preconditions:**
    `ilist.size() <= capacity()`

    **Effects:**
    Constructs the container with the contents of the initializer list `ilist`.

    If multiple elements in the range have keys that compare equivalent, then the first element is inserted.

    <br><br>



7.  ```
    static_map(const static_map& other);
    ```

    **Effects:**
    Copy constructor.
    Constructs the container with the copy of the contents of `other`.

    <br><br>



8.  ```
    static_map(static_map&& other);
    ```

    **Effects:**
    Move constructor.
    Constructs the container with the contents of `other` using move semantics.

    `other` is not guaranteed to be empty after the move.

    `other` is in a valid but unspecified state after the move.

    <br><br>



9.  ```
    template <typename Range>
    static_map(sfl::from_range_t, Range&& range);
    ```
10. ```
    template <typename Range>
    static_map(sfl::from_range_t, Range&& range, const Compare& comp);
    ```

    **Effects:**
    Constructs the container with the contents of `range`.

    If multiple elements in the range have keys that compare equivalent, then the first element is inserted.

    **Note:**
    It is available in C++11. In C++20 are used proper C++20 range concepts.

    <br><br>



### (destructor)

1.  ```
    ~static_map();
    ```

    **Effects:**
    Destructs the container. The destructors of the elements are called and the used storage is deallocated.

    **Complexity:**
    Linear in size.

    <br><br>



### operator=

1.  ```
    static_map& operator=(const static_map& other);
    ```

    **Effects:**
    Copy assignment operator.
    Replaces the contents with a copy of the contents of `other`.

    **Returns:**
    `*this()`.

    <br><br>



2.  ```
    static_map& operator=(static_map&& other);
    ```

    **Effects:**
    Move assignment operator.
    Replaces the contents with those of `other` using move semantics.

    `other` is not guaranteed to be empty after the move.

    `other` is in a valid but unspecified state after the move.

    **Returns:**
    `*this()`.

    <br><br>



3.  ```
    static_map& operator=(std::initializer_list<value_type> ilist);
    ```

    **Preconditions:**
    `ilist.size() <= capacity()`

    **Effects:**
    Replaces the contents with those identified by initializer list `ilist`.

    **Returns:**
    `*this()`.

    <br><br>



### key_comp

1.  ```
    key_compare key_comp() const;
    ```

    **Effects:**
    Returns the function object that compares the keys, which is a copy of this container's constructor argument `comp`.

    **Complexity:**
    Constant.

    <br><br>



### value_comp

1.  ```
    value_compare value_comp() const;
    ```

    **Effects:**
    Returns a function object that compares objects of type `value_type`.

    **Complexity:**
    Constant.

    <br><br>



### begin, cbegin

1.  ```
    iterator begin() noexcept;
    ```
2.  ```
    const_iterator begin() const noexcept;
    ```
3.  ```
    const_iterator cbegin() const noexcept;
    ```

    **Effects:**
    Returns an iterator to the first element of the container.
    If the container is empty, the returned iterator will be equal to `end()`.

    **Complexity:**
    Constant.

    <br><br>



### end, cend

1.  ```
    iterator end() noexcept;
    ```
2.  ```
    const_iterator end() const noexcept;
    ```
3.  ```
    const_iterator cend() const noexcept;
    ```

    **Effects:**
    Returns an iterator to the element following the last element of the container.
    This element acts as a placeholder; attempting to access it results in undefined behavior.

    **Complexity:**
    Constant.

    <br><br>



### rbegin, crbegin

1.  ```
    reverse_iterator rbegin() noexcept;
    ```
2.  ```
    const_reverse_iterator rbegin() const noexcept;
    ```
3.  ```
    const_reverse_iterator crbegin() const noexcept;
    ```

    **Effects:**
    Returns a reverse iterator to the first element of the reversed container.
    It corresponds to the last element of the non-reversed container.
    If the container is empty, the returned iterator is equal to `rend()`.

    **Complexity:**
    Constant.

    <br><br>



### rend, crend

1.  ```
    reverse_iterator rend() noexcept;
    ```
2.  ```
    const_reverse_iterator rend() const noexcept;
    ```
3.  ```
    const_reverse_iterator crend() const noexcept;
    ```

    **Effects:**
    Returns a reverse iterator to the element following the last element of the reversed container.
    It corresponds to the element preceding the first element of the non-reversed container.
    This element acts as a placeholder, attempting to access it results in undefined behavior.

    **Complexity:**
    Constant.

    <br><br>



### empty

1.  ```
    bool empty() const noexcept;
    ```

    **Effects:**
    Returns `true` if the container has no elements, i.e. whether `begin() == end()`.

    **Complexity:**
    Constant.

    <br><br>



### full

1.  ```
    bool full() const noexcept;
    ```

    **Effects:**
    Returns `true` if the container is full, i.e. whether `size() == capacity()`.

    **Complexity:**
    Constant.

    <br><br>



### size

1.  ```
    size_type size() const noexcept;
    ```

    **Effects:**
    Returns the number of elements in the container, i.e. `std::distance(begin(), end())`.

    **Complexity:**
    Constant.

    <br><br>



### max_size

1.  ```
    static constexpr size_type max_size() const noexcept;
    ```

    **Effects:**
    Returns the maximum number of elements the container is able to hold, i.e. `N`.

    **Complexity:**
    Constant.

    <br><br>



### capacity

1.  ```
    static constexpr size_type capacity() const noexcept;
    ```

    **Effects:**
    Returns the maximum number of elements the container is able to hold, i.e. `N`.

    **Complexity:**
    Constant.

    <br><br>



### available

1.  ```
    size_type available() const noexcept;
    ```

    **Effects:**
    Returns the number of elements that can be inserted into the container, i.e. `capacity() - size()`.

    **Complexity:**
    Constant.

    <br><br>



### clear

1.  ```
    void clear() noexcept;
    ```

    **Effects:**
    Erases all elements from the container.
    After this call, `size()` returns zero and `capacity()` remains unchanged.

    **Complexity:**
    Linear in `size()`.

    <br><br>



### emplace

1.  ```
    template <typename... Args>
    std::pair<iterator, bool> emplace(Args&&... args);
    ```

    **Preconditions:**
    `!full()`

    **Effects:**
    Inserts new element into the container if the container doesn't already contain an element with an equivalent key.

    New element is constructed as `value_type(std::forward<Args>(args)...)`.

    The element may be constructed even if there already is an element with the key in the container, in which case the newly constructed element will be destroyed immediately.

    **Returns:**
    The iterator component points to the inserted element or to the already existing element. The `bool` component is `true` if insertion happened and `false` if it did not.

    <br><br>



### emplace_hint

1.  ```
    template <typename... Args>
    iterator emplace_hint(const_iterator hint, Args&&... args);
    ```

    **Preconditions:**
    `!full()`

    **Effects:**
    Inserts new element into the container if the container doesn't already contain an element with an equivalent key.

    New element is constructed as `value_type(std::forward<Args>(args)...)`.

    The element may be constructed even if there already is an element with the key in the container, in which case the newly constructed element will be destroyed immediately.

    Iterator `hint` is used as a suggestion where to start to search insert position.

    **Returns:**
    Iterator to the inserted element or to the already existing element.

    <br><br>



### insert

1.  ```
    std::pair<iterator, bool> insert(const value_type& value);
    ```

    **Preconditions:**
    `!full()`

    **Effects:**
    Inserts copy of `value` if the container doesn't already contain an element with an equivalent key.

    **Returns:**
    The iterator component points to the inserted element or to the already existing element. The `bool` component is `true` if insertion happened and `false` if it did not.

    <br><br>



2.  ```
    std::pair<iterator, bool> insert(value_type&& value);
    ```

    **Preconditions:**
    `!full()`

    **Effects:**
    Inserts `value` using move semantics if the container doesn't already contain an element with an equivalent key.

    **Returns:**
    The iterator component points to the inserted element or to the already existing element. The `bool` component is `true` if insertion happened and `false` if it did not.

    <br><br>



3.  ```
    template <typename P>
    std::pair<iterator, bool> insert(P&& value);
    ```

    **Preconditions:**
    `!full()`

    **Effects:**
    Inserts new element into the container if the container doesn't already contain an element with an equivalent key.

    New element is constructed as `value_type(std::forward<P>(value))`.

    **Note:**
    This overload participates in overload resolution only if `std::is_constructible<value_type, P&&>::value` is `true`.

    **Returns:**
    The iterator component points to the inserted element or to the already existing element. The `bool` component is `true` if insertion happened and `false` if it did not.

    <br><br>



4.  ```
    iterator insert(const_iterator hint, const value_type& value);
    ```

    **Preconditions:**
    `!full()`

    **Effects:**
    Inserts copy of `value` if the container doesn't already contain an element with an equivalent key.

    Iterator `hint` is used as a suggestion where to start to search insert position.

    **Returns:**
    Iterator to the inserted element or to the already existing element.

    <br><br>



5.  ```
    iterator insert(const_iterator hint, value_type&& value);
    ```

    **Preconditions:**
    `!full()`

    **Effects:**
    Inserts `value` using move semantics if the container doesn't already contain an element with an equivalent key.

    Iterator `hint` is used as a suggestion where to start to search insert position.

    **Returns:**
    Iterator to the inserted element or to the already existing element.

    <br><br>



6.  ```
    template <typename P>
    iterator insert(const_iterator hint, P&& value);
    ```

    **Preconditions:**
    `!full()`

    **Effects:**
    Inserts new element into the container if the container doesn't already contain an element with an equivalent key.

    New element is constructed as `value_type(std::forward<P>(value))`.

    Iterator `hint` is used as a suggestion where to start to search insert position.

    **Note:**
    This overload participates in overload resolution only if `std::is_constructible<value_type, P&&>::value` is `true`.

    **Returns:**
    Iterator to the inserted element or to the already existing element.

    <br><br>



7.  ```
    template <typename InputIt>
    void insert(InputIt first, InputIt last);
    ```

    **Preconditions:**
    `std::distance(first, last) <= available()`

    **Effects:**
    Inserts elements from range `[first, last)` if the container doesn't already contain an element with an equivalent key.

    If multiple elements in the range have keys that compare equivalent, then the first element is inserted.

    The call to this function is equivalent to:
    ```
    while (first != last)
    {
        insert(*first);
        ++first;
    }
    ```

    **Note:**
    This overload participates in overload resolution only if `InputIt` satisfies requirements of [*LegacyInputIterator*](https://en.cppreference.com/w/cpp/named_req/InputIterator).

    <br><br>



8.  ```
    void insert(std::initializer_list<value_type> ilist);
    ```

    **Preconditions:**
    `ilist.size() <= available()`

    **Effects:**
    Inserts elements from initializer list `ilist` if the container doesn't already contain an element with an equivalent key.

    If multiple elements in the range have keys that compare equivalent, then the first element is inserted.

    The call to this function is equivalent to `insert(ilist.begin(), ilist.end())`.

    <br><br>



### insert_range

1.  ```
    template <typename Range>
    void insert_range(Range&& range);
    ```

    **Effects:**
    Inserts elements from `range` if the container doesn't already contain an element with an equivalent key.

    If multiple elements in the range have keys that compare equivalent, then the first element is inserted.

    **Note:**
    It is available in C++11. In C++20 are used proper C++20 range concepts.

    <br><br>



### insert_or_assign

1.  ```
    template <typename M>
    std::pair<iterator, bool> insert_or_assign(const Key& key, M&& obj);
    ```
2.  ```
    template <typename M>
    std::pair<iterator, bool> insert_or_assign(Key&& key, M&& obj);
    ```
3.  ```
    template <typename K, typename M>
    std::pair<iterator, bool> insert_or_assign(K&& key, M&& obj);
    ```

    **Effects:**
    If a key equivalent to `key` already exists in the container, assigns `std::forward<M>(obj)` to the mapped type corresponding to the key `key`. If the key does not exist, inserts the new element.

    *   **Overload (1):** New element is constructed as

        ```
        value_type( std::piecewise_construct,
                    std::forward_as_tuple(key),
                    std::forward_as_tuple(std::forward<M>(obj)) )
        ```

        **Note:** This overload participates in overload resolution only if `std::is_assignable_v<mapped_type&, M&&>` is `true`.

    *   **Overload (2):** New element is constructed as

        ```
        value_type( std::piecewise_construct,
                    std::forward_as_tuple(std::move(key)),
                    std::forward_as_tuple(std::forward<M>(obj)) )
        ```

        **Note:** This overload participates in overload resolution only if `std::is_assignable_v<mapped_type&, M&&>` is `true`.

    *   **Overload (3):** New element is constructed as

        ```
        value_type( std::piecewise_construct,
                    std::forward_as_tuple(std::forward<K>(key)),
                    std::forward_as_tuple(std::forward<M>(obj)) )
        ```

        **Note:** This overload participates in overload resolution only if all following conditions are satisfied:
        1.  `Compare::is_transparent` exists and is a valid type. It allows calling this function without constructing an instance of `Key`.
        2.  `std::is_assignable_v<mapped_type&, M&&>` is `true`.

    **Returns:**
    The iterator component points to the inserted element or to the updated element. The `bool` component is `true` if insertion took place and `false` if assignment took place.

    <br><br>



4.  ```
    template <typename M>
    iterator insert_or_assign(const_iterator hint, const Key& key, M&& obj);
    ```
5.  ```
    template <typename M>
    iterator insert_or_assign(const_iterator hint, Key&& key, M&& obj);
    ```
6.  ```
    template <typename K, typename M>
    iterator insert_or_assign(const_iterator hint, K&& key, M&& obj);
    ```

    **Effects:**
    If a key equivalent to `key` already exists in the container, assigns `std::forward<M>(obj)` to the mapped type corresponding to the key `key`. If the key does not exist, inserts the new element.

    Iterator `hint` is used as a suggestion where to start to search insert position.

    *   **Overload (4):** New element is constructed as

        ```
        value_type( std::piecewise_construct,
                    std::forward_as_tuple(key),
                    std::forward_as_tuple(std::forward<M>(obj)) )
        ```

        **Note:** This overload participates in overload resolution only if `std::is_assignable_v<mapped_type&, M&&>` is `true`.

    *   **Overload (5):** New element is constructed as

        ```
        value_type( std::piecewise_construct,
                    std::forward_as_tuple(std::move(key)),
                    std::forward_as_tuple(std::forward<M>(obj)) )
        ```

        **Note:** This overload participates in overload resolution only if `std::is_assignable_v<mapped_type&, M&&>` is `true`.

    *   **Overload (6):** New element is constructed as

        ```
        value_type( std::piecewise_construct,
                    std::forward_as_tuple(std::forward<K>(key)),
                    std::forward_as_tuple(std::forward<M>(obj)) )
        ```

        **Note:** This overload participates in overload resolution only if all following conditions are satisfied:
        1.  `Compare::is_transparent` exists and is a valid type. It allows calling this function without constructing an instance of `Key`.
        2.  `std::is_assignable_v<mapped_type&, M&&>` is `true`.

    **Returns:**
    Iterator to the element that was inserted or updated.

    <br><br>



### try_emplace

1.  ```
    template <typename... Args>
    std::pair<iterator, bool> try_emplace(const Key& key, Args&&... args);
    ```
2.  ```
    template <typename... Args>
    std::pair<iterator, bool> try_emplace(Key&& key, Args&&... args);
    ```
3.  ```
    template <typename K, typename... Args>
    std::pair<iterator, bool> try_emplace(K&& key, Args&&... args);
    ```

    **Preconditions:**
    `!full()`

    **Effects:**
    If a key equivalent to `key` already exists in the container, does nothing.
    Otherwise, inserts a new element into the container.

    *   **Overload (1):** Behaves like `emplace` except that the element is constructed as

        ```
        value_type( std::piecewise_construct,
                    std::forward_as_tuple(key),
                    std::forward_as_tuple(std::forward<Args>(args)...) )
        ```

    *   **Overload (2):** Behaves like `emplace` except that the element is constructed as

        ```
        value_type( std::piecewise_construct,
                    std::forward_as_tuple(std::move(key)),
                    std::forward_as_tuple(std::forward<Args>(args)...) )
        ```

    *   **Overload (3):** Behaves like `emplace` except that the element is constructed as

        ```
        value_type( std::piecewise_construct,
                    std::forward_as_tuple(std::forward<K>(key)),
                    std::forward_as_tuple(std::forward<Args>(args)...) )
        ```

        **Note:** This overload participates in overload resolution only if all following conditions are satisfied:
        1.  `Compare::is_transparent` exists and is a valid type. It allows calling this function without constructing an instance of `Key`.
        2.  `std::is_convertible_v<K&&, iterator>` is `false`.
        3.  `std::is_convertible_v<K&&, const_iterator>` is `false`.

    **Returns:**
    The iterator component points to the inserted element or to the already existing element. The `bool` component is `true` if insertion happened and `false` if it did not.

    <br><br>



4.  ```
    template <typename... Args>
    iterator try_emplace(const_iterator hint, const Key& key, Args&&... args);
    ```
5.  ```
    template <typename... Args>
    iterator try_emplace(const_iterator hint, Key&& key, Args&&... args);
    ```
6.  ```
    template <typename K, typename... Args>
    iterator try_emplace(const_iterator hint, K&& key, Args&&... args);
    ```

    **Preconditions:**
    `!full()`

    **Effects:**
    If a key equivalent to `key` already exists in the container, does nothing.
    Otherwise, inserts a new element into the container.

    Iterator `hint` is used as a suggestion where to start to search insert position.

    *   **Overload (4):** Behaves like `emplace_hint` except that the element is constructed as

        ```
        value_type( std::piecewise_construct,
                    std::forward_as_tuple(key),
                    std::forward_as_tuple(std::forward<Args>(args)...) )
        ```

    *   **Overload (5):** Behaves like `emplace_hint` except that the element is constructed as

        ```
        value_type( std::piecewise_construct,
                    std::forward_as_tuple(std::move(key)),
                    std::forward_as_tuple(std::forward<Args>(args)...) )
        ```

    *   **Overload (6):** Behaves like `emplace_hint` except that the element is constructed as

        ```
        value_type( std::piecewise_construct,
                    std::forward_as_tuple(std::forward<K>(key)),
                    std::forward_as_tuple(std::forward<Args>(args)...) )
        ```

        **Note:** This overload participates in overload resolution only if `Compare::is_transparent` exists and is a valid type. It allows calling this function without constructing an instance of `Key`.

    **Returns:**
    Iterator to the inserted element or to the already existing element.

    <br><br>



### erase

1.  ```
    iterator erase(iterator pos);
    ```
2.  ```
    iterator erase(const_iterator pos);
    ```

    **Effects:**
    Removes the element at `pos`.

    **Returns:**
    Iterator following the last removed element.

    <br><br>



3.  ```
    iterator erase(const_iterator first, const_iterator last);
    ```

    **Effects:**
    Removes the elements in the range `[first, last)`.

    **Returns:**
    Iterator following the last removed element.

    <br><br>



4.  ```
    size_type erase(const Key& key);
    ```
5.  ```
    template <typename K>
    size_type erase(K&& x);
    ```

    **Effects:**
    Removes the element (if one exists) with the key equivalent to `key` or `x`.

    **Note:**
    Overload (5) participates in overload resolution only if `Compare::is_transparent` exists and is a valid type. It allows calling this function without constructing an instance of `Key`.

    **Returns:**
    Number of elements removed (0 or 1).

    <br><br>



### swap

1.  ```
    void swap(static_map& other);
    ```

    **Effects:**
    Exchanges the contents of the container with those of `other`.

    <br><br>



### lower_bound

1.  ```
    iterator lower_bound(const Key& key);
    ```
2.  ```
    const_iterator lower_bound(const Key& key) const;
    ```
3.  ```
    template <typename K>
    iterator lower_bound(const K& x);
    ```
4.  ```
    template <typename K>
    const_iterator lower_bound(const K& x) const;
    ```

    **Effects:**
    Returns an iterator pointing to the first element with key that compares **not less than** `key` or `x`. Returns `end()` if no such element is found.

    **Note:**
    Overloads (3) and (4) participate in overload resolution only if `Compare::is_transparent` exists and is a valid type. It allows calling these functions without constructing an instance of `Key`.

    **Complexity:**
    Logarithmic in `size()`.

    <br><br>



### upper_bound

1.  ```
    iterator upper_bound(const Key& key);
    ```
2.  ```
    const_iterator upper_bound(const Key& key) const;
    ```
3.  ```
    template <typename K>
    iterator upper_bound(const K& x);
    ```
4.  ```
    template <typename K>
    const_iterator upper_bound(const K& x) const;
    ```

    **Effects:**
    Returns an iterator pointing to the first element with key that compares **greater than** `key` or `x`. Returns `end()` if no such element is found.

    **Note:**
    Overloads (3) and (4) participate in overload resolution only if `Compare::is_transparent` exists and is a valid type. It allows calling these functions without constructing an instance of `Key`.

    **Complexity:**
    Logarithmic in `size()`.

    <br><br>



### equal_range

1.  ```
    std::pair<iterator, iterator> equal_range(const Key& key);
    ```
2.  ```
    std::pair<const_iterator, const_iterator> equal_range(const Key& key) const;
    ```
3.  ```
    template <typename K>
    std::pair<iterator, iterator> equal_range(const K& x);
    ```
4.  ```
    template <typename K>
    std::pair<const_iterator, const_iterator> equal_range(const K& x) const;
    ```

    **Effects:**
    Returns a range containing all elements with key that compares equivalent to `key` or `x`.
    *   The first iterator in pair points to the first element that compares **not less than** `key` or `x`. It is equal to `end()` if no such element is found.
    *   The second iterator in pair points to the first element that compares **greater than** `key` or `x`. It is equal to `end()` is no such element is found.

    **Note:**
    Overloads (3) and (4) participate in overload resolution only if `Compare::is_transparent` exists and is a valid type. It allows calling these functions without constructing an instance of `Key`.

    **Complexity:**
    Logarithmic in `size()`.

    <br><br>



### find

1.  ```
    iterator find(const Key& key);
    ```
2.  ```
    const_iterator find(const Key& key) const;
    ```
3.  ```
    template <typename K>
    iterator find(const K& x);
    ```
4.  ```
    template <typename K>
    const_iterator find(const K& x) const;
    ```

    **Effects:**
    Returns an iterator pointing to the element with key equivalent to `key` or `x`. Returns `end()` if no such element is found.

    **Note:**
    Overloads (3) and (4) participate in overload resolution only if `Compare::is_transparent` exists and is a valid type. It allows calling these functions without constructing an instance of `Key`.

    **Complexity:**
    Logarithmic in `size()`.

    <br><br>



### count

1.  ```
    size_type count(const Key& key) const;
    ```
2.  ```
    template <typename K>
    size_type count(const K& x) const;
    ```

    **Effects:**
    Returns the number of elements with key equivalent to `key` or `x`, which is either 1 or 0 since this container does not allow duplicates.

    **Note:**
    Overload (2) participates in overload resolution only if `Compare::is_transparent` exists and is a valid type. It allows calling this function without constructing an instance of `Key`.

    **Complexity:**
    Logarithmic in `size()`.

    <br><br>



### contains

1.  ```
    bool contains(const Key& key) const;
    ```
2.  ```
    template <typename K>
    bool contains(const K& x) const;
    ```

    **Effects:**
    Returns `true` if the container contains an element with key equivalent to `key` or `x`, otherwise returns `false`.

    **Note:**
    Overload (2) participates in overload resolution only if `Compare::is_transparent` exists and is a valid type. It allows calling this function without constructing an instance of `Key`.

    **Complexity:**
    Logarithmic in `size()`.

    <br><br>



### at

1.  ```
    T& at(const Key& key);
    ```
2.  ```
    const T& at(const Key& key) const;
    ```
3.  ```
    template <typename K>
    const T& at(const K& x) const;
    ```

    **Effects:**
    Returns a reference to the mapped value of the element with key equivalent to `key` or `x`. If no such element exists, an exception of type `std::out_of_range` is thrown.

    **Note:**
    Overload (3) participates in overload resolution only if `Compare::is_transparent` exists and is a valid type. It allows calling this function without constructing an instance of `Key`.

    **Complexity:**
    Logarithmic in `size()`.

    **Exceptions:**
    `std::out_of_range` if the container does not have an element with the specified key.

    <br><br>



### operator[]

1.  ```
    T& operator[](const Key& key);
    ```
2.  ```
    T& operator[](Key&& key);
    ```
3.  ```
    template <typename K>
    T& operator[](const K& x);
    ```
4.  ```
    template <typename K>
    T& operator[](K&& x);
    ```

    **Preconditions:**
    `!full()`

    **Effects:**
    Returns a reference to the value that is mapped to a key equivalent to `key` or `x`, performing an insertion if such key does not already exist.

    *   Overload (1) is equivalent to
      `return try_emplace(key).first->second;`

    *   Overload (2) is equivalent to
      `return try_emplace(std::move(key)).first->second;`

    *   Overload (3) is equivalent to
      `return try_emplace(x).first->second;`

    *   Overload (4) is equivalent to
      `return try_emplace(std::forward<K>(x)).first->second;`

    **Note:**
    Overloads (3) and (4) participate in overload resolution only if `Compare::is_transparent` exists and is a valid type. It allows calling these functions without constructing an instance of `Key`.

    **Complexity:**
    Logarithmic in `size()`.

    <br><br>



## Non-member Functions

### operator==

1.  ```
    template <typename K, typename T, std::size_t N, typename C>
    bool operator==
    (
        const static_map<K, T, N, C>& x,
        const static_map<K, T, N, C>& y
    );
    ```

    **Effects:**
    Checks if the contents of `x` and `y` are equal.

    The contents of `x` and `y` are equal if the following conditions hold:
    * `x.size() == y.size()`
    * Each element in `x` compares equal with the element in `y` at the same position.

    The comparison is performed by `std::equal`.
    This comparison ignores the container's ordering `Compare`.

    **Returns:**
    Returns `true` if the contents of the `x` and `y` are equal, `false` otherwise.

    <br><br>



### operator!=

1.  ```
    template <typename K, typename T, std::size_t N, typename C>
    bool operator!=
    (
        const static_map<K, T, N, C>& x,
        const static_map<K, T, N, C>& y
    );
    ```

    **Effects:**
    Checks if the contents of `x` and `y` are equal.

    For details see `operator==`.

    **Returns:**
    Returns `true` if the contents of the `x` and `y` are not equal, `false` otherwise.

    <br><br>



### operator<

1.  ```
    template <typename K, typename T, std::size_t N, typename C>
    bool operator<
    (
        const static_map<K, T, N, C>& x,
        const static_map<K, T, N, C>& y
    );
    ```

    **Effects:**
    Compares the contents of `x` and `y` lexicographically.
    The comparison is performed by a function `std::lexicographical_compare`.
    This comparison ignores the container's ordering `Compare`.

    **Returns:**
    `true` if the contents of the `x` are lexicographically less than the contents of `y`, `false` otherwise.

    <br><br>



### operator>

1.  ```
    template <typename K, typename T, std::size_t N, typename C>
    bool operator>
    (
        const static_map<K, T, N, C>& x,
        const static_map<K, T, N, C>& y
    );
    ```

    **Effects:**
    Compares the contents of lhs and rhs lexicographically.

    The comparison is performed by a function `std::lexicographical_compare`.
    This comparison ignores the container's ordering `Compare`.

    **Returns:**
    `true` if the contents of the `x` are lexicographically greater than the contents of `y`, `false` otherwise.

    <br><br>



### operator<=

1.  ```
    template <typename K, typename T, std::size_t N, typename C>
    bool operator<=
    (
        const static_map<K, T, N, C>& x,
        const static_map<K, T, N, C>& y
    );
    ```

    **Effects:**
    Compares the contents of `x` and `y` lexicographically.
    The comparison is performed by a function `std::lexicographical_compare`.
    This comparison ignores the container's ordering `Compare`.

    **Returns:**
    `true` if the contents of the `x` are lexicographically less than or equal to the contents of `y`, `false` otherwise.

    <br><br>



### operator>=

1.  ```
    template <typename K, typename T, std::size_t N, typename C>
    bool operator>=
    (
        const static_map<K, T, N, C>& x,
        const static_map<K, T, N, C>& y
    );
    ```

    **Effects:**
    Compares the contents of `x` and `y` lexicographically.
    The comparison is performed by a function `std::lexicographical_compare`.
    This comparison ignores the container's ordering `Compare`.

    **Returns:**
    `true` if the contents of the `x` are lexicographically greater than or equal to the contents of `y`, `false` otherwise.

    <br><br>



### swap

1.  ```
    template <typename K, typename T, std::size_t N, typename C>
    void swap
    (
        static_map<K, T, N, C>& x,
        static_map<K, T, N, C>& y
    );
    ```

    **Effects:**
    Swaps the contents of `x` and `y`. Calls `x.swap(y)`.

    <br><br>



### erase_if

1.  ```
    template <typename K, typename T, std::size_t N, typename C, typename Predicate>
    typename static_map<K, T, N, C>::size_type
        erase_if(static_map<K, T, N, C>& c, Predicate pred)
    ```

    **Effects:**
    Erases all elements that satisfy the predicate `pred` from the container.

    `pred` is unary predicate which returns `true` if the element should be removed.

    **Returns:**
    The number of erased elements.

    <br><br>



End of document.
