#include <bitsery/adapter/buffer.h>
#include <bitsery/bitsery.h>
// we'll be using std::array as a buffer type, so include traits for this
#include <bitsery/traits/array.h>
#include <bitsery/traits/string.h>
#include <bitsery/traits/vector.h>
// include extension that will allow to compress our data
#include <bitsery/ext/value_range.h>

namespace MyTypes {

struct Vec3
{
  float x, y, z;
};

struct Monster
{
  Vec3 pos;
  std::vector<Vec3> path;
  std::string name;
};

template<typename S>
void
serialize(S& s, MyTypes::Vec3& o)
{
  s.value4b(o.x);
  s.value4b(o.y);
  s.value4b(o.z);
}

template<typename S>
void
serialize(S& s, Monster& o)
{
  s.text1b(o.name, 20);
  s.object(o.pos);
  // compress path in a range of -1.0 .. 1.0 with 0.01 precision
  // enableBitPacking creates separate serializer/deserializer object, that
  // contains bit packing operations
  s.enableBitPacking([&o](typename S::BPEnabledType& sbp) {
    sbp.container(o.path, 1000, [](typename S::BPEnabledType& sbp, Vec3& vec3) {
      constexpr bitsery::ext::ValueRange<float> range{ -1.0f, 1.0f, 0.01f };
      sbp.ext(vec3.x, range);
      sbp.ext(vec3.y, range);
      sbp.ext(vec3.z, range);
    });
  });
}
}

// use fixed-size buffer
using Buffer = std::array<uint8_t, 10000>;
using OutputAdapter = bitsery::OutputBufferAdapter<Buffer>;
using InputAdapter = bitsery::InputBufferAdapter<Buffer>;

int
main()
{
  // set some random data
  MyTypes::Monster data{};
  data.name = "lew";

  // create buffer to store data to
  Buffer buffer{};
  auto writtenSize = bitsery::quickSerialization<OutputAdapter>(buffer, data);

  MyTypes::Monster res{};
  auto state = bitsery::quickDeserialization<InputAdapter>(
    { buffer.begin(), writtenSize }, res);

  assert(state.first == bitsery::ReaderError::NoError && state.second);
}
