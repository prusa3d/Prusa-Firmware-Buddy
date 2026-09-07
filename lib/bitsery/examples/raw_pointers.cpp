#include <bitsery/adapter/buffer.h>
#include <bitsery/bitsery.h>
#include <bitsery/traits/vector.h>

// include pointers extension
// this header contains multiple extensions for different pointer types and
// pointer linking context, that validates pointer ownership and checks if there
// are and no dangling pointers after serialization/deserialization. dangling
// pointer in this context means, that non-owning pointer points to data, that
// was not serialized.
#include <bitsery/ext/pointer.h>

using bitsery::ext::PointerObserver;
using bitsery::ext::PointerOwner;
using bitsery::ext::PointerType;
using bitsery::ext::ReferencedByPointer;

enum class MyEnum : uint16_t
{
  V1,
  V2,
  V3
};
struct MyStruct
{
  MyStruct(uint32_t i_, MyEnum e_, std::vector<float> fs_)
    : i{ i_ }
    , e{ e_ }
    , fs{ fs_ }
  {
  }
  MyStruct()
    : MyStruct{ 0, MyEnum::V1, {} }
  {
  }
  uint32_t i;
  MyEnum e;
  std::vector<float> fs;
};

template<typename S>
void
serialize(S& s, MyStruct& o)
{
  s.value4b(o.i);
  s.value2b(o.e);
  s.container4b(o.fs, 10);
}

// our test data
struct Test1Data
{
  // regular data, nothing fancy here
  MyStruct o1;
  int32_t i1;
  // these container elements can be referenced by pointers
  std::vector<MyStruct> vdata;
  // container that holds non owning pointers (observers),
  std::vector<MyStruct*> vptr;
  // treat it as is observer
  MyStruct* po1;
  // we treat this as owner (responsible for allocation/deallocation
  int32_t* pi1;

private:
  friend bitsery::Access;

  template<typename S>
  void serialize(S& s)
  {
    // just a regular fields
    s.object(o1);
    s.value4b(i1);

    // set container elements to be candidates for non-owning pointers
    s.container(
      vdata, 100, [](S& s, MyStruct& d) { s.ext(d, ReferencedByPointer{}); });
    // contains non owning pointers
    //
    // IMPORTANT !!!
    // ALWAYS ACCEPT BY REFERENCE like this: T* (&obj)
    // if using c++14, then auto& always works.
    //
    // you can also serialize non owning pointers first, pointer linking context
    // will keep track on them and as soon as pointer owner data is
    // deserialized, all non-owning pointers will be updated
    s.container(
      vptr, 100, [](S& s, MyStruct*(&d)) { s.ext(d, PointerObserver{}); });
    // observer
    s.ext(po1, PointerObserver{});
    // owner, mark it as not null
    s.ext4b(pi1, PointerOwner{ PointerType::NotNull });
  }
};

// some helper types
using Buffer = std::vector<uint8_t>;
using Writer = bitsery::OutputBufferAdapter<Buffer>;
using Reader = bitsery::InputBufferAdapter<Buffer>;

// we will need PointerLinkingContext to work with pointers
// if we would require additional context for our own custom flow, we can define
// it as tuple like this:
//   std::tuple<MyContext,ext::PointerLinkingContext>
// and other code will work as expected as long as it cast to proper type.
// see context_usage.cpp for usage example

int
main()
{
  // set some random data
  Test1Data data{};
  data.vdata.emplace_back(8941, MyEnum::V1, std::vector<float>{ 4.4f });
  data.vdata.emplace_back(15478, MyEnum::V2, std::vector<float>{ 15.0f });
  data.vdata.emplace_back(59, MyEnum::V3, std::vector<float>{ -8.5f, 0.045f });
  // container of non owning pointers (observers)
  data.vptr.emplace_back(nullptr);
  data.vptr.emplace_back(std::addressof(data.vdata[0]));
  data.vptr.emplace_back(std::addressof(data.vdata[2]));
  // regular fields
  data.o1 = MyStruct{ 4, MyEnum::V2, { 57.078f } };
  data.i1 = 9455;
  // observer
  data.po1 = std::addressof(data.vdata[1]);
  // owning pointer
  data.pi1 = new int32_t{};

  // create buffer to store data
  Buffer buffer{};
  size_t writtenSize{};
  // in order to use pointers, we need to pass pointer linking context
  // serializer/deserializer
  {
    bitsery::ext::PointerLinkingContext ctx{};
    writtenSize = quickSerialization(ctx, Writer{ buffer }, data);

    // make sure that pointer linking context is valid
    // this ensures that all non-owning pointers points to data that has been
    // serialized, so we can successfully reconstruct pointers after
    // deserialization
    assert(ctx.isValid());
  }

  Test1Data res{};
  {
    bitsery::ext::PointerLinkingContext ctx{};
    auto state =
      quickDeserialization(ctx, Reader{ buffer.begin(), writtenSize }, res);
    // check if everything went find
    assert(state.first == bitsery::ReaderError::NoError && state.second);
    // also check for dangling pointers, after deserialization
    assert(ctx.isValid());
  }
  // owning pointers owns data
  assert(*res.pi1 == *data.pi1);
  assert(res.pi1 != data.pi1);
  // observers, points to other data
  assert(res.vptr[0] == nullptr);
  assert(res.vptr[1] == std::addressof(res.vdata[0]));
  assert(res.vptr[2] == std::addressof(res.vdata[2]));
  assert(res.po1 == std::addressof(res.vdata[1]));

  // delete raw owning pointers
  delete data.pi1;
  delete res.pi1;
}
