#include "marlin_client_mock.h"

#include <gcode/inject_queue_actions.hpp>
#include <common/filepath_operation.h>
#include <marlin_events.h>
#include <tool_index.hpp>
#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>

namespace marlin_client {

std::string last_gcode;

void gcode_printf(const char *format, ...) {
    char buf[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    last_gcode = buf;
}

void inject(InjectQueueRecord record) {
    char buf[128];
    std::visit([&]<typename T>(const T &val) {
        if constexpr (std::is_same_v<T, GCodeLiteral>) {
            snprintf(buf, sizeof(buf), val.gcode, (double)val.parameter);
        } else {
            throw;
        }
    },
        record);
    last_gcode = buf;
}

void print_start(const char *, marlin_server::PreviewSkipIfAble, marlin_server::ResetToolMapping) {}

} // namespace marlin_client

bool f_gcode_get_next_comment_assignment(FILE *, char *, int, char *, int) {
    return false;
}

bool random32bit(uint32_t *output) {
    *output = random();
    return true;
}

template <>
bool VirtualToolIndex::is_enabled() const {
    return true;
}

extern "C" {
void notify_reconfigure() {}

void netdev_get_hostname(uint32_t netdev_id, char *buffer, size_t buffer_len) {}
}
