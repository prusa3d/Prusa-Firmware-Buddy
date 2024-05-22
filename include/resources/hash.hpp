#pragma once
#include <array>
#include <cstdint>

namespace buddy::resources {

using Hash = std::array<uint8_t, 32>;

bool calculate_directory_hash(Hash &hash, const char *root);

} // namespace buddy::resources
