/**
 * @file png_resources.hpp
 * @brief this file is generated ...
 */

// clang-format off

#pragma once
#include <cstdint>
#include <cstddef>
#include <cstdio>

namespace png {

struct Resource {
    FILE* file = nullptr; // default file
    const char* name = nullptr; // name is optional, external file might not need it
    size_t offset = 0; // 0 == no offset, all "normal" png files containing only one png has no offset
    size_t size = 0; // 0 == calculate at run time
    uint16_t w = 0; // 0 == calculate at run time
    uint16_t h = 0; // 0 == calculate at run time

    FILE* Get() const;
};

// ordered by width, height and alphabetically
enum class Id {
#include "png_id.gen"
};

static constexpr Resource resources[] {
#include "png_resources.gen"
};

// I did not use reference to support array of pointers to resource
template<Id ID>
constexpr const Resource* Get() {
    return &resources[size_t(ID)];
}

} // namespace png

// clang-format on
