# [5.2.4](https://github.com/fraillt/bitsery/compare/v5.2.3...v5.2.4) (2024-07-30)

### Improvements
* implement brief syntax for std::optional and std::bitset. #116 (thanks to [Destroyerrrocket](https://github.com/Destroyerrrocket))
* improve performance for buffer adapters. #118 (thanks to [Destroyerrrocket](https://github.com/Destroyerrrocket))
* check if should swap by taking into account actual type (in addition to configuration). #105 (thanks to [SoftdriveFelix](https://github.com/SoftdriveFelix))
* fix compile errors for latest compilers. #106 (thanks to [NBurley93](https://github.com/NBurley93))

### Other notes
* change cmake_minimum_required to 3.25.
* change compilers for ubuntu (gcc 14 and clang 18).

# [5.2.3](https://github.com/fraillt/bitsery/compare/v5.2.2...v5.2.3) (2022-12-01)

### Improvements
* refactored bit-packing implementation to make sure that `MeasureSize` adapter acts as all other adapters and is not bit-packing enabled by default. #91
* `BufferAdapterTraits::increaseBufferSize` now accepts current size and minimum required size in order to better allocate required memory.

### Bug fixes
* fix shadow warning in gcc8 #87 (thanks to [museghost](https://github.com/museghost) and brian.kim)

### Other notes
* format whole code base in Mozilla style.
* simplified adapters implementations

# [5.2.2](https://github.com/fraillt/bitsery/compare/v5.2.1...v5.2.2) (2021-08-31)

### Improvements
* add 16 byte value support #75 (thanks to [Victor Stewart](https://github.com/victorstewart))
* avoid reinitializing nontrivial std::variant #77 (thanks to [Robbert van der Helm](https://github.com/robbert-vdh))
* avoid reinitializing nontrivial std::optional.

### Bug fixes
* fix missing headers for GCC11, also added test to check includes #82 (thanks to [michael-mueller-git](https://github.com/michael-mueller-git))
* fixed **StdBitset** to build on macOS (proxy type, returned by `[]` operator wasn't correctly converted to unsigned integral type).

### Other notes
* migrated to [Github actions](https://docs.github.com/en/actions) for running tests.
* fixes few warnings on MSVC compiler.
* now tests are also run on macOS and Windows.

# [5.2.1](https://github.com/fraillt/bitsery/compare/v5.2.0...v5.2.1) (2020-11-14)

### Improvements
* `Input/OutputBufferAdapter` now statically asserts that underlying type is 1byte in size.

### Bug fixes
* fixed serialization in `StdBitset` when it's size is less then `unsigned long long`.

# [5.2.0](https://github.com/fraillt/bitsery/compare/v5.1.0...v5.2.0) (2020-11-09)

### Features

* new extension **StdBitset**.

### Improvements
* removed unused variable warnings in release build, where `max_size` variable during serialization is ignored.
* removed unknown pragmas warnings for GCC/Clang (thanks to [Mmpuskas](https://github.com/Mmpuskas)).

# [5.1.0](https://github.com/fraillt/bitsery/compare/v5.0.3...v5.1.0) (2020-06-08)

### Features

* new extension **StdAtomic** (thanks for [VelocityRa](https://github.com/VelocityRa)).

### Improvements
* [examples](examples) no longer include bitsery in global namespace (removed `using namespace bitsery`).
* removed multiple warning regarding integer conversions (thanks to [tower120](https://github.com/tower120)).
* removed unnecessary `dynamic_cast` from BasicInput/OutputStreamAdapter (thanks to [tower120](https://github.com/tower120)).
* fixed some include paths, now you can basically to copy/paste bitsery include directory to your project without cmake support.

### Other notes
* added tutorial of how to write your own extension ([here](doc/tutorial/first_extension.md)).
* now gtest 1.10 is required if you want to build tests.

# [5.0.3](https://github.com/fraillt/bitsery/compare/v5.0.2...v5.0.3) (2020-01-29)

### Improvements
* rewritten buffer adapters (and `BasicBufferedOutputStreamAdapter`) to fix UB when incrementing past the end iterator, and added an additional read/write method that accepts a number of bytes to be read/written at compile time.
This provides additional optimization opportunities.

# [5.0.2](https://github.com/fraillt/bitsery/compare/v5.0.1...v5.0.2) (2020-01-17)

### Bug fixes
* fixed a bug when deserializing non-default constructible containers (thanks to [nicktrandafil](https://github.com/nicktrandafil)).
* fixed issue with a brace initialization in extension StdMap and StdSet. It was working on major compilers, but it wasn't C++11 compatible.
More info about it in [stackoverflow](https://stackoverflow.com/questions/25612262/why-does-auto-x3-deduce-an-initializer-list) (thanks to [BotellaA](https://github.com/BotellaA)).  

### Other notes
* added [patches/centos7_gcc4.8.2.diff](patches/centos7_gcc4.8.2.diff) that allows to use bitsery with gcc4.8.2 on Centos7 (thanks to [BotellaA](https://github.com/BotellaA)).
More information on patches is [here](patches/README.md).
* added documentation on how [extensions](doc/design/extensions.md) work.

# [5.0.1](https://github.com/fraillt/bitsery/compare/v5.0.0...v5.0.1) (2019-08-21)

### Bug fixes
* fixed polymorphic handler deleter in `PolymorphicContext`, now it is captured by value and doesn't depend on lifetime of `PolymorphicContext` instance.
* fixed compilation errors in `ext/utils/pointer_utils.h` on macOS.
* reduced warnings on Visual Studio (thanks to [BotellaA](https://github.com/BotellaA))

# [5.0.0](https://github.com/fraillt/bitsery/compare/v4.6.1...v5.0.0) (2019-07-09)

This version reduces library complexity by removing redundant features and changing existing ones.
Additionally, it provides more customization options for serialization/deserialization flow.

### Features

* added config options to enable/disable checking input adapter and data errors: **CheckAdapterErrors** and **CheckDataErrors**.
If you can trust your data this will improve deserialization performance. Error checks are enabled by default.
* all memory allocation in the library can be customized using memory resource (similar to c++ 17 memory resource)
  * contexts that internally requires to allocate memory are: *PointerLinkingContext, PolymorphicContext, and InheritanceContext*.
  * extensions, for pointers like types, that allocate memory are: *PointerOwner* and *StdSharedPtr*.
  More information about pointer extension can be found [here](doc/design/pointers.md).

### Breaking changes

* improved design for serializer/deserializer *context*. It was hard to understand and easy to misuse, so several changes were made.
  * removed *internal* context from config, because it doesn't actually solve any problems, only allows doing the same thing in multiple ways.
  * removed `T* context()`. This allowed to get a context that is `std::tuple<...>`, but you can do the same with other methods, by wrapping in an outer tuple, e.g. `std::tuple<std::tuple<...>>`.
  * if a context is defined, in serializer/deserializer, it is passed (and stored) by reference as a first argument (instead of pointer). Other parameters are forwarded to a input/output adapter.
  * changed signature `T* context<T>` to `T& context<T>`, this will either return context or doesn't compile, previously it could also return nullptr.
  * `context<T>` and `contextOrNull<T>` now also check if a type is convertible, so it can work with base classes
    (e.g. you can require a base class of context in extension, but provided child implementation instead). This allows to further customise extension by providing different context implementations.
* reworked *BufferedSessions*. It was the only feature that was in a bitsery core and required allocations.
It was removed and instead was added ability to change read/write position directly for buffered adapters.
This is also a more flexible solution, and can be used to implement solutions that allow deserialization [in-place](https://github.com/fraillt/bitsery/issues/19) like *flatbuffers* does.
* changed the signature to all serialization functions that accept lambdas, instead of accepting `(T& )`, now accept `(S& ,T& )`.
This allows much easier serialization customization, because no additional state is required in a functor, and these methods can now accept function pointers or stateless lambdas.
* removed various functions/classes that became redundant:
  * *AdapterWriter/Reader* classes - their functionality is moved to *adapters*.
  * *AdapterAccess* class - now serializer/deserializer expose adapter directly via `adapter()` method.
  Additionally adapter can be moved out if serializer/deserializer is rvalue .e.g `auto adapter = std::move(ser).adapter();`.
  * removed *Writer/Reader* parameters from extensions *serialize/deserialize* methods - because serializer/deserializer now expose *adapter* directly.
  * *archive* from serializer/deserializer - because `operator()` do the same thing, but it is more terse, and is compatible with `cereal` library.
  * *align* function from serializer/deserializer - it can now be called directly on input/output adapter.
  * *UnsafeInputBufferAdapter* - instead config option is provided to disable adapter read errors (*CheckAdapterErrors*).
  * *isValidState* from stream output adapter - it didn't provide any additional information that couldn't be queried directly on stream object.
  * *registerBasesList* from *PolymorphicContext* - it was previously deprecated.
* renamed various classes/functions and files:
  * *flexible* to **brief_syntax** - this is a big change and might affect a lot of code, but it was necessary, because this name was contradicting to what it actually was trying to achieve.
  Migration should not be very hard, just global replace `flexible` to `brief_syntax` and it should work:).
  * *BasicSerializer/BasicDeserializer* to **Serializer/Deserializer** - previously they were aliases, but after removing adapter writer/reader,
  serializer/deserializer signature has changed to `Serializer/Deserializer<Adapter, Context=void>` and no longer need any alias.
  * *setError* to **error** for input adapters.
  * *NetworkEndianness* to **Endianness** in config.
  * *MeasureSize* adapter moved to separate file `/adapter/measure_size.h`

### Improvements

* added `quickSerialization/Deserialization` overloads that can accept context as first parameter.
* added convenience classes (functors) **FtorExtValue, FtorExtObject**, in places where you need to provide (des)serialize function/lambda that uses extension.
e.g. instead of writing `s.container(obj, [](S& s, MyData& data) {s.ext(data, MyExtension{});});` you can write `s.container(obj, FtorExtObject<MyExtension>{});`
* added more tests for adapters.
* input adapters now save first error that occured, this allows better diagnostic for what actually happened.

### Bug fixes
* fixed `enabledBitPacking` in serializer/deserializer where writer/reader and internal context states were not restored properly after bit packing is complete.

# [4.6.1](https://github.com/fraillt/bitsery/compare/v4.6.0...v4.6.1) (2019-06-27)

### Features
* flexible syntax now also supports `cereal` like serialization interface (thanks to [Luli2020](https://github.com/Luli2020))

### Bug fixes
* when using `enableBitPacking`, internal context (i.e. context<T>()) in serializer/deserializer was not copied to/from new bitpacking enabled instance.

# [4.6.0](https://github.com/fraillt/bitsery/compare/v4.5.1...v4.6.0) (2019-03-12)

### Features
* new extensions **StdTuple** and **StdVariant** for `std::tuple` and `std::variant`. These are the first extensions that requires C++17, or higher, standard enabled.
Although `std::tuple` is C++11 type, but from usage perspective it has exactly the same requirements as `std::variant` and relies heavily on having class template argument deduction guides to make it convenient to use.
You can easily use `std::tuple` without any extension at all, so the main motivation was to create convenient interface for **StdVariant** and use the same interface for **StdTuple** as well.
  * instead of providing custom lambda to overload each type in tuple or variant, there was added several helper callable objects.
  **OverloadValue** wrapper around `s.value<N>(o)`, **OverloadExtValue** wrapper around `s.ext<N>(o, Ext{})` and **OverloadExtObject** wrapper around `s.ext(o, Ext{})`.
* new extensions **StdDuration** and **StdTimePoint** for `std::chrono::duration` and `std::chrono::time_point`.

### Improvements
tests now uses `gtest_discover_tests` function, to automatically discover tests, which requires CMake 3.10.

# [4.5.1](https://github.com/fraillt/bitsery/compare/v4.5.0...v4.5.1) (2019-01-16)

### Improvements
* template specializations, where possible, was changed to avoid using variadics, some Visual Studio compilers has [issues](https://developercommunity.visualstudio.com/content/problem/3437/error-with-c11-variadics.html) with variadic templates.
* reduced compile warnings for VisualStudio:
  * added explicit casts
  * renamed `struct` to `class` where class is used as friend. e.g. `friend class bitsery::Access`, because it is more conventional usage.

# [4.5.0](https://github.com/fraillt/bitsery/compare/v4.4.0...v4.5.0) (2019-01-10)

### Features
* ability to create non default constructible objects, by defining private default constructor and making `friend class bitsery::Access;` to access it.
It is not necessary to enforce class invariant immediately, because internal object representation will be overriden anyway.

### Improvements
* `StdSmartPtr` supports `std::unique_ptr` with custom deleter.
* `*InputBufferAdapter`(all) can also accept const buffer;

### Bug fixes
* fixed deserialization in `bitsery/ext/std_map{set}` when target container is not empty.
* added missing template parameters for specializations on `std` containers in multiple files in `bitsery/ext/*`.

# [4.4.0](https://github.com/fraillt/bitsery/compare/v4.3.0...v4.4.0) (2019-01-08)

### Features
* new extensions **CompactValue** and **CompactValueAsObject**, stores integral values in less space if possible. This is useful when you're working with mostly small values, that in rare cases can be large.
E.g. `int64_t money = 8000;` will only use 2 bytes, instead of 8. **CompactValueAsObject** allows to use `ext()` overload, without specifying size of underlying type and sets BUFFER_OVERFLOW error if value doesn't fit in underlying type during deserialization.

### Improvements
* improved **PolymorphicContext**, allows to extend already registered hierarchy in one translation unit, using different type other than `PolymorphicBaseClass` to avoid symbol collision between translation units or libraries.
`registerBasesList` was modified, so that it could accept user defined type (instead of `PolymorphicBaseClass`) that is used to declare hierarchy, by default it is `PolymorphicBaseClass`.
This introduced breaking change, for those who used this syntax (`registerBasesList<MySerializer, Shape>({})`) during registration.
It is encouraged to define helper type, that could be used for registering hierarchy for serialization and deserialization [example](examples/smart_pointers_with_polymorphism.cpp).
`This is only relevant then you want to use **PolymorphicContext** between different translation units or libraries`.
```cpp
//libA
namespace bitsery {
    namespace ext {
        template<>
        struct PolymorphicBaseClass<Shape> : PolymorphicDerivedClasses<Circle, Rectangle> {};
    }
}
using MyPolymorphicClassesForRegistering = bitsery::ext::PolymorphicClassesList<Shape>;
...
    ctx.registerBasesList<MySerializer>(MyPolymorphicClassesForRegistering{}).

//otherLib
struct MySquare: Shape {...}
//now it must define different type (exactly the same as PolymorphicBaseClass) to declare hierarchy
template<typename TBase>
struct MyHierarchy {
    using Childs = PolymorphicClassesList<>;
};

template <>
struct MyHierarchy<Shape>: bitsery::ext::bitsery::ext::PolymorphicClassesList<MySquare> {};
...
//notice that we pass MyHierarchy as second argument
    ctx.registerBasesList<MySerializer, MyHierarchy>(MyPolymorphicClassesForRegistering{}).
```
* **PolymorphicContext** also get optional method `registerSingleBaseBranch`, that allows manually register hierarchies, this might be more convenient when using you need to register in different translation units (or libraries), but it is error-prone.

# [4.3.0](https://github.com/fraillt/bitsery/compare/v4.2.1...v4.3.0) (2018-08-23)

### Features

* added runtime polymorphism support for pointer like types (raw and smart pointers).
In order to enable polymorphism new **PolymorphicContext** was created. It provides capability to register classes with serializer/deserializer.
  * runtime polymorphism can be customized, by replacing **StandardRTTI** from <bitsery/ext/utils/rtti_utils.h> header.
* added smart pointers support for std::unique_ptr, std::shared_ptr and std::weak_ptr via **StdSmartPtr** extension.
* new **UnsafeInputBufferAdapter** doesn't check for buffer size on deserialization, on some compilers can improve deserialization performance up to ~40%.

### Improvements
* creatly improved interface for extending/implementing support for pointer like types. Now all pointer like types extends from **PointerObjectExtensionBase** and implements/configures required details.
* reimplemented **PointerOwner**, **PointerObserver**, **ReferencedByPointer**.
* reimplemented **PointerLinkingContext** to properly support shared objects and runtime polymorphism, pointer ownership for shared objects now has two states: SharedOwner e.g. std::shared_ptr and SharedObserver std::weak_ptr.

### Other notes
There is one *minor?* issue/limitation for pointer like types that uses virtual inheritance. When several pointers points to same object through different static type. it will not work correctly e.g.:
```cpp
struct Derived: virtual Base {...};
struct MyData {
    std::shared_ptr<Derived> sptr;
    std::weak_ptr<Base> wptddr;
}
```
In this example wptr and sptr have different static type, and *Derived* is virtually inherited from *Base*, so I get different pointer address for different types.

# [4.2.1](https://github.com/fraillt/bitsery/compare/v4.2.0...v4.2.1) (2018-03-09)

### Improvements
* changed CMake structure, to follow **Modern CMake** principles.
  * bitsery now has *install* target and **find_package(Bitsery)** exports **Bitsery::bitsery** target.
  * *GTest* no longer downloads as external application, but tries to find via *find_package*.
  * removed *ext* folder, and instead added *scripts* folder that contains few helper scripts for development, currently tested on Ubuntu.
* fixed/added few tests cases.

### Other notes
* some work was done on polymorphism support, but current solution, although working, but has many design and performance issues, and interfaces for extensibility might change drastically.

# [4.2.0](https://github.com/fraillt/bitsery/compare/v4.1.0...v4.2.0) (2017-11-12)

### Features

* serializer/deserializer can now have **internal context(s)** via configuration.
It is convenient way to pass context, when it doesn't convey useful information outside of serializer/deserializer and is default constructable.
* added **contextOrNull\<T\>()** overload to *BasicSerializer/BasicDeserializer*.
Difference between *contextOrNull\<T\>()* and *context\<T\>()* is, that using *context\<T\>()* code doesn't compile if T doesn't exists at all, while using *contextOrNull\<T\>()* code compiles, but returns *nullptr* at runtime.
* added inheritance support via extensions.
In order to correctly manage virtual inheritance two extensions was created in **<bitsery/ext/inheritance.h>** header:
  * **BaseClass\<TBase\>** - use when inheriting from objects without virtual inheritance.
  * **VirtualBaseClass\<TBase\>** - ensures that only one copy of each virtual base class is serialized.

  To keep track of virtual base classes **InheritanceContext** is required, but it is optional if no virtual bases exists in serialization flow.
  I.e. if context is not defined, code will not compile only if virtual inheritance is used.
  See [inheritance](examples/inheritance.cpp) for usage example.

### Improvements
* added optional ctor parameter for *PointerOwner* and *PointerObserver* - **PointerType**, which specifies if pointer can be null or not.
Default is **Nullable**.

# [4.1.0](https://github.com/fraillt/bitsery/compare/v4.0.1...v4.1.0) (2017-10-27)

### Features
* added raw pointers support via extensions.
In order to correctly manage pointer ownership, three extensions was created in **<bitsery/ext/pointer.h>** header:
  * **PointerOwner** - manages life time of the pointer, creates or destroys if required.
  * **PointerObserver** - doesn't own pointer so it doesn't create or destroy anything.
  * **ReferencedByPointer** - when non-owning pointer (*PointerObserver*) points to reference type, this extension marks this object as a valid target for PointerObserver.

  To validate and update pointers **PointerLinkingContext** have to be passed to serialization/deserialization.
  It ensures that all pointers are valid, that same pointer doesn't have multiple owners, and non-owning pointers doesn't point outside of scope (i.e. non owning pointers points to data that is serialized/deserialized), see [raw_pointers example](examples/raw_pointers.cpp) for usage example.

  *Currently polimorphism and std::shared_ptr,  std::unique_ptr is not supported.*

* added **context\<T\>()** overload to *BasicSerializer/BasicDeserializer* and now they became typesafe.
For better extensions support, added posibility to have multiple types in context with *std::tuple*.
E.g. when using multiple extensions, that requires specific contexts, together with your custom context, you can define your context as *std::tuple\<PointerLinkingContext, MyContext\>* and in serialization function you can correctly get your data via *context\<MyContext\>()*.


### Improvements

* new **OutputBufferedStreamAdapter** use internal buffer instead of directly writing to stream, can get more than 2x performance increase.
  * can use any contiguous container as internal buffer.
  * when using fixed-size, stack allocated container (*std::array*), buffer size via constructor is ignored.
  * default internal buffer is *std::array<char,256>*.
* added *static_assert* when trying to use *BufferAdapter* with non contiguous container.


# [4.0.1](https://github.com/fraillt/bitsery/compare/v4.0.0...v4.0.1) (2017-10-18)

### Improvements

* improved usage with Visual Studio:
  * improved CMake.
  * Visual Studio doesn't properly support expression SFINAE using std::void_t, so it was rewritten.
* refactorings to remove compiler warnings when using *-Wextra -Wno-missing-braces -Wpedantic -Weffc++*
* added assertion when session is empty (sessions is created via *growable* extension).
* stream adapter manually *setstate* to *std::ios_base::eofbit* when unable to read required bytes.

# [4.0.0](https://github.com/fraillt/bitsery/compare/v3.0.0...v4.0.0) (2017-10-13)

I feel that current library public API is complete, and should be stable for long time.
Most changes was made to improve performance or/and make library usage easier.

### Features

* new **flexible** syntax similar to *cereal* library.
This syntax no longer requires to specify explicit fundamental type sizes and container maxsize (container max size can be enforced by special function *maxSize*).
Be careful when using deserializing untrusted data and make sure to enforce fundamental type sizes when using on multiple platforms.
(use helper function *assertFundamentalTypeSizes* to enforce type sizes for multiple platforms)
* added streaming support, by introducing new **adapter** concept. Two adapter implementations is available: stream adapter, or buffer adapter.
* added missing std containers support: forward_list, deque, stack, queue, priority_queue, set, multiset, unordered_set, unordered_multiset.

### Breaking changes

* a lot of classes and files were renamed.
* improved error messages.
* traits reworked:
  * ContainerTraits get **isContiguous**.
  * TextTraits is separate from ContainerTraits, only has **length**, and **addNUL**.
  * buffer traits renamed to **BufferAdapterTraits** and removed difference type.
  * added **TValue** to all trait types, this is used to diagnose better errors.
* BasicBufferReader/Writer is split in two different types: **AdapterReader/Writer** and separate type that enables bit-packing operations  **AdapterBitPacking(Reader/Writer)Wrapper**.
* Serializer/Deserializer reworked
  * No longer copyable, because it stores adapter writer/reader.
  * Removed boolByte, boolBit, and added **boolValue** and it writes bit or byte, depeding on if bit-packing is enabled or not.
  * Bit-packing is enabled by calling **enableBitPacking**, if bitpacking is already enabled, this method will return same instance.
* changed defaults for *DefaultConfig*, BufferSessionsEnabled is false by default, because it doesn't work with input streams.
* serialization config no longer needs typedef *Buffer*.

# [3.0.0](https://github.com/fraillt/bitsery/compare/v2.0.1...v3.0.0) (2017-09-21)

### Features

* refactored interface, now works with C++11 compiler.
* new extension **Growable**, that allows to have forward/backward compatability within this functions serialization flow. It only allows to append new data at the end of to existing flow without breaking old consumers.
  * old consumer: correctly read old interfce and ignore new data.
  * new consumer: get defaults (zero values) for new fields, when reading old data.
* added new extension for associative *map* containers **ContainerMap**.
* friendly static_assert message when serializing **object**, that doesn't have **serialize** function defined.
* added **object** overload, that invokes user function/lambda with object. It is the same as calling user function directly, but makes more consistent API.
* Serializer/Deserializer now have optional *context* parameter, that might be required in some specific serialization cases.
* added traits for custom types specialization, in *details/traits.h*
* improved serialization performance: added support for fixed size buffer for BufferWriter for best performance.
* improved performance for reading/writing container sizes.
* added new method to BufferReader **getError** which returns on of three values: NO_ERROR, BUFFER_OVERFLOW, INVALID_BUFFER_DATA, also added setError method, that is only used by Deserializer.


### Breaking changes

* now all serializer/deserializer functions return void, to avoid undefined behaviour for functions parameters evaluation when using method chaining. There was no benefits apart from *nicer* syntax, but could have undefined behaviour when building complex serialization flows.
* removed **SERIALIZE** macro, and changed interface for all functions that use custom lambdas, to work with C++11. Now lambda function must capture serializer/deserializer.
* to make serializer a little bit *lighter* removed advanced features and made available as extensions via **extend** method
  * removed **range** method, instead added **ValueRange** extension.
  * removed **substitution** method, instead added **Entropy** extension, to sound more familiar to *entropy encoding* term.
* removed **array**, instead added fixed sizes overloads for **container**.
* removed **isValid** method from Deserializer, only BufferReader/Writer store states.
* container and text sizes representation changed, to allow much faster size reads/writes for small values.
* renamed functions:
  * **ext** to **extend** and changed its interface, to make it more easy to extend.
  * alias functions that write bytes directly no has *b* (meaning bytes) at the end of the name eg. *value4* now is *value4b*.
* changed BufferWriter/Reader behaviour:
  * added support for fixed size buffers for better serializer performance (more than 50% improvement). Default config is resizable buffer (*std::vector<uint8_t>*).
  * after serialization, call *getWrittenRange* to get valid range written to buffer, because BufferWritter for resizable buffer now always resize to *capacity* to avoid using *back_insert_iterator* for better performance.
  * BufferReader has constructor with iterators (range), and raw value type pointers (begin, end).
* renamed BufferReader **isCompleted** to  **isCompletedSuccessfully**, that returns true only when there is no errors and buffer is fully read.

### Bug fixes

* **readBytes** was reading in aligned mode, when scratch value from previous bit operations was zero.
* **writeBits** had incorrect assert when writing negative int64_t.

<a name="2.0.1"></a>
# [2.0.1](https://github.com/fraillt/bitsery/compare/v2.0.0...v2.0.1) (2017-08-12)

### Other notes
* added travis build status

<a name="2.0.0"></a>
# [2.0.0](https://github.com/fraillt/bitsery/compare/v1.1.1...v2.0.0) (2017-07-25)

### Features

* endianness support, default network configuration is *little endian*
* added user extensible function **ext**, to work with objects that require different serialization/deserialization path (e.g. pointers)
* **optional** extension (for *ext* function), to work with *std::optional* types

### Breaking changes

* file structure changed, added *details* folder.
* no longer support for implicit size converions for all functions (*value*, *array*, *container*), instead added helper functions with specific size, to avoid typing *s.template value<1>...* within serialization function body
* changed parameters order for all functions that use custom function (lambda)
* *BufferReader* and *BufferWriter* is now alias types for real types *BasicBufferReader/Writer&lt;DefaultConfig&gt;* (*DefaultConfig* is defined in *common.h*)

### Bug fixes
* *align* method fixed in *BufferReader*

# [1.1.1](https://github.com/fraillt/bitsery/compare/v1.0.0...v1.1.1) (2017-02-23)

### Notes
* changed folder structure
* added more BufferReader constructors

# 1.0.0 (2017-02-22)

### Features

Serialization functions:
* **value** - [fundamental types](doc/design/fundamental_types.md)
* **container** - dynamic size containers
* **array** - fixed size containers
* **text** - for c-array and std::string
* **range** - compresion for fundamental types (e.g. int between [255..512] will take up 8bits
* **substitution** - default value from list (e.g. 4d vector, that is most of the time equals to [0,0,0,1] can store only 1bit)
* **boolBit**/**boolByte** - serialize bool, as 1bit or 1byte.
