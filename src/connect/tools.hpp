/// @file
#pragma once

#include <option/has_mmu2.h>
#include <tool_index.hpp>
#include <utils/string/inplace_string.hpp>

#include <cstddef>
#include <variant>

namespace connect_client {

/// Tool identifier in the Connect "Prusa Architecture" tool protocol.
/// Print heads are "<physical>", feeder slots of a head with an MMU are "<physical>.<slot>".
using ConnectToolIndex = InplaceString<4>;

/// Printer state the Connect tool ids depend on.
/// The response is rendered in chunks, so the caller samples this once per Connect response and
/// passes the same value to every to_connect_tool() call of that response - a change in between
/// would produce an inconsistent tool structure.
struct ToConnectToolArgs {
#if HAS_MMU2()
    /// The MMU can be switched on/off at runtime. When off, virtual tool 0 is the print head itself.
    bool mmu_enabled : 1;
#endif
};

/// A tool as Connect addresses it: a print head, or a slot of the feeder attached to one
using ConnectTool = std::variant<PhysicalToolIndex, VirtualToolIndex>;

/// @returns how many tools Connect sees: all the print heads, followed by the feeder slots when there is a feeder
[[nodiscard]] constexpr size_t connect_tool_count([[maybe_unused]] ToConnectToolArgs args) {
#if HAS_MMU2()
    if (args.mmu_enabled) {
        return PhysicalToolIndex::count + VirtualToolIndex::count;
    }
#endif
    return PhysicalToolIndex::count;
}

/// @returns the tool at @p index in the sequence connect_tool_count() counts, 0 <= index < connect_tool_count()
/// A plain index lets the chunked JSON renderer keep its place between the chunks.
[[nodiscard]] constexpr ConnectTool connect_tool(size_t index) {
    if (index < PhysicalToolIndex::count) {
        return PhysicalToolIndex::from_raw(index);
    }
    return VirtualToolIndex::from_raw(index - PhysicalToolIndex::count);
}

[[nodiscard]] ConnectToolIndex to_connect_tool(PhysicalToolIndex tool);

[[nodiscard]] ConnectToolIndex to_connect_tool(VirtualToolIndex tool, ToConnectToolArgs args);

} // namespace connect_client
