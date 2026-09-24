/// @file
#pragma once

#include <option/has_mmu2.h>
#include <tool_index.hpp>
#include <utils/string/inplace_string.hpp>

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

[[nodiscard]] ConnectToolIndex to_connect_tool(PhysicalToolIndex tool);

[[nodiscard]] ConnectToolIndex to_connect_tool(VirtualToolIndex tool, ToConnectToolArgs args);

} // namespace connect_client
