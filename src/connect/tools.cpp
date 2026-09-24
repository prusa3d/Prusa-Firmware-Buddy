#include "tools.hpp"

#include <option/has_toolchanger.h>
#include <utils/string_builder.hpp>

namespace connect_client {

namespace {

    // Each index is rendered as a single digit, so the longest id is "d.d"
    static_assert(PhysicalToolIndex::count <= 10 && VirtualToolIndex::count <= 10);
    static_assert(ConnectToolIndex::capacity() >= sizeof("0.0"));

    char to_digit(uint8_t raw_index) {
        return static_cast<char>('0' + raw_index);
    }

} // namespace

ConnectToolIndex to_connect_tool(PhysicalToolIndex tool) {
    ConnectToolIndex result;
    StringBuilder::from_ptr(result.data(), result.capacity()).append_char(to_digit(tool.to_raw()));
    return result;
}

ConnectToolIndex to_connect_tool(VirtualToolIndex tool, [[maybe_unused]] ToConnectToolArgs args) {
#if HAS_MMU2()
    static_assert(!HAS_TOOLCHANGER());
    // A single head: its id is always "0" and the slot index equals the virtual index
    static_assert(PhysicalToolIndex::count == 1);

    // A nonzero index without the MMU means the caller sampled the tool set and the MMU state
    // at different times (the MMU was switched while a response was being built). That is reachable
    // by the user and must not BSOD - keep the slot naming, the next INFO corrects it.
    if (args.mmu_enabled || tool.to_raw() != 0) {
        ConnectToolIndex result;
        StringBuilder::from_ptr(result.data(), result.capacity())
            .append_string("0.")
            .append_char(to_digit(tool.to_raw()));
        return result;
    }
#else
    // Toolchangers and single-tool printers map virtual tools 1:1 to the heads
    static_assert(VirtualToolIndex::count == PhysicalToolIndex::count);
#endif

    return to_connect_tool(tool.to_physical());
}

} // namespace connect_client
