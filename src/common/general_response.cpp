#include "general_response.hpp"

#include <cstring>

#define R(NAME) \
    std::make_pair(Response::NAME, #NAME)

constexpr std::pair<Response, const char *> response_str[] = {
    R(Abort),
    R(Abort_invalidate_test),
    R(Adjust),
    R(All),
    R(Back),
    R(Calibrate),
    R(Cancel),
    R(Help),
    R(Change),
    R(Continue),
    R(Cooldown),
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
    R(PRINT),
    R(Tool1),
    R(Tool2),
    R(Tool3),
    R(Tool4),
    R(Tool5),
};
// NOTE: -1 is to exclude the _count itself
static_assert(static_cast<uint32_t>(Response::_count) - 1 == std::size(response_str), "Handle all responses!");

Response from_str(std::string_view str) {
    for (auto pair : response_str) {
        if (str.length() == strlen(pair.second) && strncmp(str.data(), pair.second, str.length()) == 0) {
            return pair.first;
        }
    }
    return Response::_none;
}

const char *to_str(const Response response) {
    for (auto pair : response_str) {
        if (pair.first == response) {
            return pair.second;
        }
    }
    return "";
}
