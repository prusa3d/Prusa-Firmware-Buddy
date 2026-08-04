#include <nfcv/commands.hpp>

namespace nfcv {

bool is_response_expected(const Command &command) {
    return std::visit([]<typename T>(const T &) -> bool { return requires { typename T::Response; }; }, command);
}

bool is_write_like_command(const Command &command) {
    return std::visit([](const auto &cmd) {
        if constexpr (requires { cmd.is_write_alike; }) {
            return cmd.is_write_alike;
        } else {
            return false;
        }
    },
        command);
}

std::byte command_id(const Command &command) {
    return std::visit([]<typename T>(const T &) { return T::cmd_id; }, command);
}

} // namespace nfcv
