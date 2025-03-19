# sfl library

This is header-only C++11 library that offers several new or less-known containers:

* Sequence containers:

  * [`vector`](doc/vector.md) — Equivalent to `std::vector` with few minor differences.
  * [`devector`](doc/devector.md) — Double-ended vector that allows faster insertion/deletion at front than `vector`.
  * [`small_vector`](doc/small_vector.md) — Vector that internally embeds small statically allocated storage to avoid using dynamic memory management if number of elements is small.
  * [`static_vector`](doc/static_vector.md) — Vector with fixed-size capacity specified at compile-time. Capacity cannot be changed at run-time. Dynamic memory management is never used.
  * [`compact_vector`](doc/compact_vector.md) — Vector that always have `capacity()` equal to `size()`. It is inspired by [OpenFOAM's](https://openfoam.org/) container [`List`](https://github.com/OpenFOAM/OpenFOAM-dev/blob/master/src/OpenFOAM/containers/Lists/List/List.H).
  * [`segmented_vector`](doc/segmented_vector.md) — Vector with segmented storage that allows fast insertion/deletion at back.
  * [`segmented_devector`](doc/segmented_devector.md) — Double-ended vector with segmented storage that allows fast insertion/deletion at both front and back.

* Associative containers implemented as **red-black tree**:

  * [`map`](doc/map.md)
  * [`set`](doc/set.md)
  * [`multimap`](doc/multimap.md)
  * [`multiset`](doc/multiset.md) <br><br>
  * [`small_map`](doc/small_map.md)
  * [`small_set`](doc/small_set.md)
  * [`small_multimap`](doc/small_multimap.md)
  * [`small_multiset`](doc/small_multiset.md) <br><br>
  * [`static_map`](doc/static_map.md)
  * [`static_set`](doc/static_set.md)
  * [`static_multimap`](doc/static_multimap.md)
  * [`static_multiset`](doc/static_multiset.md)

* Associative containers implemented as **sorted vector**:

  * [`small_flat_map`](doc/small_flat_map.md)
  * [`small_flat_set`](doc/small_flat_set.md)
  * [`small_flat_multimap`](doc/small_flat_multimap.md)
  * [`small_flat_multiset`](doc/small_flat_multiset.md) <br><br>
  * [`static_flat_map`](doc/static_flat_map.md)
  * [`static_flat_set`](doc/static_flat_set.md)
  * [`static_flat_multimap`](doc/static_flat_multimap.md)
  * [`static_flat_multiset`](doc/static_flat_multiset.md)

* Unordered associative containers implemented as **unsorted vector**:

  * [`small_unordered_flat_map`](doc/small_unordered_flat_map.md)
  * [`small_unordered_flat_set`](doc/small_unordered_flat_set.md)
  * [`small_unordered_flat_multimap`](doc/small_unordered_flat_multimap.md)
  * [`small_unordered_flat_multiset`](doc/small_unordered_flat_multiset.md) <br><br>
  * [`static_unordered_flat_map`](doc/static_unordered_flat_map.md)
  * [`static_unordered_flat_set`](doc/static_unordered_flat_set.md)
  * [`static_unordered_flat_multimap`](doc/static_unordered_flat_multimap.md)
  * [`static_unordered_flat_multiset`](doc/static_unordered_flat_multiset.md)



# Features

* Compiles with GCC 4.8.5 and Clang 3.4.2.
* Containers support [stateful allocators](https://en.cppreference.com/w/cpp/named_req/Allocator#Stateful_and_stateless_allocators) and allocators with [fancy pointers](https://en.cppreference.com/w/cpp/named_req/Allocator#Fancy_pointers).
* Containers have available range constructor `container(sfl::from_range_t, Range&& r)` in C++11.
* Containers have available range insertion member function `insert_range(Range&& r)` in C++11.
* Maps and sets support heterogeneous insertion, erasure and lookup in C++11.
* Functions taking iterator range properly handle [input iterators](https://en.cppreference.com/w/cpp/named_req/InputIterator).
* There is no undefined behavior when constructing maps and sets from range containing duplicates.
* There is no undefined behavior when inserting range containing duplicates into maps and sets.
* Vectors are not specialized for `bool`.
* Static containers can be used for bare-metal embedded software development.



# Requirements

This library requires C++11 compiler or newer.
If available, library uses features of newer C++ standards.

Tested compilers:
* GCC 4.8.5 on CentOS 7 (C++11)
* Clang 3.4.2 on CentOS 7 (C++11)
* GCC 7.3.1 on CentOS 7 (C++11, 14, 17)
* Clang 5.0.1 on CentOS 7 (C++11, 14, 17)
* GCC 14.2.1 on Arch Linux (C++11, 14, 17, 20, 23)
* Clang 18.1.8 on Arch Linux (C++11, 14, 17, 20, 23)
* MSVC 19.38 (C++14, 17, 20, latest)



# Installation and usage

## Method 1: Manual copy-paste

**Step 1:** Copy `sfl` subdirectory from `include` into your project directory.

**Step 2:** Configure your project or compiler to search include files in `sfl` subdirectory.

**Step 3:** `#include` what you need.

## Method 2: CMake integration

This library can be integrated into CMake project using CMake module [FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html).

**Step 1:** Add the following lines into your `CMakeLists.txt`:

```
include(FetchContent)

FetchContent_Declare(
    sfl
    GIT_REPOSITORY https://github.com/slavenf/sfl-library)

FetchContent_MakeAvailable(sfl)
```

**Step 2:** Add this library as a dependency into your `CMakeLists.txt`::

```
target_link_libraries(your_target_name PRIVATE sfl)
```

**Step 3:** `#include` what you need.



# Exceptions

This library by default throw exceptions in case of error.

If macro `SFL_NO_EXCEPTIONS` is defined then library avoids using exceptions and calls `std::abort` in case of error.

This library does not automatically detect whether the compiler is invoked with appropriate flag or not, like `-fno-exceptions` in GCC and Clang.



# Debugging

This library extensively uses macro `assert` from header `<cassert>`.

The definition of the macro `assert` depends on another macro, `NDEBUG`, which is not defined by the standard library.

If macro `NDEBUG` is defined then `assert` does nothing.

If macro `NDEBUG` is not defined then `assert` performs check. If check fails, `assert` outputs implementation-specific diagnostic information on the standard error output and calls `std::abort`.



# Detailed documentation

The detailed documentation is located in directory `doc`. The detailed documentation is handwritten in Markdown format, it is not automatically generated with tools like Doxygen, so there may be some errors or mistakes. If you find one, please report it.



# Tests

Test programs and scripts are located in directory `test`.

Tested compilers are listed in section [Requirements](#requirements).

Each test program is checked with Valgrind tool.



# License

Licensed under zlib license. The license text is in file [`LICENSE.txt`](LICENSE.txt).
