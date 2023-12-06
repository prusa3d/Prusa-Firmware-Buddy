#pragma once

#include <tuple>
#include <optional>
#include <inttypes.h>

namespace printer_state {

enum class DeviceState {
    Unknown,
    Idle,
    Printing,
    Paused,
    Finished,
    Stopped,
    Ready,
    Busy,
    Attention,
    Error,
};

enum class AttentionCode {
    PrintPreviewUnfinishedSelftest = 3101,
    PrintPreviewNewFW = 3102,
    PrintPreviewWrongPrinter = 3103,
    PrintPreviewNoFilament = 3104,
    PrintPreviewWrongFilament = 3105,
    PrintPreviewMMUFilamentInserted = 3106,
    PrintPreviewFileError = 3107,
    PowerpanicColdBed = 3108,
    CrashRecoveryAxisNok = 3109,
    CrashRecoveryRepeatedCrash = 3110,
    CrashRecoveryHomeFail = 3111,
    CrashRecoveryToolPickup = 3112,
    PrintPreviewToolsMapping = 3113,
    FilamentRunout = 3114,
    MMULoadUnloadError = 3115,
};

struct StateWithAttentionCode {
    DeviceState device_state;
    std::optional<AttentionCode> attention_code = std::nullopt;
};

DeviceState get_state(bool ready = false);
StateWithAttentionCode get_state_with_attenion_code(bool ready = false);

bool remote_print_ready(bool preview_only);

bool has_job();

const char *to_str(DeviceState state);
const char *to_str(AttentionCode attention_code, char *buffer, size_t size);
} // namespace printer_state
