# Quick Start

This is a quick guide to get **bitsery** up and running in a matter of minutes.
The only prerequisite for running bitsery is a modern C++11 compliant compiler, such as GCC 4.9.4, clang 3.4, MSVC 2015, or newer.
Older versions might work, but they have not been tested.

## Get bitsery

**bitsery** can be directly included in your project or installed anywhere you can access header files.
Grab the latest version, and include directory `bitsery_base_dir/include/` to your project.
There's nothing to build or make - **bitsery** is header only.

## Include required headers and define some helper types

```cpp
#include <bitsery/bitsery.h>
#include <bitsery/adapter/buffer.h>
#include <bitsery/traits/vector.h>
#include <bitsery/traits/string.h>

using Buffer = std::vector<uint8_t>;
using OutputAdapter = bitsery::OutputBufferAdapter<Buffer>;
using InputAdapter = bitsery::InputBufferAdapter<Buffer>;

```

**bitsery** is very lightweight, so we need to explicitly include what we need.

Include  |  Description
--|--
`<bitsery/bitsery.h>`  |  This is a core header, that includes our Serializer and Deserializer.
`<bitsery/adapter/buffer.h>`  |  In order to write/read data, we need a specific adapter, depending on what underlying buffer will be. In this example, we'll be using `std::vector` as our buffer, so we include the buffer adapter.
`<bitsery/traits/...>`  |  Traits tell the library how to efficiently serialize a particular container. Many common STL containers are supported out of the box.

Create alias types for *InputAdapter* and *OutputAdapter* using our vector as buffer.

## Add serialization method for your type

**bitsery** needs to know which data members to serialize in your classes.
Let it know by implementing a serialize method for your type:

```cpp
struct MyStruct {
    uint32_t i;
    char str[6];
    std::vector<float> fs;
};

template <typename S>
void serialize(S& s, MyStruct& o) {
    s.value4b(o.i);
    s.text1b(o.str);
    s.container4b(o.fs, 100);
};
```

**bitsery** also allows to define serialize function in side your class, and can also serialize private class members, just make *friend bitsery::Access;*

**bitsery** supports two ways how to describe your serialization flow: *verbose syntax* (as in example) or *brief syntax*, similar to *cereal* library, just include `<bitsery/brief_syntax.h>` to use it.

This example we choosed probably unfamiliar verbose syntax, so lets explain core functionality that you'll use all the time:
* **s.value4b(o.i);** serialize fundamental types (ints, floats, enums) value**4b** means, that data type is 4 bytes. If you use same code on different machines, if it compiles it means it is compatible.
* **s.text1b(o.str);** serialize text (null-terminated) of char type, if you use *wchar* then you would write *text2b* or *text4b* depending on the OS platform.
* **s.container4b(o.fs, 100);** serializes any container of fundamental types of size 4bytes, **100** is max size of container.
**Bitsery** is designed to be save with untrusted (malicious) data from network, so for dynamic containers you always need to provide max possible size available, to avoid buffer-overflow attacks.
**text** didn't had this max size specified, because it was serializing fixed size container.

External serialization functions should be placed either in the same namespace as the types they serialize or in the **bitsery** namespace so that the compiler can find them properly.

## Serialization and deserialization

Create buffer and use helper functions for serialization and deserialization.

```cpp
Buffer buffer;

  auto writtenSize = bitsery::quickSerialization(OutputAdapter{buffer}, data);
  auto state = bitsery::quickDeserialization(InputAdapter{buffer.begin(), writtenSize}, res);
```

These helper functions use default configuration *bitsery::DefaultConfig*
* **quickSerialization** create serializer using output adapter, serializes data and returns written size.
* **quickDeserialization** create deserializer using input adapter, deserializes to object, and returns deserialization state.
deserialization state has two properties, error code and bool that indicates if buffer was fully read and there is no errors.

## Full example code

```cpp
#include <bitsery/bitsery.h>
#include <bitsery/adapter/buffer.h>
#include <bitsery/traits/vector.h>
#include <bitsery/traits/string.h>

using Buffer = std::vector<uint8_t>;
using OutputAdapter = bitsery::OutputBufferAdapter<Buffer>;
using InputAdapter = bitsery::InputBufferAdapter<Buffer>;

struct MyStruct {
  uint32_t i;
  char str[6];
  std::vector<float> fs;
};

template <typename S>
void serialize(S& s, MyStruct& o) {
  s.value4b(o.i);
  s.text1b(o.str);
  s.container4b(o.fs, 100);
}

int main() {
  MyStruct data{8941, "hello", {15.0f, -8.5f, 0.045f}};
  MyStruct res{};

  Buffer buffer;
  auto writtenSize = bitsery::quickSerialization(OutputAdapter{buffer}, data);
  auto state = bitsery::quickDeserialization(InputAdapter{buffer.begin(), writtenSize}, res);

  assert(state.first == bitsery::ReaderError::NoError && state.second);
  assert(data.fs == res.fs && data.i == res.i && std::strcmp(data.str, res.str) == 0);
}
```

**currently documentation and tutorial is progress, but for more usage examples see examples folder**
