#include <bitsery/adapter/buffer.h>
#include <bitsery/bitsery.h>
// include traits for types, that we'll be using
#include <bitsery/traits/array.h>
#include <bitsery/traits/string.h>
#include <bitsery/traits/vector.h>
// include extension that will allow to have backward/forward compatibility
#include <bitsery/ext/growable.h>

namespace MyTypes {

// define data
enum Color : uint8_t
{
  Red,
  Green,
  Blue
};

struct Vec3
{
  float x, y, z;
};

struct Weapon
{
  std::string name{};
  int16_t damage{};
  Weapon() = default;
  Weapon(const std::string& _name, int16_t dmg)
    : name{ _name }
    , damage{ dmg }
  {
  }

private:
  // define serialize function as private, and give access to bitsery
  friend bitsery::Access;
  template<typename S>
  void serialize(S& s)
  {
    // forward/backward compatibility for weapons
    s.ext(*this, bitsery::ext::Growable{}, [](S& s, Weapon& o1) {
      s.text1b(o1.name, 20);
      s.value2b(o1.damage);
    });
  }
};

struct Monster
{
  Vec3 pos;
  int16_t mana;
  int16_t hp;
  std::string name;
  std::vector<uint8_t> inventory;
  Color color;
  std::vector<Weapon> weapons;
  Weapon equipped;
  std::vector<Vec3> path;
};

template<typename S>
void
serialize(S& s, Vec3& o)
{
  s.value4b(o.x);
  s.value4b(o.y);
  s.value4b(o.z);
}

template<typename S>
void
serialize(S& s, Monster& o)
{
  // forward/backward compatibility for monsters
  s.ext(o, bitsery::ext::Growable{}, [](S& s, Monster& o1) {
    s.value1b(o1.color);
    s.value2b(o1.mana);
    s.value2b(o1.hp);
    s.object(o1.equipped);
    s.object(o1.pos);
    s.container(o1.path, 1000);
    s.container(o1.weapons, 100);
    s.container1b(o1.inventory, 50);
    s.text1b(o1.name, 20);
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
  data.weapons.push_back(MyTypes::Weapon{ "GoodWeapon", 100 });

  // create buffer to store data to
  Buffer buffer{};
  auto writtenSize = bitsery::quickSerialization<OutputAdapter>(buffer, data);

  MyTypes::Monster res{};
  // deserialize
  auto state = bitsery::quickDeserialization<InputAdapter>(
    { buffer.begin(), writtenSize }, res);

  assert(state.first == bitsery::ReaderError::NoError && state.second);
}
