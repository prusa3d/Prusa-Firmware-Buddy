#pragma once
#include "crash_dump_distribute.hpp"
#include <device/board.h>

namespace crash_dump {
inline constexpr const char *buddy_dump_usb_path { "/usb/dump_buddy.bin" };
void upload_buddy_dump_to_server();

struct DumpHandler {
    bool (*presence_check)();
    void (*usb_save)();
    void (*server_upload)();
    void (*remove)();
};

inline constexpr auto dump_handlers { std::to_array<DumpHandler>({
    {
        .presence_check = []() { return static_cast<bool>(dump_in_xflash_is_valid()); },
        .usb_save = []() { dump_save_to_usb(buddy_dump_usb_path); },
        .server_upload = []() { upload_buddy_dump_to_server(); },
        .remove = []() { dump_in_xflash_reset(); },
    },
}) };

inline constexpr auto dump_handlers_have_valid_pointers { []() {
    for (const auto &handle : dump_handlers) {
        if (!handle.presence_check || !handle.usb_save || !handle.server_upload || !handle.remove) {
            return false;
        }
    }
    return true;
}() };
static_assert(dump_handlers_have_valid_pointers, "dump handlers must have valid function pointers");

using BufferT = std::array<const DumpHandler *, dump_handlers.size()>;

/**
     * @brief Gets the _present_dumps.
     *
     * @param buffer Buffer that will be used to store present dumps
     * @return Span into the buffer with present dumps
     */
std::span<const DumpHandler *> get_present_dumps(BufferT &buffer);

}
