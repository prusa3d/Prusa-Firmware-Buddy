# sfl::static_unordered_flat_map

<details>

<summary>Table of Contents</summary>

* [Summary](#summary)
* [Template Parameters](#template-parameters)
* [Public Member Types](#public-member-types)
* [Public Member Classes](#public-member-classes)
  * [value\_equal](#value_equal)
* [Public Data Members](#public-data-members)
  * [static\_capacity](#static_capacity)
* [Public Member Functions](#public-member-functions)
  * [(constructor)](#constructor)
  * [(destructor)](#destructor)
  * [operator=](#operator)
  * [key\_eq](#key_eq)
  * [value\_eq](#value_eq)
  * [begin, cbegin](#begin-cbegin)
  * [end, cend](#end-cend)
  * [nth](#nth)
  * [index\_of](#index_of)
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
  * [find](#find)
  * [count](#count)
  * [contains](#contains)
  * [at](#at)
  * [operator\[\]](#operator-1)
  * [data](#data)
* [Non-member Functions](#non-member-functions)
  * [operator==](#operator-2)
  * [operator!=](#operator-3)
  * [swap](#swap-1)
  * [erase\_if](#erase_if)

</details>



## Summary

Defined in header `sfl/static_unordered_flat_map.hpp`

```
namespace sfl
{
    template < typename Key,
               typename T,
               std::size_t N,
               typename KeyEqual = std::equal_to<Key> >
    class static_unordered_flat_map;
}
```

`sfl::static_unordered_flat_map` is an associative container that contains **unsorted** set of **key-value** pairs with **unique** keys.

Underlying storage is implemented as **unsorted vector**.

Complexity of search, insert and remove operations is O(N).

This internally holds statically allocated array of size `N` and stores elements into this array, which avoids dynamic memory allocation and deallocation. This container **never** uses dynamic memory management. The number of elements in this container **cannot** be greater than `N`. Attempting to insert more than `N` elements into this container results in **undefined behavior**.

Elements of this container are always stored **contiguously** in the memory.

Iterators to elements are random access iterators and they meet the requirements of [*LegacyRandomAccessIterator*](https://en.cppreference.com/w/cpp/named_req/RandomAccessIterator).

`sfl::static_unordered_flat_map` meets the requirements of [*Container*](https://en.cppreference.com/w/cpp/named_req/Container) and [*ContiguousContainer*](https://en.cppreference.com/w/cpp/named_req/ContiguousContainer). The requirements of [*UnorderedAssociativeContainer*](https://en.cppreference.com/w/cpp/named_req/UnorderedAssociativeContainer) are partionally met (this container doesn't use [*Hash*](https://en.cppreference.com/w/cpp/named_req/Hash)).

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
    typename KeyEqual
    ```

    Function for comparing keys.

<br><br>



## Public Member Types

| Member Type               | Definition |
| :------------------------ | :--------- |
| `key_type`                | `Key` |
| `mapped_type`             | `T` |
| `value_type`              | `std::pair<Key, T>` |
| `size_type`               | `std::size_t` |
| `difference_type`         | `std::ptrdiff_t` |
| `key_equal`               | `KeyEqual` |
| `reference`               | `value_type&` |
| `const_reference`         | `const value_type&` |
| `pointer`                 | `value_type*` |
| `const_pointer`           | `const value_type*` |
| `iterator`                | [*LegacyRandomAccessIterator*](https://en.cppreference.com/w/cpp/named_req/RandomAccessIterator) and [*LegacyContiguousIterator*](https://en.cppreference.com/w/cpp/named_req/ContiguousIterator) to `value_type` |
| `const_iterator`          | [*LegacyRandomAccessIterator*](https://en.cppreference.com/w/cpp/named_req/RandomAccessIterator) and [*LegacyContiguousIterator*](https://en.cppreference.com/w/cpp/named_req/ContiguousIterator) to `const value_type` |

<br><br>



## Public Member Classes

### value_equal

```
class value_equal
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
    static_unordered_flat_map() noexcept(std::is_nothrow_default_constructible<KeyEqual>::value)
    ```
2.  ```
    explicit static_unordered_flat_map(const KeyEqual& equal) noexcept(std::is_nothrow_copy_constructible<KeyEqual>::value)
    ```

    **Effects:**
    Constructs an empty container.

    <br><br>



3.  ```
    template <typename InputIt>
    static_unordered_flat_map(InputIt first, InputIt last);
    ```
4.  ```
    template <typename InputIt>
    static_unordered_flat_map(InputIt first, InputIt last, const KeyEqual& equal);
    ```

    **Preconditions:**
    `std::distance(first, last) <= capacity()`

    **Effects:**
    Constructs the container with the contents of the range `[first, last)`.

    If multiple elements in the range have keys that compare equivalent, then the first element is inserted.

    **Note:**
    These overloads participate in overload resolution only if `InputIt` satisfies requirements of [*LegacyInputIterator*](https://en.cppreference.com/w/cpp/named_req/InputIterator).

    **Complexity:**
    Linear in `std::distance(first, last)`.

    <br><br>



5.  ```
    static_unordered_flat_map(std::initializer_list<value_type> ilist);
    ```
6.  ```
    static_unordered_flat_map(std::initializer_list<value_type> ilist, const KeyEqual& equal);
    ```

    **Preconditions:**
    `ilist.size() <= capacity()`

    **Effects:**
    Constructs the container with the contents of the initializer list `ilist`.

    If multiple elements in the range have keys that compare equivalent, then the first element is inserted.

    **Complexity:**
    Linear in `ilist.size()`.

    <br><br>



7.  ```
    static_unordered_flat_map(const static_unordered_flat_map& other);
    ```

    **Effects:**
    Copy constructor.
    Constructs the container with the copy of the contents of `other`.

    **Complexity:**
    Linear in size.

    <br><br>



8.  ```
    static_unordered_flat_map(static_unordered_flat_map&& other);
    ```

    **Effects:**
    Move constructor.
    Constructs the container with the contents of `other` using move semantics.

    `other` is not guaranteed to be empty after the move.

    `other` is in a valid but unspecified state after the move.

    **Complexity:**
    Linear in size.

    <br><br>



9.  ```
    template <typename Range>
    static_unordered_flat_map(sfl::from_range_t, Range&& range);
    ```
10. ```
    template <typename Range>
    static_unordered_flat_map(sfl::from_range_t, Range&& range, const KeyEqual& equal);
    ```

    **Effects:**
    Constructs the container with the contents of `range`.

    If multiple elements in the range have keys that compare equivalent, then the first element is inserted.

    **Note:**
    It is available in C++11. In C++20 are used proper C++20 range concepts.

    <br><br>



### (destructor)

1.  ```
    ~static_unordered_flat_map();
    ```

    **Effects:**
    Destructs the container. The destructors of the elements are called and the used storage is deallocated.

    **Complexity:**
    Linear in size.

    <br><br>



### operator=

1.  ```
    static_unordered_flat_map& operator=(const static_unordered_flat_map& other);
    ```

    **Effects:**
    Copy assignment operator.
    Replaces the contents with a copy of the contents of `other`.

    **Returns:**
    `*this()`.

    **Complexity:**
    Linear in size.

    <br><br>



2.  ```
    static_unordered_flat_map& operator=(static_unordered_flat_map&& other);
    ```

    **Effects:**
    Move assignment operator.
    Replaces the contents with those of `other` using move semantics.

    `other` is not guaranteed to be empty after the move.

    `other` is in a valid but unspecified state after the move.

    **Returns:**
    `*this()`.

    **Complexity:**
    Linear in size.

    <br><br>



3.  ```
    static_unordered_flat_map& operator=(std::initializer_list<value_type> ilist);
    ```

    **Preconditions:**
    `ilist.size() <= capacity()`

    **Effects:**
    Replaces the contents with those identified by initializer list `ilist`.

    **Returns:**
    `*this()`.

    **Complexity:**
    Linear in size.

    <br><br>



### key_eq

1.  ```
    key_equal key_eq() const;
    ```

    **Effects:**
    Returns the function object that compares keys for equality, which is a copy of this container's constructor argument `equal`.

    **Complexity:**
    Constant.

    <br><br>



### value_eq

1.  ```
    value_equal value_eq() const;
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



### nth

1.  ```
    iterator nth(size_type pos) noexcept;
    ```
2.  ```
    const_iterator nth(size_type pos) const noexcept;
    ```

    **Preconditions:**
    `pos <= size()`

    **Effects:**
    Returns an iterator to the element at position `pos`.

    If `pos == size()`, the returned iterator is equal to `end()`.

    **Complexity:**
    Constant.

    <br><br>



### index_of

1.  ```
    size_type index_of(const_iterator pos) const noexcept;
    ```

    **Preconditions:**
    `cbegin() <= pos && pos <= cend()`

    **Effects:**
    Returns position of the element pointed by iterator `pos`, i.e. `std::distance(begin(), pos)`.

    If `pos == end()`, the returned value is equal to `size()`.

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
    1. `!full()`
    2. `cbegin() <= hint && hint <= cend()`

    **Effects:**
    Inserts new element into the container if the container doesn't already contain an element with an equivalent key.

    New element is constructed as `value_type(std::forward<Args>(args)...)`.

    The element may be constructed even if there already is an element with the key in the container, in which case the newly constructed element will be destroyed immediately.

    Iterator `hint` is used as a suggestion where to start to search insert position.

    Iterator `hint` is ignored due to container's underlying storage implementation. This overload exists just to have this container compatible with standard C++ containers as much as possible.

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
    1. `!full()`
    2. `cbegin() <= hint && hint <= cend()`

    **Effects:**
    Inserts copy of `value` if the container doesn't already contain an element with an equivalent key.

    Iterator `hint` is used as a suggestion where to start to search insert position.

    Iterator `hint` is ignored due to container's underlying storage implementation. This overload exists just to have this container compatible with standard C++ containers as much as possible.

    **Returns:**
    Iterator to the inserted element or to the already existing element.

    <br><br>



5.  ```
    iterator insert(const_iterator hint, value_type&& value);
    ```

    **Preconditions:**
    1. `!full()`
    2. `cbegin() <= hint && hint <= cend()`

    **Effects:**
    Inserts `value` using move semantics if the container doesn't already contain an element with an equivalent key.

    Iterator `hint` is used as a suggestion where to start to search insert position.

    Iterator `hint` is ignored due to container's underlying storage implementation. This overload exists just to have this container compatible with standard C++ containers as much as possible.

    **Returns:**
    Iterator to the inserted element or to the already existing element.

    <br><br>



6.  ```
    template <typename P>
    iterator insert(const_iterator hint, P&& value);
    ```

    **Preconditions:**
    1. `!full()`
    2. `cbegin() <= hint && hint <= cend()`

    **Effects:**
    Inserts new element into the container if the container doesn't already contain an element with an equivalent key.

    New element is constructed as `value_type(std::forward<P>(value))`.

    Iterator `hint` is used as a suggestion where to start to search insert position.

    Iterator `hint` is ignored due to container's underlying storage implementation. This overload exists just to have this container compatible with standard C++ containers as much as possible.

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

    **Preconditions:**
    `!full()`

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
        1. `KeyEqual::is_transparent` exists and is a valid type. It allows calling this function without constructing an instance of `Key`.
        2. `std::is_assignable_v<mapped_type&, M&&>` is `true`.

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

    **Preconditions:**
    1. `!full()`
    2. `cbegin() <= hint && hint <= cend()`

    **Effects:**
    If a key equivalent to `key` already exists in the container, assigns `std::forward<M>(obj)` to the mapped type corresponding to the key `key`. If the key does not exist, inserts the new element.

    Iterator `hint` is used as a suggestion where to start to search insert position.

    Iterator `hint` is ignored due to container's underlying storage implementation. These overloads exist just to have this container compatible with standard C++ containers as much as possible.

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
        1. `KeyEqual::is_transparent` exists and is a valid type. It allows calling this function without constructing an instance of `Key`.
        2. `std::is_assignable_v<mapped_type&, M&&>` is `true`.

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
        1. `KeyEqual::is_transparent` exists and is a valid type. It allows calling this function without constructing an instance of `Key`.
        2. `std::is_convertible_v<K&&, iterator>` is `false`.
        3. `std::is_convertible_v<K&&, const_iterator>` is `false`.

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
    1. `!full()`
    2. `cbegin() <= hint && hint <= cend()`

    **Effects:**
    If a key equivalent to `key` already exists in the container, does nothing.
    Otherwise, inserts a new element into the container.

    Iterator `hint` is used as a suggestion where to start to search insert position.

    Iterator `hint` is ignored due to container's underlying storage implementation. These overloads exist just to have this container compatible with standard C++ containers as much as possible.

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

        **Note:** This overload participates in overload resolution only if `KeyEqual::is_transparent` exists and is a valid type. It allows calling this function without constructing an instance of `Key`.

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

    **Preconditions:**
    `cbegin() <= pos && pos < cend()`

    **Effects:**
    Removes the element at `pos`.

    **Returns:**
    Iterator following the last removed element.

    <br><br>



3.  ```
    iterator erase(const_iterator first, const_iterator last);
    ```

    **Preconditions:**
    `cbegin() <= first && first <= last && last <= cend()`

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
    Overload (5) participates in overload resolution only if `KeyEqual::is_transparent` exists and is a valid type. It allows calling this function without constructing an instance of `Key`.

    **Returns:**
    Number of elements removed (0 or 1).

    <br><br>



### swap

1.  ```
    void swap(small_flat_map& other);
    ```

    **Effects:**
    Exchanges the contents of the container with those of `other`.

    **Complexity:**
    Linear in size.

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
    Overloads (3) and (4) participate in overload resolution only if `KeyEqual::is_transparent` exists and is a valid type. It allows calling these functions without constructing an instance of `Key`.

    **Complexity:**
    Constant in the best case. Linear in `size()` in the worst case.

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
    Overload (2) participates in overload resolution only if `KeyEqual::is_transparent` exists and is a valid type. It allows calling this function without constructing an instance of `Key`.

    **Complexity:**
    Constant in the best case. Linear in `size()` in the worst case.

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
    Overload (2) participates in overload resolution only if `KeyEqual::is_transparent` exists and is a valid type. It allows calling this function without constructing an instance of `Key`.

    **Complexity:**
    Constant in the best case. Linear in `size()` in the worst case.

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
    Overload (3) participates in overload resolution only if `KeyEqual::is_transparent` exists and is a valid type. It allows calling this function without constructing an instance of `Key`.

    **Complexity:**
    Linear in `size()`.

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

    * Overload (1) is equivalent to
      `return try_emplace(key).first->second;`

    * Overload (2) is equivalent to
      `return try_emplace(std::move(key)).first->second;`

    * Overload (3) is equivalent to
      `return try_emplace(x).first->second;`

    * Overload (4) is equivalent to
      `return try_emplace(std::forward<K>(x)).first->second;`

    **Note:**
    Overloads (3) and (4) participate in overload resolution only if `KeyEqual::is_transparent` exists and is a valid type. It allows calling these functions without constructing an instance of `Key`.

    **Complexity:**
    Linear in `size()`.

    <br><br>



### data

1.  ```
    value_type* data() noexcept;
    ```
2.  ```
    const value_type* data() const noexcept;
    ```

    **Effects:**
    Returns pointer to the underlying array serving as element storage. The pointer is such that range `[data(), data() + size())` is always a valid range, even if the container is empty. `data()` is not dereferenceable if the container is empty.

    **Complexity:**
    Constant.

    <br><br>



## Non-member Functions

### operator==

1.  ```
    template <typename K, typename T, std::size_t N, typename E>
    bool operator==
    (
        const static_unordered_flat_map<K, T, N, E>& x,
        const static_unordered_flat_map<K, T, N, E>& y
    );
    ```

    **Effects:**
    Checks if the contents of `x` and `y` are equal.

    The contents of `x` and `y` are equal if the following conditions hold:
    * `x.size() == y.size()`
    * For each element in `x` there is equal element in `y`.

    The comparison is performed by `std::is_permutation`.
    This comparison ignores the container's `KeyEqual` function.

    **Returns:**
    `true` if the contents of the `x` and `y` are equal, `false` otherwise.

    <br><br>



### operator!=

1.  ```
    template <typename K, typename T, std::size_t N, typename E>
    bool operator!=
    (
        const static_unordered_flat_map<K, T, N, E>& x,
        const static_unordered_flat_map<K, T, N, E>& y
    );
    ```

    **Effects:**
    Checks if the contents of `x` and `y` are equal.

    For details see `operator==`.

    **Returns:**
    `true` if the contents of the `x` and `y` are not equal, `false` otherwise.

    <br><br>



### swap

1.  ```
    template <typename K, typename T, std::size_t N, typename E>
    void swap
    (
        static_unordered_flat_map<K, T, N, E>& x,
        static_unordered_flat_map<K, T, N, E>& y
    );
    ```

    **Effects:**
    Swaps the contents of `x` and `y`. Calls `x.swap(y)`.

    <br><br>



### erase_if

1.  ```
    template <typename K, typename T, std::size_t N, typename E, typename Predicate>
    typename static_unordered_flat_map<K, T, N, E>::size_type
        erase_if(static_unordered_flat_map<K, T, N, E>& c, Predicate pred);
    ```

    **Effects:**
    Erases all elements that satisfy the predicate `pred` from the container.

    `pred` is unary predicate which returns `true` if the element should be removed.

    **Returns:**
    The number of erased elements.

    **Complexity:**
    Linear.

    <br><br>



End of document.
