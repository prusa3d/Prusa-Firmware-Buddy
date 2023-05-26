# CTHASH (Compile Time Hash)

This library is constexpr implementation of SHA-2, SHA-3, and xxhash family of hashes.

## Supported hash function

The library also implements hash_value literals in namespace `cthash::literals` (suffixes in parenthesis for each hash function type). This literal types doesn't compute hash value of its content, but are merely strong typed value from specified hash algorithm (eg. so you won't mix up SHA-256 and SHA3-256 results).

* SHA-224 (`_sha224`)
* SHA-256 (`_sha256`)
* SHA-384 (`_sha384`)
* SHA-512 (`_sha512`)
* SHA-512/t (only for T dividable by 8) (`_sha512_224`, `_sha512_256`)

* SHA3-224 (`_sha3_224`)
* SHA3-256 (`_sha3_256`)
* SHA3-384 (`_sha3_384`)
* SHA3-512 (`_sha3_512`)

* SHAKE-128 (`_shake128`)
* SHAKE-256 (`_shake256`)

* XXHASH-32 (`_xxh32`)
* XXHASH-64 (`_xxh64`)

## Example

SHA256:
```c++
using namespace cthash::literals;

constexpr auto my_hash = cthash::sha256{}.update("hello there!").final();
// or
constexpr auto my_hash = cthash::simple<cthash::sha256>("hello there!");

static_assert(my_hash == "c69509590d81db2f37f9d75480c8efedf79a77933db5a8319e52e13bfd9874a3"_sha256);
```

SHA-3:
```c++
using namespace cthash::literals;

constexpr auto my_hash = cthash::sha3_256{}.update("hello there!").final();
// or
constexpr auto my_hash = cthash::simple<cthash::sha3_256>("hello there!");

static_assert(my_hash == "c7fd85f649fba4bd6fb605038ae8530cf2239152bbbcb9d91d260cc2a90a9fea"_sha3_256);
```

SHAKE128 (d=1024) (with compile time set `d` bits):
```c++
using namespace cthash::literals;

constexpr auto my_hash = cthash::shake128{}.update("hello there!").final<1024>();

static_assert(my_hash == "86089a77e15628597e45caf70c8ef271def6775c54d42d61fb45b9cd6d3b288e5fbd0042241a4aa9180c1bfe94542e16765b3a48d549771202e50aebf8d4f51bd00be2a427f81b7b58aaebc97f89559bca1ea21fec5047de70d075e14e5a3c95c002fd9f81925672d408d4b60c0105e5858df25b64af9b20cec973d66616da81"_shake128);
```

Also look at [runtime example](example.cpp).

### Including library

You can include specific hash function only by `#include <cthash/sha2/sha256.hpp>` or you can include whole library by `#include <cthash/cthash.hpp>`

#### Specific include for SHA-512/t

Just include `#include <cthash/sha2/sha512/t.hpp>`.

## Implementation note

There is no allocation at all, everything is done as a value type from user's perspective. No explicit optimizations were done (for now).

## Compiler support

You need a C++20 compiler.

* Clang 15.0.7+
* GCC 12.2+
