#include "general_response.hpp"

#include <cstring>
#include <utils/enum_array.hpp>

#define R(NAME) \
    { Response::NAME, #NAME }

static constexpr EnumArray<Response, const char *, Response::_count> response_str {
    { Response::_none, "" },
    R(Abort),
    R(Abort_invalidate_test),
    R(Adjust),
    R(All),
    R(Always),
    R(Back),
    R(Calibrate),
    R(Cancel),
    R(Help),
    R(Change),
    R(Continue),
    R(Cooldown),
    R(Details),
    R(Disable),
    R(Done),
    R(Filament),
    R(Filament_removed),
    R(Finish),
    R(FS_disable),
    R(Ignore),
    R(Left),
    R(Load),
    R(MMU_disable),
    R(Never),
    R(Next),
    R(No),
    R(NotNow),
    R(Ok),
    R(Pause),
    R(Print),
    R(Purge_more),
    R(Quit),
    R(Reheat),
    R(Replace),
    R(Remap),
    R(Remove),
    R(Restart),
    R(Resume),
    R(Retry),
    R(Right),
    R(Skip),
    R(Slowly),
    R(SpoolJoin),
    R(Stop),
    R(Unload),
    R(Yes),
    R(Heatup),
    R(Postpone5Days),
    R(Docks4),
    R(Docks8),
    R(Other),
    R(Empty),
};

#undef R

Response from_str(std::string_view str) {
    for (int i = 0; i < static_cast<int>(Response::_count); i++) {
        if (str == response_str[i]) {
            return static_cast<Response>(i);
        }
    }
    return Response::_none;
}

const char *to_str(const Response response) {
    return response_str.get_fallback(response, Response::_none);
}
