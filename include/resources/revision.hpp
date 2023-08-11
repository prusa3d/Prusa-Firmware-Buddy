#pragma once
#include <array>
#include <cstdint>
#include "resources/hash.hpp"

namespace buddy::resources {

struct Revision {
    /// SHA256 hash representing the content of the resources folder
    buddy::resources::Hash hash;
};

inline bool operator==(const Revision &lhs, const Revision &rhs) {
    return lhs.hash == rhs.hash;
}

inline bool operator!=(const Revision &lhs, const Revision &rhs) {
    return !(lhs == rhs);
}

struct InstalledRevision {
    /// File path at which we store a file with hash of the currently installed resources
    constexpr static const char *file_path = "/internal/resources_revision";

    /// Fetch revision of the currently installed resources
    static bool fetch(Revision &revision);

    /// Clear the revision of the currently installed resources
    static void clear();

    /// Set the revision of the currently installed resources
    static bool set(const Revision &revision);
};

}; // namespace buddy::resources
