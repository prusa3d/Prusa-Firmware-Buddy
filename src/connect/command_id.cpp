#include "command_id.hpp"

using http::HeaderName;

namespace connect_client {

void ExtractCommanId::character(char c, HeaderName name) {
    if (name == HeaderName::CommandId) {
        if (!command_id.has_value()) {
            command_id = 0;
        }
        *command_id = 10 * *command_id + (c - '0');
    }
    // Ignoring other headers. That's kind of the point of the whole ExtraHeader exercise.
}

} // namespace connect_client
