//
// Created by fraillt on 18.4.26.
//
#include <bitsery/adapter/buffer.h>
#include <bitsery/bitsery.h>
#include <bitsery/ext/inheritance.h>
#include <bitsery/ext/pointer.h>
#include <bitsery/ext/std_smart_ptr.h>
#include <bitsery/traits/vector.h>
#include <cassert>
#include <memory>

// in order to work with polymorphic types, we need to describe few steps:
//  1) describe relationships between base and derived types
//  this will allow to know what are possible types reachable from base class
//  2) bind serializer to base class
//  this will allow to iterate through all types, and add serialization
//  functions, without this step compiler would simply remove functions that are
//  not bound at compile-time even it we use type at runtime.

using bitsery::ext::BaseClass;

using bitsery::ext::PointerObserver;
using bitsery::ext::StdSmartPtr;

// define our data structures
struct Color
{
  float r{}, g{}, b{};
  bool operator==(const Color& o) const
  {
    return std::tie(r, g, b) == std::tie(o.r, o.g, o.b);
  }
};

struct Shape
{
  Color clr{};
  virtual ~Shape() = 0;
};

Shape::~Shape() = default;

struct Circle : Shape
{
  int32_t radius{};
  bool operator==(const Circle& o) const
  {
    return std::tie(radius, clr) == std::tie(o.radius, o.clr);
  }
};

struct Rectangle : Shape
{
  int32_t width{};
  int32_t height{};
  bool operator==(const Rectangle& o) const
  {
    return std::tie(width, height, clr) == std::tie(o.width, o.height, o.clr);
  }
};

struct RoundedRectangle : Rectangle
{
  int32_t radius{};
  bool operator==(const RoundedRectangle& o) const
  {
    return std::tie(radius, static_cast<const Rectangle&>(*this)) ==
           std::tie(o.radius, static_cast<const Rectangle&>(o));
  }
};

// define serialization functions
template<typename S>
void
serialize(S& s, Color& o)
{
  // in real world scenario, it might be possible to serialize this using
  // ValueRange, to map values in smaller space but for the sake of this example
  // keep it simple
  s.value4b(o.r);
  s.value4b(o.g);
  s.value4b(o.b);
}

template<typename S>
void
serialize(S& s, Shape& o)
{
  s.object(o.clr);
}

template<typename S>
void
serialize(S& s, Circle& o)
{
  s.ext(o, bitsery::ext::BaseClass<Shape>{});
  s.value4b(o.radius);
}

template<typename S>
void
serialize(S& s, Rectangle& o)
{
  s.ext(o, bitsery::ext::BaseClass<Shape>{});
  s.value4b(o.width);
  s.value4b(o.height);
}

template<typename S>
void
serialize(S& s, RoundedRectangle& o)
{
  s.ext(o, bitsery::ext::BaseClass<Rectangle>{});
  s.value4b(o.radius);
}

// define our test structure
struct SomeShapes
{
  std::vector<std::shared_ptr<Shape>> sharedList;
  std::unique_ptr<Shape> uniquePtr;
  // weak ptr and refPtr will point to sharedList
  std::weak_ptr<Shape> weakPtr;
  Shape* refPtr;
};

// creates object, and populates some data
SomeShapes
createData()
{
  SomeShapes data{};
  {
    auto tmp = new RoundedRectangle{};
    tmp->height = 151572;
    tmp->width = 488795;
    tmp->radius = 898;
    tmp->clr.r = 0.5f;
    tmp->clr.g = 1.0f;
    tmp->clr.b = 1.0f;
    data.uniquePtr.reset(tmp);
  }
  {
    auto tmp = new Circle{};
    tmp->radius = 75987;
    tmp->clr.r = 0.5f;
    tmp->clr.g = 0.0f;
    tmp->clr.b = 1.0f;
    data.sharedList.emplace_back(tmp);
  }
  {
    auto tmp = new Rectangle{};
    tmp->height = 15157;
    tmp->width = 48879;
    tmp->clr.r = 1.0f;
    tmp->clr.g = 0.0f;
    tmp->clr.b = 0.0f;
    data.sharedList.emplace_back(tmp);
  }
  data.weakPtr = data.sharedList[0];
  data.refPtr = data.sharedList[1].get();

  return data;
}

template<typename S>
void
serialize(S& s, SomeShapes& o)
{
  s.ext(o.uniquePtr, StdSmartPtr{});
  // to make things more interesting first serialize weakPtr and refPtr,
  // even though objects that weakPtr and refPtr is serialized later,
  // bitsery will work regardless
  s.ext(o.weakPtr, StdSmartPtr{});
  s.ext(o.refPtr, PointerObserver{});
  s.container(o.sharedList, 100, [](S& s, std::shared_ptr<Shape>& item) {
    s.ext(item, StdSmartPtr{});
  });
}

// STEP 1
// define relationships between base and derived classes

namespace bitsery {
namespace ext {

// for each base class define DIRECTLY derived classes
// e.g. PolymorphicBaseClass<Shape> : PolymorphicDerivedClasses<Circle,
// Rectangle, RoundedRectangle>
//  is incorrect, because RoundedRectangle does not directly derive from Shape
template<>
struct PolymorphicBaseClass<Shape>
  : PolymorphicDerivedClasses<Circle, Rectangle>
{};

template<>
struct PolymorphicBaseClass<Rectangle>
  : PolymorphicDerivedClasses<RoundedRectangle>
{};
}
}

// convenient type that stores all our types, so that we could easily register
// and also it automatically ensures, that classes is registered in the same
// order for serialization and deserialization
using MyPolymorphicClassesForRegistering =
  bitsery::ext::PolymorphicClassesList<Shape>;

// some helper types
using Buffer = std::vector<uint8_t>;
using Writer = bitsery::OutputBufferAdapter<Buffer>;
using Reader = bitsery::InputBufferAdapter<Buffer>;

// we need to define few things in order to work with polymorphism
// 1) we need pointer linking context to work with pointers
// 2) we need polymorphic context to be able to work with polymorphic types
using TContext =
  std::tuple<bitsery::ext::PointerLinkingContext,
             bitsery::ext::PolymorphicContext<bitsery::ext::StandardRTTI>>;
// NOTE:
//  RTTI can be customizable, if you can't use dynamic_cast and typeid, and have
//  'custom' solution
using MySerializer = bitsery::Serializer<Writer, TContext>;
using MyDeserializer = bitsery::Deserializer<Reader, TContext>;

// checks if deserialized data is equal
void
assertSameShapes(const SomeShapes& data, const SomeShapes& res)
{
  {
    auto d = dynamic_cast<RoundedRectangle*>(data.uniquePtr.get());
    auto r = dynamic_cast<RoundedRectangle*>(res.uniquePtr.get());
    assert(r != nullptr);
    assert(*d == *r);
  }
  {
    auto d = dynamic_cast<Circle*>(data.sharedList[0].get());
    auto r = dynamic_cast<Circle*>(res.sharedList[0].get());
    assert(r != nullptr);
    assert(*d == *r);
  }
  {
    auto d = dynamic_cast<Rectangle*>(data.sharedList[1].get());
    auto r = dynamic_cast<Rectangle*>(res.sharedList[1].get());
    assert(r != nullptr);
    assert(*d == *r);
  }
  assert(res.weakPtr.lock().get() == res.sharedList[0].get());
  assert(res.refPtr == res.sharedList[1].get());
}

int
main()
{

  auto data = createData();

  // create buffer to store data
  Buffer buffer{};
  size_t writtenSize{};
  // we will not use quickSerialization/Deserialization functions to show, that
  // we need to register polymorphic classes, explicitly
  {

    // STEP 2
    //  before start serialization/deserialization,
    //  bind it with base polymorphic types, it will go through all reachable
    //  classes that is defined in first step. NOTE: you dont need to add
    //  Rectangle to reach for RoundedRectangle
    TContext ctx{};
    std::get<1>(ctx).registerBasesList<MySerializer>(
      MyPolymorphicClassesForRegistering{});
    // create writer and serialize
    MySerializer ser{ ctx, buffer };
    ser.object(data);
    ser.adapter().flush();
    writtenSize = ser.adapter().writtenBytesCount();

    // make sure that pointer linking context is valid
    // this ensures that all non-owning pointers points to data that has been
    // serialized, so we can successfully reconstruct pointers after
    // deserialization
    assert(std::get<0>(ctx).isValid());
  }
  SomeShapes res{};
  {
    TContext ctx{};
    std::get<1>(ctx).registerBasesList<MyDeserializer>(
      MyPolymorphicClassesForRegistering{});
    // deserialize our data
    MyDeserializer des{ ctx, buffer.begin(), writtenSize };
    des.object(res);
    assert(des.adapter().error() == bitsery::ReaderError::NoError &&
           des.adapter().isCompletedSuccessfully());
    // also check for dangling pointers, after deserialization
    assert(std::get<0>(ctx).isValid());
    // clear shared state from pointer linking context,
    // it is only required if there are any pointers that manage shared state,
    // e.g. std::shared_ptr
    assert(res.weakPtr.use_count() ==
           2); // one in sharedList and one in pointer linking context
    std::get<0>(ctx).clearSharedState();
    assert(res.weakPtr.use_count() == 1);
  }
  assertSameShapes(data, res);
  return 0;
}
