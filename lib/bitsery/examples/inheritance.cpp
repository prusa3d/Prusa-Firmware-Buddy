//
// this example covers all the corner cases that can happen using inheritance
// in reality virtual inherintance is usually avoided, so your code would look
// much simpler.
//

#include <bitsery/adapter/buffer.h>
#include <bitsery/bitsery.h>
#include <bitsery/traits/vector.h>

// include inheritance extension
// this header contains two extensions, that specifies inheritance type of base
// class
//   BaseClass - normal inheritance
//   VirtualBaseClass - when virtual inheritance is used
// in order for virtual inheritance to work, InheritanceContext is required. for
// normal inheritance it is not required
#include <bitsery/ext/inheritance.h>

using bitsery::ext::BaseClass;
using bitsery::ext::VirtualBaseClass;

struct Base
{
  uint8_t x{};
  // Base doesn't have to be polymorphic class, inheritance works at
  // compile-time.
};
template<typename S>
void
serialize(S& s, Base& o)
{
  s.value1b(o.x);
}

struct Derive1 : virtual Base
{ // virtually inherits from base
  uint8_t y1{};
};
template<typename S>
void
serialize(S& s, Derive1& o)
{
  // define virtual inheritance, it will not compile if InheritanceContext is
  // not defined in serializer/deserializer
  s.ext(o, VirtualBaseClass<Base>{});
  s.value1b(o.y1);
}

// to make it more interesting, serialize private member
struct Derived2 : virtual Base
{
  explicit Derived2(uint8_t y)
    : y2{ y }
  {
  }

  uint8_t getY2() const { return y2; };

private:
  friend bitsery::Access;
  uint8_t y2{};
  template<typename S>
  void serialize(S& s)
  {
    // notice virtual inheritance
    s.ext(*this, VirtualBaseClass<Base>{});
    s.value1b(y2);
  }
};

struct MultipleInheritance
  : Derive1
  , Derived2
{
  explicit MultipleInheritance(uint8_t y2)
    : Derived2{ y2 }
  {
  }
  uint8_t z{};
};
template<typename S>
void
serialize(S& s, MultipleInheritance& o)
{
  // has two bases, serialize them separately
  s.ext(o, BaseClass<Derive1>{});
  s.ext(o, BaseClass<Derived2>{});
  s.value1b(o.z);
}

namespace bitsery {
// call to serialize function with Derived2 and MultipleInheritance is
// ambiguous, it matches two serialize functions: Base classes non-member fnc
// and Derived2 member fnc we need explicitly select which function to use
template<>
struct SelectSerializeFnc<Derived2> : UseMemberFnc
{};

// multiple inheritance has non-member serialize function defined
template<>
struct SelectSerializeFnc<MultipleInheritance> : UseNonMemberFnc
{};
}

// some helper types
using Buffer = std::vector<uint8_t>;
using Writer = bitsery::OutputBufferAdapter<Buffer>;
using Reader = bitsery::InputBufferAdapter<Buffer>;

int
main()
{

  MultipleInheritance data{ 98 };
  data.x = 254;
  data.y1 = 47;
  data.z = 1;

  Buffer buf{};

  bitsery::ext::InheritanceContext ctx1;
  auto writtenSize = bitsery::quickSerialization(ctx1, Writer{ buf }, data);
  assert(writtenSize ==
         4); // base is serialized once, because it is inherited virtually

  MultipleInheritance res{ 0 };
  bitsery::ext::InheritanceContext ctx2;
  auto state = bitsery::quickDeserialization(
    ctx2, Reader{ buf.begin(), writtenSize }, res);
  assert(state.first == bitsery::ReaderError::NoError && state.second);
  assert(data.x == res.x && data.y1 == res.y1 && data.getY2() == res.getY2() &&
         data.z == res.z);
}
