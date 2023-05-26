#ifndef CTHASH_CTHASH_HPP
#define CTHASH_CTHASH_HPP

// SHA-2 family
#include "sha2/sha224.hpp"
#include "sha2/sha256.hpp"
#include "sha2/sha384.hpp"
#include "sha2/sha512.hpp"
#include "sha2/sha512/t.hpp"

// SHA-3 (keccak) family
#include "sha3/sha3-224.hpp"
#include "sha3/sha3-256.hpp"
#include "sha3/sha3-384.hpp"
#include "sha3/sha3-512.hpp"
#include "sha3/shake128.hpp"
#include "sha3/shake256.hpp"

// xxhash (non-crypto fast hash)
#include "xxhash.hpp"

#endif
