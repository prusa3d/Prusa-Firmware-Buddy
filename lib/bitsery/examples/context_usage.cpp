#include <bitsery/adapter/buffer.h>
#include <bitsery/bitsery.h>

#include <bitsery/traits/string.h>
#include <bitsery/traits/vector.h>

#include <bitsery/ext/value_range.h>

namespace MyTypes {

struct Monster
{
  Monster() = default;
  Monster(std::string _name, uint32_t minDmg, uint32_t maxDmg)
    : name{ _name }
    , minDamage{ minDmg }
    , maxDamage{ maxDmg }
  {
  }

  std::string name{};
  uint32_t minDamage{};
  uint32_t maxDamage{};
  //...
};

struct GameState
{
  std::vector<Monster> monsters;
};

// default flow for monster
template<typename S>
void
serialize(S& s, Monster& o)
{
  s.text1b(o.name, 20);
  s.value4b(o.minDamage);
  s.value4b(o.maxDamage);
}

template<typename S>
void
serialize(S& s, GameState& o)
{
  // we can have multiple types in context with std::tuple
  // if data type doesn't match then it will be compile time error
  // NOTE: if context is optional then you can call contextOrNull<T>, and it
  // will return null if T doesn't exists
  auto maxMonsters = s.template context<int>();
  auto& dmgRange = s.template context<std::pair<uint32_t, uint32_t>>();

  s.container(o.monsters, maxMonsters, [&dmgRange](S& s, Monster& m) {
    s.text1b(m.name, 20);
    // we know min/max damage range for monsters, so we can use this range
    // instead of full value
    bitsery::ext::ValueRange<uint32_t> range{ dmgRange.first, dmgRange.second };
    // enable bit packing
    s.enableBitPacking([&m, &range](typename S::BPEnabledType& sbp) {
      sbp.ext(m.minDamage, range);
      sbp.ext(m.maxDamage, range);
    });
  });
}

}

// context can contain multiple types by wrapping these types in std::tuple
// in serialization function we can get type that we need like this:
//   s.template context<int>();
// this templated version also works if our context is the same as cast:
//   struct MyContext {...};
//   ...
//   s.template context<MyContext>();
// NOTE:
//  if your context has no additional usage outside of serialization flow,
//  then you can create it internally via configuration (see inheritance.cpp)
using Context = std::tuple<int, std::pair<uint32_t, uint32_t>>;

// use fixed-size buffer
using Buffer = std::vector<uint8_t>;
// define adapter types,
using OutputAdapter = bitsery::OutputBufferAdapter<Buffer>;
using InputAdapter = bitsery::InputBufferAdapter<Buffer>;

int
main()
{

  MyTypes::GameState data{};
  data.monsters.push_back({ "weaksy", 100, 200 });
  data.monsters.push_back({ "bigsy", 500, 1000 });
  data.monsters.push_back({ "tootoo", 350, 750 });

  // set context
  Context ctx{};
  // max monsters
  std::get<0>(ctx) = 4;
  // damage range
  std::get<1>(ctx).first = 100;
  std::get<1>(ctx).second = 1000;

  // create buffer to store data to
  Buffer buffer{};
  auto writtenSize =
    bitsery::quickSerialization(ctx, OutputAdapter{ buffer }, data);

  MyTypes::GameState res{};
  auto state = bitsery::quickDeserialization(
    ctx, InputAdapter{ buffer.begin(), writtenSize }, res);

  assert(state.first == bitsery::ReaderError::NoError && state.second);
}
