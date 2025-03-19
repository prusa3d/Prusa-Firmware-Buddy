# sfl::static_vector

<details>

<summary>Table of Contents</summary>

* [Summary](#summary)
* [Template Parameters](#template-parameters)
* [Public Member Types](#public-member-types)
* [Public Data Members](#public-data-members)
  * [static\_capacity](#static_capacity)
* [Public Member Functions](#public-member-functions)
  * [(constructor)](#constructor)
  * [(destructor)](#destructor)
  * [assign](#assign)
  * [assign\_range](#assign_range)
  * [operator=](#operator)
  * [begin, cbegin](#begin-cbegin)
  * [end, cend](#end-cend)
  * [rbegin, crbegin](#rbegin-crbegin)
  * [rend, crend](#rend-crend)
  * [nth](#nth)
  * [index\_of](#index_of)
  * [empty](#empty)
  * [full](#full)
  * [size](#size)
  * [max\_size](#max_size)
  * [capacity](#capacity)
  * [available](#available)
  * [at](#at)
  * [operator\[\]](#operator-1)
  * [front](#front)
  * [back](#back)
  * [data](#data)
  * [clear](#clear)
  * [emplace](#emplace)
  * [insert](#insert)
  * [insert\_range](#insert_range)
  * [emplace\_back](#emplace_back)
  * [push\_back](#push_back)
  * [append\_range](#append_range)
  * [pop\_back](#pop_back)
  * [erase](#erase)
  * [resize](#resize)
  * [swap](#swap)
* [Non-member Functions](#non-member-functions)
  * [operator==](#operator-2)
  * [operator!=](#operator-3)
  * [operator\<](#operator-4)
  * [operator\>](#operator-5)
  * [operator\<=](#operator-6)
  * [operator\>=](#operator-7)
  * [swap](#swap-1)
  * [erase](#erase-1)
  * [erase\_if](#erase_if)

</details>



## Summary

Defined in header `sfl/static_vector.hpp`:

```
namespace sfl
{
    template < typename T, std::size_t N >
    class static_vector;
}
```

`sfl::static_vector` is a sequence container similar to [`std::vector`](https://en.cppreference.com/w/cpp/container/vector), but with the different storage model.

This container internally holds statically allocated array of size `N` and stores elements into this array, which avoids dynamic memory allocation and deallocation. This container **never** uses dynamic memory management. The number of elements in static vector **cannot** be greater than `N`. Attempting to insert more than `N` elements into this container results in **undefined behavior**.

`sfl::static_vector` is **not** specialized for `bool`.

`sfl::static_vector` meets the requirements of [*Container*](https://en.cppreference.com/w/cpp/named_req/Container), [*ReversibleContainer*](https://en.cppreference.com/w/cpp/named_req/ReversibleContainer), [*ContiguousContainer*](https://en.cppreference.com/w/cpp/named_req/ContiguousContainer) and [*SequenceContainer*](https://en.cppreference.com/w/cpp/named_req/SequenceContainer).

This container is convenient for bare-metal embedded software development.

<br><br>



## Template Parameters

1.  ```
    typename T
    ```

    The type of the elements.

2.  ```
    std::size_t N
    ```

    Size of the internal statically allocated array, i.e. the maximal number of elements that this container can contain.

<br><br>



## Public Member Types

| Member Type               | Definition |
| :------------------------ | :--------- |
| `value_type`              | `T` |
| `size_type`               | `std::size_t` |
| `difference_type`         | `std::ptrdiff_t` |
| `reference`               | `value_type&` |
| `const_reference`         | `const value_type&` |
| `pointer`                 | `value_type*` |
| `const_pointer`           | `const value_type*` |
| `iterator`                | [*LegacyRandomAccessIterator*](https://en.cppreference.com/w/cpp/named_req/RandomAccessIterator) and [*LegacyContiguousIterator*](https://en.cppreference.com/w/cpp/named_req/ContiguousIterator) to `value_type` |
| `const_iterator`          | [*LegacyRandomAccessIterator*](https://en.cppreference.com/w/cpp/named_req/RandomAccessIterator) and [*LegacyContiguousIterator*](https://en.cppreference.com/w/cpp/named_req/ContiguousIterator) to `const value_type` |
| `reverse_iterator`        | `std::reverse_iterator<iterator>` |
| `const_reverse_iterator`  | `std::reverse_iterator<const_iterator>` |

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
    static_vector() noexcept;
    ```

    **Effects:**
    Constructs an empty container.

    **Complexity:**
    Constant.

    <br><br>



2.  ```
    static_vector(size_type n);
    ```

    **Preconditions:**
    `n <= capacity()`

    **Effects:**
    Constructs the container with `n` [value-initialized](https://en.cppreference.com/w/cpp/language/value_initialization) elements.

    **Complexity:**
    Linear in `n`.

    <br><br>



3.  ```
    static_vector(size_type n, sfl::default_init_t);
    ```

    **Preconditions:**
    `n <= capacity()`

    **Effects:**
    Constructs the container with `n` [default-initialized](https://en.cppreference.com/w/cpp/language/default_initialization) elements.

    **Complexity:**
    Linear in `n`.

    <br><br>



4.  ```
    static_vector(size_type n, const T& value);
    ```

    **Preconditions:**
    `n <= capacity()`

    **Effects:**
    Constructs the container with `n` copies of elements with value `value`.

    **Complexity:**
    Linear in `n`.

    <br><br>



5.  ```
    template <typename InputIt>
    static_vector(InputIt first, InputIt last);
    ```

    **Preconditions:**
    `std::distance(first, last) <= capacity()`

    **Effects:**
    Constructs the container with the contents of the range `[first, last)`.

    **Note:**
    This overload participates in overload resolution only if `InputIt` satisfies requirements of [*LegacyInputIterator*](https://en.cppreference.com/w/cpp/named_req/InputIterator).

    **Complexity:**
    Linear in `std::distance(first, last)`.

    <br><br>



6.  ```
    static_vector(std::initializer_list<T> ilist);
    ```

    **Preconditions:**
    `ilist.size() <= capacity()`

    **Effects:**
    Constructs the container with the contents of the initializer list `ilist`.

    **Complexity:**
    Linear in `ilist.size()`.

    <br><br>



7.  ```
    static_vector(const static_vector& other);
    ```

    **Effects:**
    Copy constructor.
    Constructs the container with the copy of the contents of `other`.

    **Complexity:**
    Linear in size.

    <br><br>



8.  ```
    static_vector(static_vector&& other);
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
    static_vector(sfl::from_range_t, Range&& range);
    ```

    **Preconditions:**
    Number of elements in `range` must be `<= capacity()`.

    **Effects:**
    Constructs the container with the contents of `range`.

    **Note:**
    It is available in C++11. In C++20 are used proper C++20 range concepts.

    <br><br>



### (destructor)

1.  ```
    ~static_vector();
    ```

    **Effects:**
    Destructs the container. The destructors of the elements are called and the used storage is deallocated.

    **Complexity:**
    Linear in size.

    <br><br>



### assign

1.  ```
    void assign(size_type n, const T& value);
    ```

    **Preconditions:**
    `n <= capacity()`

    **Effects:**
    Replaces the contents of the container with `n` copies of value `value`.

    **Complexity:**
    Linear in size.

    <br><br>



2.  ```
    template <typename InputIt>
    void assign(InputIt first, InputIt last);
    ```

    **Preconditions:**
    `std::distance(first, last) <= capacity()`

    **Effects:**
    Replaces the contents of the container with the contents of the range `[first, last)`.

    **Note:**
    This overload participates in overload resolution only if `InputIt` satisfies requirements of [*LegacyInputIterator*](https://en.cppreference.com/w/cpp/named_req/InputIterator).

    **Note:**
    The behavior is undefined if either `first` or `last` is an iterator into `*this`.

    **Complexity:**
    Linear in size.

    <br><br>



3.  ```
    void assign(std::initializer_list<T> ilist);
    ```

    **Preconditions:**
    `ilist.size() <= capacity()`

    **Effects:**
    Replaces the contents of the container with the contents of the initializer list `ilist`.

    **Complexity:**
    Linear in size.

    <br><br>



### assign_range

1.  ```
    template <typename Range>
    void assign_range(Range&& range);
    ```

    **Preconditions:**
    Number of elements in `range` must be `<= capacity()`.

    **Effects:**
    Replaces the contents of the container with the contents of `range`.

    **Note:**
    It is available in C++11. In C++20 are used proper C++20 range concepts.

    <br><br>



### operator=

1.  ```
    static_vector& operator=(const static_vector& other);
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
    static_vector& operator=(static_vector&& other);
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
    static_vector& operator=(std::initializer_list<T> ilist);
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



### at

1.  ```
    reference at(size_type pos);
    ```
2.  ```
    const_reference at(size_type pos) const;
    ```

    **Effects:**
    Returns a reference to the element at specified location `pos`, with bounds checking.

    **Complexity:**
    Constant.

    **Exceptions:**
    `std::out_of_range` if `pos >= size()`.

    <br><br>



### operator[]

1.  ```
    reference operator[](size_type pos) noexcept;
    ```
2.  ```
    const_reference operator[](size_type pos) const noexcept;
    ```

    **Preconditions:**
    `pos < size()`

    **Effects:**
    Returns a reference to the element at specified location pos. No bounds checking is performed.

    **Note:**
    This operator never inserts a new element into the container.

    **Complexity:**
    Constant.

    <br><br>



### front

1.  ```
    reference front() noexcept;
    ```
2.  ```
    const_reference front() const noexcept;
    ```

    **Preconditions:**
    `!empty()`

    **Effects:**
    Returns a reference to the first element in the container.

    **Complexity:**
    Constant.

    <br><br>



### back

1.  ```
    reference back() noexcept;
    ```
2.  ```
    const_reference back() const noexcept;
    ```

    **Preconditions:**
    `!empty()`

    **Effects:**
    Returns a reference to the last element in the container.

    **Complexity:**
    Constant.

    <br><br>



### data

1.  ```
    T* data() noexcept;
    ```
2.  ```
    const T* data() const noexcept;
    ```

    **Effects:**
    Returns pointer to the underlying array serving as element storage. The pointer is such that range `[data(), data() + size())` is always a valid range, even if the container is empty. `data()` is not dereferenceable if the container is empty.

    **Complexity:**
    Constant.

    <br><br>



### clear

1.  ```
    void clear() noexcept;
    ```

    **Effects:**
    Erases all elements from the container.
    After this call, `size()` returns zero.

    **Complexity:**
    Linear in `size()`.

    <br><br>



### emplace

1.  ```
    template <typename... Args>
    iterator emplace(const_iterator pos, Args&&... args);
    ```

    **Preconditions:**
    1. `!full()`
    2. `cbegin() <= pos && pos <= cend()`

    **Effects:**
    Inserts a new element into the container at position `pos`.

    New element is constructed as `value_type(std::forward<Args>(args)...)`.

    `args...` may directly or indirectly refer to a value in the container.

    **Returns:**
    Iterator to the inserted element.

    **Complexity:**
    Constant plus linear in `std::distance(pos, end())`.

    <br><br>



### insert

1.  ```
    iterator insert(const_iterator pos, const T& value);
    ```

    **Preconditions:**
    1. `!full()`
    2. `cbegin() <= pos && pos <= cend()`

    **Effects:**
    Inserts copy of `value` at position `pos`.

    **Returns:**
    Iterator to the inserted element.

    **Complexity:**
    Constant plus linear in `std::distance(pos, end())`.

    <br><br>



2.  ```
    iterator insert(const_iterator pos, T&& value);
    ```

    **Preconditions:**
    1. `!full()`
    2. `cbegin() <= pos && pos <= cend()`

    **Effects:**
    Inserts `value` using move semantics at position `pos`.

    **Returns:**
    Iterator to the inserted element.

    **Complexity:**
    Constant plus linear in `std::distance(pos, end())`.

    <br><br>



3.  ```
    iterator insert(const_iterator pos, size_type n, const T& value);
    ```

    **Preconditions:**
    1. `n <= available()`
    2. `cbegin() <= pos && pos <= cend()`

    **Effects:**
    Inserts `n` copies of `value` before position `pos`.

    **Returns:**
    Iterator to the first element inserted, or `pos` if `n == 0`.

    **Complexity:**
    Linear in `n` plus linear in `std::distance(pos, end())`.

    <br><br>



4.  ```
    template <typename InputIt>
    iterator insert(const_iterator pos, InputIt first, InputIt last);
    ```

    **Preconditions:**
    1. `std::distance(first, last) <= available()`
    2. `cbegin() <= pos && pos <= cend()`

    **Effects:**
    Inserts elements from the range `[first, last)` before position `pos`.

    **Note:**
    This overload participates in overload resolution only if `InputIt` satisfies requirements of [*LegacyInputIterator*](https://en.cppreference.com/w/cpp/named_req/InputIterator).

    **Returns:**
    Iterator to the first element inserted, or `pos` if `first == last`.

    **Complexity:**
    Linear in `std::distance(first, last)` plus linear in `std::distance(pos, end())`.

    <br><br>



5.  ```
    iterator insert(const_iterator pos, std::initializer_list<T> ilist);
    ```

    **Preconditions:**
    1. `ilist.size() <= available()`
    2. `cbegin() <= pos && pos <= cend()`

    **Effects:**
    Inserts elements from initializer list `ilist` before position `pos`.

    **Returns:**
    Iterator to the first element inserted, or `pos` if `ilist` is empty.

    **Complexity:**
    Linear in `ilist.size()` plus linear in `std::distance(pos, end())`.

    <br><br>



### insert_range

1.  ```
    template <typename Range>
    iterator insert_range(const_iterator pos, Range&& range);
    ```

    **Preconditions:**
    Number of elements in `range` must be `<= available()`.

    **Effects:**
    Inserts elements from `range` before position `pos`. Elements are inserted in non-reversing order.

    `range` must not overlap with the container. Otherwise, the behavior is undefined.

    **Returns:**
    Iterator to the first element inserted, or `pos` if `range` is empty.

    **Note:**
    It is available in C++11. In C++20 are used proper C++20 range concepts.

    <br><br>



### emplace_back

1.  ```
    template <typename... Args>
    reference emplace_back(Args&&... args);
    ```

    **Preconditions:**
    `!full()`

    **Effects:**
    Inserts a new element at the end of container.

    New element is constructed as `value_type(std::forward<Args>(args)...)`.

    **Returns:**
    Reference to the inserted element.

    **Complexity:**
    Constant.

    <br><br>



### push_back

1.  ```
    void push_back(const T& value);
    ```

    **Preconditions:**
    `!full()`

    **Effects:**
    Inserts copy of `value` at the end of container.

    **Complexity:**
    Constant.

    <br><br>



2.  ```
    void push_back(T&& value);
    ```

    **Preconditions:**
    `!full()`

    **Effects:**
    Inserts `value` using move semantics at the end of container.

    **Complexity:**
    Constant.

    <br><br>



### append_range

1.  ```
    template <typename Range>
    void append_range(Range&& range);
    ```

    **Preconditions:**
    Number of elements in `range` must be `<= available()`.

    **Effects:**
    Inserts elements from `range` before `end()`. Elements are inserted in non-reversing order.

    **Note:**
    It is available in C++11. In C++20 are used proper C++20 range concepts.

    <br><br>



### pop_back

1.  ```
    void pop_back();
    ```

    **Preconditions:**
    `!empty()`

    **Effects:**
    Removes the last element of the container.

    **Complexity:**
    Constant.

    <br><br>



### erase

1.  ```
    iterator erase(const_iterator pos);
    ```

    **Preconditions:**
    `cbegin() <= pos && pos < cend()`

    **Effects:**
    Removes the element at `pos`.

    **Returns:**
    Iterator following the last removed element.

    If `pos` refers to the last element, then the `end()` iterator is returned.

    <br><br>



2.  ```
    iterator erase(const_iterator first, const_iterator last);
    ```

    **Preconditions:**
    `cbegin() <= first && first <= last && last <= cend()`

    **Effects:**
    Removes the elements in the range `[first, last)`.

    **Returns:**
    Iterator following the last removed element.

    If `last == end()` prior to removal, then the updated `end()` iterator is returned.

    If `[first, last)` is an empty range, then `last` is returned.

    <br><br>



### resize

1.  ```
    void resize(size_type n);
    ```

    **Preconditions:**
    `n <= capacity()`

    **Effects:**
    Resizes the container to contain `n` elements.

    1. If the `n > size()`, additional elements are inserted at the end of container. Additional elements are [value-initialized](https://en.cppreference.com/w/cpp/language/value_initialization).
    2. If the `n < size()`, the last `size() - n` elements are removed.

    **Complexity:**
    Linear in difference between `size()` and `n`.

    <br><br>



2.  ```
    void resize(size_type n, sfl::default_init_t);
    ```

    **Preconditions:**
    `n <= capacity()`

    **Effects:**
    Resizes the container to contain `n` elements.

    1. If the `n > size()`, additional elements are inserted at the end of container. Additional elements are [default-initialized](https://en.cppreference.com/w/cpp/language/default_initialization).
    2. If the `n < size()`, the last `size() - n` elements are removed.

    **Complexity:**
    Linear in difference between `size()` and `n`.

    <br><br>



3.  ```
    void resize(size_type n, const T& value);
    ```

    **Preconditions:**
    `n <= capacity()`

    **Effects:**
    Resizes the container to contain `n` elements.

    1. If the `n > size()`, additional copies of `value` are inserted at the end of container.
    2. If the `n < size()`, the last `size() - n` elements are removed.

    **Complexity:**
    Linear in difference between `size()` and `n`.

    <br><br>



### swap

1.  ```
    void swap(static_vector& other);
    ```

    **Effects:**
    Exchanges the contents of the container with those of `other`.

    **Complexity:**
    Linear in size.

    <br><br>



## Non-member Functions

### operator==

1.  ```
    template <typename T, std::size_t N>
    bool operator==
    (
        const static_vector<T, N>& x,
        const static_vector<T, N>& y
    );
    ```

    **Effects:**
    Checks if the contents of `x` and `y` are equal.

    The contents of `x` and `y` are equal if the following conditions hold:
    * `x.size() == y.size()`
    * Each element in `x` compares equal with the element in `y` at the same position.

    **Returns:**
    `true` if the contents of the `x` and `y` are equal, `false` otherwise.

    **Complexity:**
    Constant if `x` and `y` are of different size, otherwise linear in the size of the container.

    <br><br>



### operator!=

1.  ```
    template <typename T, std::size_t N>
    bool operator!=
    (
        const static_vector<T, N>& x,
        const static_vector<T, N>& y
    );
    ```

    **Effects:**
    Checks if the contents of `x` and `y` are equal.

    For details see `operator==`.

    **Returns:**
    `true` if the contents of the `x` and `y` are not equal, `false` otherwise.

    **Complexity:**
    Constant if `x` and `y` are of different size, otherwise linear in the size of the container.

    <br><br>



### operator<

1.  ```
    template <typename T, std::size_t N>
    bool operator<
    (
        const static_vector<T, N>& x,
        const static_vector<T, N>& y
    );
    ```

    **Effects:**
    Compares the contents of `x` and `y` lexicographically.
    The comparison is performed by a function `std::lexicographical_compare`.

    **Returns:**
    `true` if the contents of the `x` are lexicographically less than the contents of `y`, `false` otherwise.

    **Complexity:**
    Linear in the size of the container.

    <br><br>



### operator>

1.  ```
    template <typename T, std::size_t N>
    bool operator>
    (
        const static_vector<T, N>& x,
        const static_vector<T, N>& y
    );
    ```

    **Effects:**
    Compares the contents of `x` and `y` lexicographically.
    The comparison is performed by a function `std::lexicographical_compare`.

    **Returns:**
    `true` if the contents of the `x` are lexicographically greater than the contents of `y`, `false` otherwise.

    **Complexity:**
    Linear in the size of the container.

    <br><br>



### operator<=

1.  ```
    template <typename T, std::size_t N>
    bool operator<=
    (
        const static_vector<T, N>& x,
        const static_vector<T, N>& y
    );
    ```

    **Effects:**
    Compares the contents of `x` and `y` lexicographically.
    The comparison is performed by a function `std::lexicographical_compare`.

    **Returns:**
    `true` if the contents of the `x` are lexicographically less than or equal to the contents of `y`, `false` otherwise.

    **Complexity:**
    Linear in the size of the container.

    <br><br>



### operator>=

1.  ```
    template <typename T, std::size_t N>
    bool operator>=
    (
        const static_vector<T, N>& x,
        const static_vector<T, N>& y
    );
    ```

    **Effects:**
    Compares the contents of `x` and `y` lexicographically.
    The comparison is performed by a function `std::lexicographical_compare`.

    **Returns:**
    `true` if the contents of the `x` are lexicographically greater than or equal to the contents of `y`, `false` otherwise.

    **Complexity:**
    Linear in the size of the container.

    <br><br>



### swap

1.  ```
    template <typename T, std::size_t N>
    void swap
    (
        static_vector<T, N>& x,
        static_vector<T, N>& y
    );
    ```

    **Effects:**
    Swaps the contents of `x` and `y`. Calls `x.swap(y)`.

    <br><br>



### erase

1.  ```
    template <typename T, std::size_t N, typename U>
    typename static_vector<T, N>::size_type
        erase(static_vector<T, N>& c, const U& value);
    ```

    **Effects:**
    Erases all elements that compare equal to `value` from the container.

    **Returns:**
    The number of erased elements.

    **Complexity:**
    Linear.

    <br><br>



### erase_if

1.  ```
    template <typename T, std::size_t N, typename Predicate>
    typename static_vector<T, N>::size_type
        erase_if(static_vector<T, N>& c, Predicate pred);
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
