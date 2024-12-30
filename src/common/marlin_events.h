// marlin_events.h
#pragma once

#include <cstdint>
#include <limits>
#include <utils/utility_extensions.hpp>

namespace marlin_server {

enum class Event : uint8_t {
    // Marlin events - UIAPI
    PrinterKilled, // onPrinterKilled(PGM_P const msg)
    MediaInserted, // onMediaInserted();
    MediaError, // onMediaError();
    MediaRemoved, // onMediaRemoved();
    PlayTone, // onPlayTone(const uint16_t frequency, const uint16_t duration)
    PrintTimerStarted, // onPrintTimerStarted()
    PrintTimerPaused, // onPrintTimerPaused()
    PrintTimerStopped, // onPrintTimerStopped()
    FilamentRunout, // onFilamentRunout()
    UserConfirmRequired, // onUserConfirmRequired(const char * const msg)
    StatusChanged, // onStatusChanged(const char * const msg)
    FactoryReset, // onFactoryReset()
    LoadSettings, // onLoadSettings()
    StoreSettings, // onStoreSettings()
    MeshUpdate, // onMeshUpdate(const uint8_t xpos, const uint8_t ypos, const float zval)
    // Marlin events - other
    CommandBegin, //
    CommandEnd, //
    Message, //
    RequestCalibrationsScreen, //
    Acknowledge, // onAcknowledge - lowest priority
    NotAcknowledge, // onNotAcknowledge - lowest priority

    _last = NotAcknowledge,
    _count
};

using EventMask = uint64_t;

static_assert(sizeof(EventMask) * 8 > ftrstd::to_underlying(Event::_last));

// event masks
constexpr EventMask make_mask(Event id) {
    return static_cast<EventMask>(1) << ftrstd::to_underlying(id);
}

inline constexpr EventMask EVENT_MSK_ALL = std::numeric_limits<EventMask>::max();
inline constexpr EventMask EVENT_MSK_DEF = EVENT_MSK_ALL & ~make_mask(Event::PrinterKilled);

// commands
enum class Cmd : uint32_t {
    NONE = 0,
    G = (uint32_t)'G' << 16,
    M = (uint32_t)'M' << 16,
    G28 = G + 28,
    G29 = G + 29,
    M109 = M + 109,
    M190 = M + 190,
    M303 = M + 303,
    M600 = M + 600,
    M701 = M + 701,
    M702 = M + 702,
    M876 = M + 876,
};

/**
 * @brief Parameter to start printing.
 * Tells whether to skip parts of preview when printing is started.
 * @warning The order matters. Element skips its screen and all screens with lower number.
 */
enum class PreviewSkipIfAble : uint8_t {
    no = 0, ///< Show all
    preview, ///< Skip preview thumbnail
    tool_mapping, ///< Skip preview thumbnail and toolmapping
    _count,
    _last = _count - 1,
    all = _last, ///< Skip all
};

} // namespace marlin_server
