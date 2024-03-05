#include "general_response.hpp"

#include <cstring>

#define R(NAME) \
    std::make_pair(Response::NAME, #NAME)

constexpr std::pair<Response, const char *> response_str[] = {
    R(Abort),
    R(Abort_invalidate_test),
    R(ABS),
    R(Adjust),
    R(All),
    R(ASA),
    R(Back),
    R(Cancel),
    R(Change),
    R(Continue),
    R(Cooldown),
    R(Disable),
    R(Done),
    R(Filament),
    R(Filament_removed),
    R(Finish),
    R(FLEX),
    R(FS_disable),
    R(NozzleType_HighFlow),
    R(NozzleType_Normal),
    R(HIPS),
    R(Ignore),
    R(Left),
    R(Load),
    R(MMU_disable),
    R(Never),
    R(Next),
    R(No),
    R(NotNow),
    R(NozzleDiameter_04),
    R(NozzleDiameter_06),
    R(Ok),
    R(Pause),
    R(PC),
    R(PETG),
    R(PLA),
    R(PP),
    R(Print),
    R(HotendType_Stock),
    R(HotendType_E3DRevo),
    R(HotendType_StockWithSock),
    R(Purge_more),
    R(PVB),
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
    R(PA),
    R(Postpone5Days),
    R(PRINT),
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
