/// @file
#pragma once

#include <cstdint>
#include <variant>
#include <optional>

#include <inc/MarlinConfigPre.h>
#include <utils/array_extensions.hpp>
#include <utils/variant_utils.hpp>
#include <utils/storage/strong_index_array.hpp>
#include <bsod/bsod.h>
#include <utils/overloaded_visitor.hpp>
#include <printers.h>
#include <string_view_utf8.hpp>
#include <utils/string_builder.hpp>
#include <utils/string/inplace_string.hpp>

#include <option/has_tool_mapping.h>
#if HAS_TOOL_MAPPING()
class ToolMapper;
#endif

#include "tool_index_iterator.hpp"

// Use XXXToolIndex::count instead
// This is only necessary to prevent dependency loops within this header
namespace tool_index_private {
#if EXTRUDERS > 1
// EXTRUDERS - 1 index represents NoTool picked
static_assert(EXTRUDERS > 2);
inline constexpr uint8_t virtual_tool_index_count = EXTRUDERS - 1;
#else
inline constexpr uint8_t virtual_tool_index_count = EXTRUDERS;
#endif

#if HOTENDS > 1
// HOTENDS - 1 index represents NoTool picked
static_assert(HOTENDS > 2);
inline constexpr uint8_t physical_tool_index_count = HOTENDS - 1;
#else
inline constexpr uint8_t physical_tool_index_count = HOTENDS;
#endif
}; // namespace tool_index_private

template <const int count_, typename Extension>
struct ToolIndex;

struct PhysicalToolIndexExtension;

/// Strong type for indexing physical tools.
/// Physical tool is a single "toolhead" a printer has.
/// This more or less corresponds with the old marlin HOTENDS macro.
/// Each toolhead has properties like hotend temperature, nozzle offset, ...
/// - XL has (up to) 5 physical tools
/// - INDX has a lot if physical tools (even though only one nozzle can be heated at a time)
/// - MINI, MKx, C1 (non-index) have a single physical tool
using PhysicalToolIndex = ToolIndex<tool_index_private::physical_tool_index_count, PhysicalToolIndexExtension>;

struct VirtualToolIndexExtension;

/// Strong type for indexing "virtual" tools.
/// Virtual tools extend the physical tools with the possibility of filament multiplexing,
/// where multiple virtual tools can be mapped to a single physical tool and swapped through multiplexing (MMU).
/// This more or less corresponds with the old marlin EXTRUDERS macro.
/// Used virtual tools are surjectively mapped to physical tools.
using VirtualToolIndex = ToolIndex<tool_index_private::virtual_tool_index_count, VirtualToolIndexExtension>;

struct GcodeToolIndexExtension;

/// Strong type for indexing virtual tools before toolmapping.
/// This type corresponds to the tool indexes as they are used in the gcode.
/// These indexes are then mapped to VirtualToolIndex, based on the current tool mapping configuration.
/// There is a bijection relation between the active virtual tools and gcode tools,
/// (spool join kind of extends this to surjection, but at any single moment, a GCodeTool is always mapped to a single VirtualTool)
using GcodeToolIndex = ToolIndex<tool_index_private::virtual_tool_index_count, GcodeToolIndexExtension>;

/// Converting between tool index types
/// ===================================
/// The mapping between gcode, virtual and physical tools is NOT a plain
/// reinterpretation of the raw number - it depends on the printer and on the
/// runtime configuration (MMU multiplexing, tool mapping, spool join, ...).
/// Reinterpreting the raw value (`PhysicalToolIndex::from_raw(virtual.to_raw())`)
/// silently produces wrong/non-existent tools wherever the mapping isn't 1:1
/// (e.g. physical tool 4 on an MK4 with MMU). Always convert through:
///   - GcodeToolIndex   -> VirtualToolIndex  : GcodeToolIndex::to_virtual()
///   - GcodeToolIndex   -> PhysicalToolIndex : GcodeToolIndex::to_physical()
///   - VirtualToolIndex -> PhysicalToolIndex : VirtualToolIndex::to_physical()
///   - PhysicalToolIndex -> VirtualToolIndex : PhysicalToolIndex::currently_selected_virtual_tool()
///   - whole variants                        : to_physical_tool_index<...>(variant)
/// from_raw()/to_raw() only bridge to/from serialized data (gcode, EEPROM, ...)
/// and must stay within a single index type.

/// Thread safety
/// =============
/// All functions and operations in this module have to be thread-safe unless documented otherwise.

/// Strong type for representing no tool using `std::variant<SomeToolIndex, NoTool>`
/// This type means a VALID situation where the toolchanger has no tool picked
struct NoTool {
    inline constexpr bool operator==(const NoTool &) const {
        return true;
    }
};

/// Strong type for representing all tools using `std::variant<SomeToolIndex, AllTools>`
struct AllTools {
    inline constexpr bool operator==(const AllTools &) const {
        return true;
    }
};

/// Strong type for representing when a tool is not mapped, like `std::variant<SomeToolIndex, ToolNotMapped>`
struct ToolNotMapped {
    inline constexpr bool operator==(const ToolNotMapped &) const {
        return true;
    }
};

/// Strong type for representing a currently selected tool
struct CurrentlySelectedTool {
    inline constexpr bool operator==(const CurrentlySelectedTool &) const {
        return true;
    }
};

/// Strong base type for indexing tools, providing common functionality between PhysicalToolIndex, VirtualToolIndex and GcodeToolIndex
template <const int count_, typename Extension>
struct ToolIndex : public Extension {

public:
    using Iterator = ToolIndexIterator<ToolIndex>;

    constexpr inline ToolIndex(const ToolIndex &) = default;

    /// Creates ToolIndex from raw uint8_t
    /// Use `from_raw_notool` instead, if you are not sure that raw index represent only valid tool
    /// @param index must be less than ToolIndex::count
    /// @note Try to replace raw index with ToolIndex for better safety
    /// @warning Do NOT build one index type from another's to_raw() (e.g.
    ///   `PhysicalToolIndex::from_raw(virtual.to_raw())`). That reinterprets the
    ///   number without the proper mapping. Use the conversion routines documented
    ///   above the type definitions instead.
    static inline constexpr ToolIndex from_raw(uint8_t index) {
        return ToolIndex(index);
    }

    /// @returns raw index as uint8_t
    /// @note Try to replace raw index with ToolIndex for better safety
    /// @warning The raw value is meaningful only together with its index type.
    ///   Do not feed it into another type's from_raw() to convert - see the
    ///   conversion routines documented above the type definitions.
    inline constexpr uint8_t to_raw() const { return this->value; }

    /// @returns Index of the tool, starting from 1, for display purposes
    inline constexpr uint8_t display_index() const {
        return to_raw() + 1;
    }

    using DisplayNameParams = StringViewUtf8Parameters<4>;

    /// @returns whether the specified tool is enabled
    /// Disabled tools cannot be selected and used for printing
    bool is_enabled() const;

    /// @returns display name of the tool - something like "Slot 1" or "Tool 1"
    string_view_utf8 display_name(StringViewUtf8ParamBase &params) const;

    static_assert(count_ < 100);
    using CompactDisplayName = InplaceString<4>;

    /// @returns compact display name of the tool - something like "F1" or "T1"
    CompactDisplayName compact_display_name() const {
        CompactDisplayName result;
        auto sb = StringBuilder::from_ptr(result.data(), result.capacity());
        sb.append_printf("%c%i", Extension::compact_name_prefix_char, display_index());
        return result;
    }

    /// Allow simpler conversion to integer when using StrongIndexArray
    /// @note pass-by-reference is needed to avoid circular dependencies
    static inline constexpr std::size_t to_raw_static(const ToolIndex &tool_index) {
        return tool_index.to_raw();
    }

    /// Maximum number of tools the firmware supports
    static constexpr uint8_t count = count_;

    /// @returns the single tool that is enabled, if there is just one tool enabled
    static std::optional<ToolIndex> single_enabled_tool() {
        const auto tool = all().skip_all_disabled();
        if (tool.at_end()) {
            // Not a single tool enabled
            return std::nullopt;
        }

        auto next = tool;
        ++next;
        if (!next.at_end()) {
            // Multiple tools enabled
            return std::nullopt;
        }

        return *tool;
    }

    /// @returns index of the last enabled tool + 1
    /// Useful for determining how many tool items display in the UI
    static uint8_t enabled_range_size() {
        uint8_t result = ToolIndex::count;
        while (result > 0 && !ToolIndex::from_raw(result - 1).is_enabled()) {
            result--;
        }
        return result;
    }

    /// List of all tools the printer offers, for `for()` loops
    static consteval Iterator all() {
        return Iterator::make_all();
    }

    inline constexpr bool operator==(const ToolIndex &other) const {
        return value == other.value;
    }

private:
    inline explicit constexpr ToolIndex(uint8_t index)
        : value(index) {
        if (index >= count) {
            bsod("ToolIndex out of range");
        }
    }

private:
    uint8_t value;
};

struct PhysicalToolIndexExtension {
    static constexpr char compact_name_prefix_char = 'T';

    /// @returns virtual tool currently assigned to @p this physical tool
    std::variant<VirtualToolIndex, NoTool> currently_selected_virtual_tool() const;

    /// Builds details about the virtual tool (things like nozzle diameter, ...)
    void build_details(StringBuilder &sb) const;

    /// Checks for legacy values representing no tool
    /// Use `from_raw` instead, if you are sure that raw index represent only valid tool
    /// @param index
    [[deprecated("Should be removed after removing all special (notool) values in raw indices")]]
    static std::variant<PhysicalToolIndex, NoTool> from_raw_notool(uint8_t index);

    /// @returns currently picked physical tool
    /// @note There can be a currrently_active() physical tool without having any currently_active() virtual one.
    /// @note That would for example mean that a tool is picked but a MMU for that tool does not have any slot loaded in.
    static std::variant<PhysicalToolIndex, NoTool> currently_selected();

    static std::optional<PhysicalToolIndex> currently_selected_opt() {
        return stdext::get_optional<PhysicalToolIndex>(currently_selected());
    }
};

struct VirtualToolIndexExtension {
    // For multi-tool printers with 1:1 mapping between virtual and physica tools, introducing some extra slots/indexing would just confuse the users.
    // So in that case, refer to the virtual tools same as to the physical ones, because they're are 1:1.
    static constexpr char compact_name_prefix_char = //
        (tool_index_private::physical_tool_index_count == tool_index_private::virtual_tool_index_count)
        ? PhysicalToolIndexExtension::compact_name_prefix_char
        : 'F';

    PhysicalToolIndex to_physical() const;

    /// Builds details about the virtual tool (things like loaded filament, nozzle diameter, ...)
    void build_details(StringBuilder &sb) const;

    /// Checks for legacy values representing no tool
    /// Use `from_raw` instead, if you are sure that raw index represent only valid tool
    /// @param index
    [[deprecated("Should be removed after removing all special (notool) values in raw indices")]]
    static std::variant<VirtualToolIndex, NoTool> from_raw_notool(uint8_t index);

    /// @returns currently active virtual tool
    /// Active = the corresponding physical tool is active and has the resulting virtual tool selected
    static std::variant<VirtualToolIndex, NoTool> currently_selected();

    static std::optional<VirtualToolIndex> currently_selected_opt() {
        return stdext::get_optional<VirtualToolIndex>(currently_selected());
    }
};

struct GcodeToolIndexExtension {
    static constexpr char compact_name_prefix_char = 'G';

    /// @returns VirtualToolIndex corresponding to the GCodeToolIndex, if there is any
    /// Will return NoTool if no tool is mapped
    std::variant<VirtualToolIndex, ToolNotMapped> to_virtual() const;

#if HAS_TOOL_MAPPING()
    /// Override allowing using non-default tool mapper
    std::variant<VirtualToolIndex, ToolNotMapped> to_virtual(const ToolMapper &tool_mapper) const;
#endif

    /// @returns PhysicalToolIndex corresponding to the GCodeToolIndex, if there is any
    /// Will return NoTool if no tool is mapped
    std::variant<PhysicalToolIndex, ToolNotMapped> to_physical() const;
};

/// @returns a variant where all the types are mapped to PhysicalToolIndex alternatives
/// NoTool and AllTools are unchanged
/// VirtualToolIndex and GCodeToolIndex are mapped using to_physical()
/// @example to_physical_tool_index<NoTool>(std::variant<VirtualToolIndex, NoTool>(VirtualToolIndex::from_raw(0))) -> std::variant<PhysicalToolIndex, NoTool>
template <typename... Variants, typename... T>
auto to_physical_tool_index(const std::variant<T...> &variant) {
    // Note: cannot use match() here because not all overloads would necessarily be in the variant
    static constexpr auto f = []<typename V>(V val) -> std::variant<PhysicalToolIndex, Variants...> {
        if constexpr (std::is_same_v<V, GcodeToolIndex> || std::is_same_v<V, VirtualToolIndex>) {
            return val.to_physical();

        } else if constexpr (std::is_same_v<V, NoTool> || std::is_same_v<V, ToolNotMapped> || std::is_same_v<V, AllTools>) {
            return val;

        } else {
            static_assert(false);
        }
    };
    return std::visit(f, variant);
}

/// Resolves the provided variant (presumably including CurrentlySelectedTool) into a std::optional
template <typename Index, typename... Variants>
std::optional<Index> resolve_tool_index(const std::variant<Index, Variants...> &variant) {
    using Result = std::optional<Index>;
    return match(
        variant, //
        [](Index tool) -> Result { return tool; }, //
        [](CurrentlySelectedTool) -> Result {
            return match(
                Index::currently_selected(), //
                [](Index tool) -> Result { return tool; }, //
                [](NoTool) -> Result { return std::nullopt; } //
            ); //
        },
        [](NoTool) -> Result { return std::nullopt; } //
    );
}

/// Same as resolve_tool_index, but also returns std::nullopt if the tool is disabled
template <typename Index, typename... Variants>
std::optional<Index> resolve_enabled_tool_index(const std::variant<Index, Variants...> &variant) {
    const auto r = resolve_tool_index(variant);
    if (r.has_value() && r->is_enabled()) {
        return r;
    } else {
        return std::nullopt;
    }
}

/// @returns range of tools represented by the variant
/// NoTool returns an empty range
/// AllTools returns a range of all tools
/// Tool returns a single item range
template <typename Index, typename... Variants>
auto tool_index_iterator(const std::variant<Index, Variants...> &variant) {
    using Iterator = Index::Iterator;
    return match(
        variant,
        [](Index i) { return Iterator::make_single(i); },
        [](NoTool) { return Iterator::make_empty(); },
        [](AllTools) { return Iterator::make_all(); });
}
