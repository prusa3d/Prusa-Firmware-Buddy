// include bitsery.h to get serialization and deserialization classes
#include <bitsery/bitsery.h>
// in ordered to serialize/deserialize data to buffer, include buffer adapter
#include <bitsery/adapter/buffer.h>
// bitsery itself doesn't is lightweight, and doesnt include any unnessessary
// files, traits helps library to know how to use types correctly, in this case
// we'll be using vector both, to serialize/deserialize data and to store use as
// a buffer.
#include <bitsery/traits/vector.h>

enum class MyEnum : uint16_t
{
  V1,
  V2,
  V3
};
struct MyStruct
{
  uint32_t i;
  MyEnum e;
  std::vector<float> fs;
};

// define how object should be serialized/deserialized
template<typename S>
void
serialize(S& s, MyStruct& o)
{
  s.value4b(o.i);          // fundamental types (ints, floats, enums) of size 4b
  s.value2b(o.e);
  s.container4b(o.fs, 10); // resizable containers also requires maxSize, to
                           // make it safe from buffer-overflow attacks
}

// some helper types
using Buffer = std::vector<uint8_t>;
using OutputAdapter = bitsery::OutputBufferAdapter<Buffer>;
using InputAdapter = bitsery::InputBufferAdapter<Buffer>;

int
main()
{
  // set some random data
  MyStruct data{ 8941, MyEnum::V2, { 15.0f, -8.5f, 0.045f } };
  MyStruct res{};

  // create buffer to store data
  Buffer buffer;
  // use quick serialization function,
  // it will use default configuration to setup all the nesessary steps
  // and serialize data to container
  auto writtenSize = bitsery::quickSerialization<OutputAdapter>(buffer, data);

  // same as serialization, but returns deserialization state as a pair
  // first = error code, second = is buffer was successfully read from begin to
  // the end.
  auto state = bitsery::quickDeserialization<InputAdapter>(
    { buffer.begin(), writtenSize }, res);

  assert(state.first == bitsery::ReaderError::NoError && state.second);
  assert(data.fs == res.fs && data.i == res.i && data.e == res.e);
}
