# sfl::devector

<details>

<summary>Table of Contents</summary>

* [Summary](#summary)
* [Template Parameters](#template-parameters)
* [Public Member Types](#public-member-types)
* [Public Member Functions](#public-member-functions)
  * [(constructor)](#constructor)
  * [(destructor)](#destructor)
  * [assign](#assign)
  * [assign\_range](#assign_range)
  * [operator=](#operator)
  * [get\_allocator](#get_allocator)
  * [begin, cbegin](#begin-cbegin)
  * [end, cend](#end-cend)
  * [rbegin, crbegin](#rbegin-crbegin)
  * [rend, crend](#rend-crend)
  * [nth](#nth)
  * [index\_of](#index_of)
  * [empty](#empty)
  * [size](#size)
  * [max\_size](#max_size)
  * [capacity](#capacity)
  * [available\_front](#available_front)
  * [available\_back](#available_back)
  * [reserve\_front](#reserve_front)
  * [reserve\_back](#reserve_back)
  * [shrink\_to\_fit](#shrink_to_fit)
  * [at](#at)
  * [operator\[\]](#operator-1)
  * [front](#front)
  * [back](#back)
  * [data](#data)
  * [clear](#clear)
  * [emplace](#emplace)
  * [insert](#insert)
  * [insert\_range](#insert_range)
  * [emplace\_front](#emplace_front)
  * [emplace\_back](#emplace_back)
  * [push\_front](#push_front)
  * [push\_back](#push_back)
  * [prepend\_range](#prepend_range)
  * [append\_range](#append_range)
  * [pop\_front](#pop_front)
  * [pop\_back](#pop_back)
  * [erase](#erase)
  * [resize](#resize)
  * [resize\_front](#resize_front)
  * [resize\_back](#resize_back)
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
* [Deduction Guides (since C++17)](#deduction-guides-since-c17)

</details>



## Summary

Defined in header `sfl/devector.hpp`:

```
namespace sfl
{
    template < typename T,
               typename Allocator = std::allocator<T> >
    class devector;
}
```

`sfl::devector` (double-ended vector) is a sequence container similar to [`std::vector`](https://en.cppreference.com/w/cpp/container/vector) with slightly different underlying storage that allows fast insertion and deletion at both its beginning and its end.

`sfl::devector` has member functions `push_front` and `pop_front` that allow fast insertion and deletion at the beginning of the container. Other member function like `emplace`, `insert` and `erase` also allow faster insertion and deletion at the beginning of the container than it is in case of `std::vector`.

Elements of `sfl::devector` are always stored contiguously in the memory.

`sfl::devector` is **not** specialized for `bool`.

`sfl::devector` meets the requirements of [*Container*](https://en.cppreference.com/w/cpp/named_req/Container), [*AllocatorAwareContainer*](https://en.cppreference.com/w/cpp/named_req/AllocatorAwareContainer), [*ReversibleContainer*](https://en.cppreference.com/w/cpp/named_req/ReversibleContainer), [*ContiguousContainer*](https://en.cppreference.com/w/cpp/named_req/ContiguousContainer) and [*SequenceContainer*](https://en.cppreference.com/w/cpp/named_req/SequenceContainer).

<br><br>



## Template Parameters

1.  ```
    typename T
    ```

    The type of the elements.

2.  ```
    typename Allocator
    ```

    Allocator used for memory allocation/deallocation and construction/destruction of elements.

    This type must meet the requirements of [*Allocator*](https://en.cppreference.com/w/cpp/named_req/Allocator).

    The program is ill-formed if `Allocator::value_type` is not the same as `T`.

<br><br>



## Public Member Types

| Member Type               | Definition |
| :------------------------ | :--------- |
| `allocator_type`          | `Allocator` |
| `allocator_traits`        | `std::allocator_traits<Allocator>` |
| `value_type`              | `T` |
| `size_type`               | `typename allocator_traits::size_type` |
| `difference_type`         | `typename allocator_traits::difference_type` |
| `reference`               | `T&` |
| `const_reference`         | `const T&` |
| `pointer`                 | `typename allocator_traits::pointer` |
| `const_pointer`           | `typename allocator_traits::const_pointer` |
| `iterator`                | [*LegacyRandomAccessIterator*](https://en.cppreference.com/w/cpp/named_req/RandomAccessIterator) and [*LegacyContiguousIterator*](https://en.cppreference.com/w/cpp/named_req/ContiguousIterator) to `value_type` |
| `const_iterator`          | [*LegacyRandomAccessIterator*](https://en.cppreference.com/w/cpp/named_req/RandomAccessIterator) and [*LegacyContiguousIterator*](https://en.cppreference.com/w/cpp/named_req/ContiguousIterator) to `const value_type` |
| `reverse_iterator`        | `std::reverse_iterator<iterator>` |
| `const_reverse_iterator`  | `std::reverse_iterator<const_iterator>` |

<br><br>



## Public Member Functions

### (constructor)

1.  ```
    devector() noexcept;
    ```
2.  ```
    explicit devector(const Allocator& alloc)
        noexcept(std::is_nothrow_copy_constructible<Allocator>::value);
    ```

    **Effects:**
    Constructs an empty container.

    **Complexity:**
    Constant.

    <br><br>



3.  ```
    devector(size_type n);
    ```
4.  ```
    explicit devector(size_type n, const Allocator& alloc);
    ```

    **Effects:**
    Constructs the container with `n` default-constructed elements.

    **Complexity:**
    Linear in `n`.

    <br><br>



5.  ```
    devector(size_type n, const T& value);
    ```
6.  ```
    devector(size_type n, const T& value, const Allocator& alloc);
    ```

    **Effects:**
    Constructs the container with `n` copies of elements with value `value`.

    **Complexity:**
    Linear in `n`.

    <br><br>



7.  ```
    template <typename InputIt>
    devector(InputIt first, InputIt last);
    ```
8.  ```
    template <typename InputIt>
    devector(InputIt first, InputIt last, const Allocator& alloc);
    ```

    **Effects:**
    Constructs the container with the contents of the range `[first, last)`.

    **Note:**
    This overload participates in overload resolution only if `InputIt` satisfies requirements of [*LegacyInputIterator*](https://en.cppreference.com/w/cpp/named_req/InputIterator).

    **Complexity:**
    Linear in `std::distance(first, last)`.

    <br><br>



9.  ```
    devector(std::initializer_list<T> ilist);
    ```
10. ```
    devector(std::initializer_list<T> ilist, const Allocator& alloc);
    ```

    **Effects:**
    Constructs the container with the contents of the initializer list `ilist`.

    **Complexity:**
    Linear in `ilist.size()`.

    <br><br>



11. ```
    devector(const devector& other);
    ```
12. ```
    devector(const devector& other, const Allocator& alloc);
    ```

    **Effects:**
    Copy constructor.
    Constructs the container with the copy of the contents of `other`.

    **Complexity:**
    Linear in `other.size()`.

    <br><br>



13. ```
    devector(devector&& other);
    ```
14. ```
    devector(devector&& other, const Allocator& alloc);
    ```

    **Effects:**
    Move constructor.
    Constructs the container with the contents of `other` using move semantics.

    `other` is not guaranteed to be empty after the move.

    `other` is in a valid but unspecified state after the move.

    **Complexity:**
    Constant in the best case. Linear in size in the worst case.

    <br><br>



15. ```
    template <typename Range>
    devector(sfl::from_range_t, Range&& range);
    ```
16. ```
    template <typename Range>
    devector(sfl::from_range_t, Range&& range, const Allocator& alloc);
    ```

    **Effects:**
    Constructs the container with the contents of `range`.

    **Note:**
    It is available in C++11. In C++20 are used proper C++20 range concepts.

    <br><br>



### (destructor)

1.  ```
    ~devector();
    ```

    **Effects:**
    Destructs the container. The destructors of the elements are called and the used storage is deallocated.

    **Complexity:**
    Linear in `size()`.

    <br><br>



### assign

1.  ```
    void assign(size_type n, const T& value);
    ```

    **Effects:**
    Replaces the contents of the container with `n` copies of value `value`.

    **Complexity:**
    Linear in `n`.

    <br><br>



2.  ```
    template <typename InputIt>
    void assign(InputIt first, InputIt last);
    ```

    **Effects:**
    Replaces the contents of the container with the contents of the range `[first, last)`.

    **Note:**
    This overload participates in overload resolution only if `InputIt` satisfies requirements of [*LegacyInputIterator*](https://en.cppreference.com/w/cpp/named_req/InputIterator).

    **Note:**
    The behavior is undefined if either `first` or `last` is an iterator into `*this`.

    **Complexity:**
    Linear in `std::distance(first, last)`.

    <br><br>



3.  ```
    void assign(std::initializer_list<T> ilist);
    ```

    **Effects:**
    Replaces the contents of the container with the contents of the initializer list `ilist`.

    **Complexity:**
    Linear in `ilist.size()`.

    <br><br>



### assign_range

1.  ```
    template <typename Range>
    void assign_range(Range&& range);
    ```

    **Effects:**
    Replaces the contents of the container with the contents of `range`.

    **Note:**
    It is available in C++11. In C++20 are used proper C++20 range concepts.

    <br><br>



### operator=

1.  ```
    devector& operator=(const devector& other);
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
    devector& operator=(devector&& other);
    ```

    **Effects:**
    Move assignment operator.
    Replaces the contents with those of `other` using move semantics.

    `other` is not guaranteed to be empty after the move.

    `other` is in a valid but unspecified state after the move.

    **Returns:**
    `*this()`.

    **Complexity:**
    Constant in the best case. Linear in size in the worst case.

    <br><br>



3.  ```
    devector& operator=(std::initializer_list<T> ilist);
    ```

    **Effects:**
    Replaces the contents with those identified by initializer list `ilist`.

    **Returns:**
    `*this()`.

    **Complexity:**
    Linear in size.

    <br><br>



### get_allocator

1.  ```
    allocator_type get_allocator() const noexcept;
    ```

    **Effects:**
    Returns the allocator associated with the container.

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
    size_type max_size() const noexcept;
    ```

    **Effects:**
    Returns the maximum number of elements the container is able to hold, i.e. `std::distance(begin(), end())` for the largest container.

    **Complexity:**
    Constant.

    <br><br>



### capacity

1.  ```
    size_type capacity() const noexcept;
    ```

    **Effects:**
    Returns the number of elements that the container has currently allocated space for.

    **Complexity:**
    Constant.

    <br><br>



### available_front

1.  ```
    size_type available() const noexcept;
    ```

    **Effects:**
    Returns the number of elements that can be pushed to the front without requiring allocation of additional memory.

    **Complexity:**
    Constant.

    <br><br>



### available_back

1.  ```
    size_type available() const noexcept;
    ```

    **Effects:**
    Returns the number of elements that can be pushed to the back without requiring allocation of additional memory.

    **Complexity:**
    Constant.

    <br><br>



### reserve_front

1.  ```
    void reserve_front(size_type new_capacity);
    ```

    **Effects:**
    Ensures that `n` elements can be pushed to the front without requiring allocation of additional memory, where `n` is `new_capacity - size()`. Otherwise, there are no effects.

    This function does not change size of the container.

    If the capacity is changed, all iterators to the elements are invalidated, but references and pointers to elements remain valid. Otherwise, no iterators or references are invalidated.

    **Complexity:**
    Linear.

    **Exceptions:**

    * `Allocator::allocate` may throw.
    * `T`'s move or copy constructor may throw.

    If an exception is thrown:

    * If type `T` has available `noexcept` move constructor:
        * This function has no effects (strong exception guarantee).
    * Else if type `T` has available copy constructor:
        * This function has no effects (strong exception guarantee).
    * Else if type `T` has available throwing move constructor:
        * Container is changed but in valid state (basic exception guarantee).

    <br><br>



### reserve_back

1.  ```
    void reserve_back(size_type new_capacity);
    ```

    **Effects:**
    Ensures that `n` elements can be pushed to the back without requiring allocation of additional memory, where `n` is `new_capacity - size()`. Otherwise, there are no effects.

    This function does not change size of the container.

    If the capacity is changed, all iterators to the elements are invalidated, but references and pointers to elements remain valid. Otherwise, no iterators or references are invalidated.

    **Complexity:**
    Linear.

    **Exceptions:**

    * `Allocator::allocate` may throw.
    * `T`'s move or copy constructor may throw.

    If an exception is thrown:

    * If type `T` has available `noexcept` move constructor:
        * This function has no effects (strong exception guarantee).
    * Else if type `T` has available copy constructor:
        * This function has no effects (strong exception guarantee).
    * Else if type `T` has available throwing move constructor:
        * Container is changed but in valid state (basic exception guarantee).

    <br><br>



### shrink_to_fit

1.  ```
    void shrink_to_fit();
    ```

    **Effects:**
    If `size() < capacity()`, the function allocates memory for new storage of capacity equal to the value of `size()`, moves elements from old storage to new storage, and deallocates memory used by old storage. Otherwise, the function does nothing.

    This function does not change size of the container.

    If the capacity is changed, all iterators and all references to the elements are invalidated. Otherwise, no iterators or references are invalidated.

    **Complexity:**
    Linear.

    **Exceptions:**

    * `Allocator::allocate` may throw.
    * `T`'s move or copy constructor may throw.

    If an exception is thrown:

    * If type `T` has available `noexcept` move constructor:
        * This function has no effects (strong exception guarantee).
    * Else if type `T` has available copy constructor:
        * This function has no effects (strong exception guarantee).
    * Else if type `T` has available throwing move constructor:
        * Container is changed but in valid state (basic exception guarantee).

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
    After this call, `size()` returns zero and `capacity()` remains unchanged.

    **Complexity:**
    Linear in `size()`.

    <br><br>



### emplace

1.  ```
    template <typename... Args>
    iterator emplace(const_iterator pos, Args&&... args);
    ```

    **Preconditions:**
    `cbegin() <= pos && pos <= cend()`

    **Effects:**
    Inserts a new element into the container at position `pos`.

    New element is constructed as `value_type(std::forward<Args>(args)...)`.

    `args...` may directly or indirectly refer to a value in the container.

    **Returns:**
    Iterator to the inserted element.

    **Complexity:**
    Constant plus linear in `std::min(std::distance(begin(), pos), std::distance(pos, end()))`.

    <br><br>



### insert

1.  ```
    iterator insert(const_iterator pos, const T& value);
    ```

    **Preconditions:**
    `cbegin() <= pos && pos <= cend()`

    **Effects:**
    Inserts copy of `value` at position `pos`.

    **Returns:**
    Iterator to the inserted element.

    **Complexity:**
    Constant plus linear in `std::min(std::distance(begin(), pos), std::distance(pos, end()))`.

    <br><br>



2.  ```
    iterator insert(const_iterator pos, T&& value);
    ```

    **Preconditions:**
    `cbegin() <= pos && pos <= cend()`

    **Effects:**
    Inserts `value` using move semantics at position `pos`.

    **Returns:**
    Iterator to the inserted element.

    **Complexity:**
    Constant plus linear in `std::min(std::distance(begin(), pos), std::distance(pos, end()))`.

    <br><br>



3.  ```
    iterator insert(const_iterator pos, size_type n, const T& value);
    ```

    **Preconditions:**
    `cbegin() <= pos && pos <= cend()`

    **Effects:**
    Inserts `n` copies of `value` before position `pos`.

    **Returns:**
    Iterator to the first element inserted, or `pos` if `n == 0`.

    **Complexity:**
    Constant plus linear in `std::min(std::distance(begin(), pos), std::distance(pos, end()))`.

    <br><br>



4.  ```
    template <typename InputIt>
    iterator insert(const_iterator pos, InputIt first, InputIt last);
    ```

    **Preconditions:**
    `cbegin() <= pos && pos <= cend()`

    **Effects:**
    Inserts elements from the range `[first, last)` before position `pos`.

    **Note:**
    This overload participates in overload resolution only if `InputIt` satisfies requirements of [*LegacyInputIterator*](https://en.cppreference.com/w/cpp/named_req/InputIterator).

    **Note:**
    The behavior is undefined if either `first` or `last` is an iterator into `*this`.

    **Returns:**
    Iterator to the first element inserted, or `pos` if `first == last`.

    **Complexity:**
    Linear in `std::distance(first, last)` plus linear in `std::min(std::distance(begin(), pos), std::distance(pos, end()))`.

    <br><br>



5.  ```
    iterator insert(const_iterator pos, std::initializer_list<T> ilist);
    ```

    **Preconditions:**
    `cbegin() <= pos && pos <= cend()`

    **Effects:**
    Inserts elements from initializer list `ilist` before position `pos`.

    **Returns:**
    Iterator to the first element inserted, or `pos` if `ilist` is empty.

    **Complexity:**
    Linear in `ilist.size()` plus linear in `std::min(std::distance(begin(), pos), std::distance(pos, end()))`.

    <br><br>



### insert_range

1.  ```
    template <typename Range>
    iterator insert_range(const_iterator pos, Range&& range);
    ```

    **Effects:**
    Inserts elements from `range` before position `pos`. Elements are inserted in non-reversing order.

    `range` must not overlap with the container. Otherwise, the behavior is undefined.

    **Returns:**
    Iterator to the first element inserted, or `pos` if `range` is empty.

    **Note:**
    It is available in C++11. In C++20 are used proper C++20 range concepts.

    <br><br>



### emplace_front

1.  ```
    template <typename... Args>
    reference emplace_front(Args&&... args);
    ```

    **Effects:**
    Inserts a new element at the beginning of container.

    New element is constructed as `value_type(std::forward<Args>(args)...)`.

    `args...` may directly or indirectly refer to a value in the container.

    **Returns:**
    Reference to the inserted element.

    **Complexity:**
    Constant.

    <br><br>



### emplace_back

1.  ```
    template <typename... Args>
    reference emplace_back(Args&&... args);
    ```

    **Effects:**
    Inserts a new element at the end of container.

    New element is constructed as `value_type(std::forward<Args>(args)...)`.

    `args...` may directly or indirectly refer to a value in the container.

    **Returns:**
    Reference to the inserted element.

    **Complexity:**
    Constant.

    <br><br>



### push_front

1.  ```
    void push_front(const T& value);
    ```

    **Effects:**
    Inserts copy of `value` at the beginning of container.

    **Complexity:**
    Constant.

    <br><br>



2.  ```
    void push_front(T&& value);
    ```

    **Effects:**
    Inserts `value` using move semantics at the beginning of container.

    **Complexity:**
    Constant.

    <br><br>



### push_back

1.  ```
    void push_back(const T& value);
    ```

    **Effects:**
    Inserts copy of `value` at the end of container.

    **Complexity:**
    Constant.

    <br><br>



2.  ```
    void push_back(T&& value);
    ```

    **Effects:**
    Inserts `value` using move semantics at the end of container.

    **Complexity:**
    Constant.

    <br><br>



### prepend_range

1.  ```
    template <typename Range>
    void prepend_range(Range&& range);
    ```

    **Effects:**
    Inserts elements from `range` before `begin()`. Elements are inserted in non-reversing order.

    **Note:**
    It is available in C++11. In C++20 are used proper C++20 range concepts.

    <br><br>



### append_range

1.  ```
    template <typename Range>
    void append_range(Range&& range);
    ```

    **Effects:**
    Inserts elements from `range` before `end()`. Elements are inserted in non-reversing order.

    **Note:**
    It is available in C++11. In C++20 are used proper C++20 range concepts.

    <br><br>



### pop_front

1.  ```
    void pop_front();
    ```

    **Preconditions:**
    `!empty()`

    **Effects:**
    Removes the first element of the container.

    **Complexity:**
    Constant.

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

    **Effects:**
    Resizes the container to contain `n` elements.

    1. If the `size() > n`, the last `size() - n` elements are removed.
    2. If the `size() < n`, additional default-constructed elements are inserted into container. Additional elements could be inserted both at the beginning and at the end of container.

    <br><br>



2.  ```
    void resize(size_type n, const T& value);
    ```

    **Effects:**
    Resizes the container to contain `n` elements.

    1. If the `size() > n`, the last `size() - n` elements are removed.
    2. If the `size() < n`, additional copies of `value` are inserted into container. Additional elements could be inserted both at the beginning and at the end of container.

    <br><br>



### resize_front

1.  ```
    void resize_front(size_type n);
    ```

    **Effects:**
    Resizes the container to contain `n` elements.

    1. If the `size() > n`, the first `size() - n` elements are removed.
    2. If the `size() < n`, additional default-constructed elements are inserted at the beginning of container.

    <br><br>



2.  ```
    void resize_front(size_type n, const T& value);
    ```

    **Effects:**
    Resizes the container to contain `n` elements.

    1. If the `size() > n`, the first `size() - n` elements are removed.
    2. If the `size() < n`, additional copies of `value` are inserted at the beginning of container.

    <br><br>



### resize_back

1.  ```
    void resize_back(size_type n);
    ```

    **Effects:**
    Resizes the container to contain `n` elements.

    1. If the `size() > n`, the last `size() - n` elements are removed.
    2. If the `size() < n`, additional default-constructed elements are inserted at the end of container.

    <br><br>



2.  ```
    void resize_back(size_type n, const T& value);
    ```

    **Effects:**
    Resizes the container to contain `n` elements.

    1. If the `size() > n`, the last `size() - n` elements are removed.
    2. If the `size() < n`, additional copies of `value` are inserted at the end of container.

    <br><br>



### swap

1.  ```
    void swap(devector& other);
    ```

    **Preconditions:**
    `allocator_traits::propagate_on_container_swap::value || get_allocator() == other.get_allocator()`

    **Effects:**
    Exchanges the contents of the container with those of `other`.

    **Complexity:**
    Constant.

    <br><br>



## Non-member Functions

### operator==

1.  ```
    template <typename T, typename A>
    bool operator==
    (
        const devector<T, A>& x,
        const devector<T, A>& y
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
    template <typename T, typename A>
    bool operator!=
    (
        const devector<T, A>& x,
        const devector<T, A>& y
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
    template <typename T, typename A>
    bool operator<
    (
        const devector<T, A>& x,
        const devector<T, A>& y
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
    template <typename T, typename A>
    bool operator>
    (
        const devector<T, A>& x,
        const devector<T, A>& y
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
    template <typename T, typename A>
    bool operator<=
    (
        const devector<T, A>& x,
        const devector<T, A>& y
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
    template <typename T, typename A>
    bool operator>=
    (
        const devector<T, A>& x,
        const devector<T, A>& y
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
    template <typename T, typename A>
    void swap
    (
        devector<T, A>& x,
        devector<T, A>& y
    );
    ```

    **Effects:**
    Swaps the contents of `x` and `y`. Calls `x.swap(y)`.

    <br><br>



### erase

1.  ```
    template <typename T, typename A, typename U>
    typename devector<T, A>::size_type
        erase(devector<T, A>& c, const U& value);
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
    template <typename T, typename A, typename Predicate>
    typename devector<T, A>::size_type
        erase_if(devector<T, A>& c, Predicate pred);
    ```

    **Effects:**
    Erases all elements that satisfy the predicate `pred` from the container.

    `pred` is unary predicate which returns `true` if the element should be removed.

    **Returns:**
    The number of erased elements.

    **Complexity:**
    Linear.

    <br><br>



## Deduction Guides (since C++17)

1.  ```
    template < typename InputIt,
               typename Allocator = std::allocator<
                   typename std::iterator_traits<InputIt>::value_type> >
    devector(InputIt, InputIt, Allocator = Allocator())
        -> devector<typename std::iterator_traits<InputIt>::value_type, Allocator>;
    ```

    **Effects:**
    Deduction from an iterator range.

    **Note:**
    This overload participates in overload resolution only if `InputIt` satisfies requirements of [*LegacyInputIterator*](https://en.cppreference.com/w/cpp/named_req/InputIterator).

    <br><br>



End of document.
