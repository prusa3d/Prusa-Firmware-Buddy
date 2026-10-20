/// @file
#pragma once

#include <cstdint>
#include <optional>
#include <atomic>
#include <array>

#include <tool_index.hpp>
#include <bsod/bsod.h>
#include <utils/compact_optional.hpp>

#include <feature/openprinttag/detail/defines.hpp>

namespace buddy::openprinttag {

bool has_tool_openprinttag_reader(VirtualToolIndex tool);

struct TagUID {
    /// UID in the NFC-V wire order: LSB first (0xE0 is the last byte)
    std::array<uint8_t, 8> data;

    using Hash = uint16_t;
    Hash hash() const;
};

struct ToolTagField;

/// Class representing a specific tag associated with a specific tool
/// The value gets "invalidated" if a tag is removed from the tool
class ToolTag {

public:
    using UIDHash = TagUID::Hash;

    /// Used in config store
    static constexpr UIDHash no_tag_hash = 0;

    using UIDHashOptional = CompactOptional<UIDHash, no_tag_hash>;

public:
    /// @returns tag that is currently detected at the specified tool spool slot
    /// This can be a different tag than @p for_tool_assigned in case the user removes the assignes pool
    /// !!! NOT to be used for anything related to printing, use for_tool_assigned for that
    static std::optional<ToolTag> for_tool_ephemeral(VirtualToolIndex tool);

    /// @returns tag that is assigned to the specified tool
    /// This is a long-term assignment that happens during filament load, and is unassigned on unload
    static std::optional<ToolTag> for_tool_assigned(VirtualToolIndex tool);

public:
    constexpr explicit ToolTag(VirtualToolIndex tool, UIDHash uid_hash)
        : uid_hash_(uid_hash)
        , tool_(tool) {
        if (uid_hash_ == no_tag_hash) {
            bsod_unreachable();
        }
    }

    constexpr ToolTag(const ToolTag &) = default;

    constexpr inline VirtualToolIndex tool() const {
        return tool_;
    }

    constexpr inline UIDHash uid_hash() const {
        return uid_hash_;
    }

    /// @returns a struct representing a specific field on the tag
    template <typename F>
    constexpr inline ToolTagField field(F field) const;

    constexpr inline bool operator==(const ToolTag &) const = default;
    constexpr inline bool operator!=(const ToolTag &) const = default;

    constexpr ToolTag &operator=(const ToolTag &) = default;

private:
    friend class Manager;

    /// Hash of the tag UID (to reduce size)
    /// This ensures that we don't accidentally read/write something from a different tag (if tag for a tool changes suddenly)
    UIDHash uid_hash_;

    VirtualToolIndex tool_;

    // Explicit padding for constexpr copy-initialization support
    uint8_t _padding = 0;
};

struct ToolTagField {
    ToolTag tag;
    Section section;
    Field field;

    constexpr inline bool operator==(const ToolTagField &) const = default;
    constexpr inline bool operator!=(const ToolTagField &) const = default;
};

template <typename F>
constexpr inline ToolTagField ToolTag::field(F field) const {
    return ToolTagField {
        .tag = *this,
        .section = ::openprinttag::field_section(field),
        .field = static_cast<Field>(field),
    };
}

/// UID hash of a tool tag that is ready to be auto-assigned on autoload
/// Basically when the OPTTagDetected screen is displayed, inserting the filament should automatically link it to the tag
/// Yes, this is a hack, bridging GUI thread and Marlin thread
inline std::atomic<ToolTag::UIDHash> autoload_auto_assign_tool_tag_hash = ToolTag::no_tag_hash;

}; // namespace buddy::openprinttag
