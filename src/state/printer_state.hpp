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
    PrintPreviewQuestions = 3101,
    PowerpanicColdBed = 3102,
    CrashRecoveryAxisNok = 3103,
    CrashRecoveryRepeatedCrash = 3104,
    CrashRecoveryHomeFail = 3105,
    CrashRecoveryToolPickup = 3106,
    PrintPreviewToolsMapping = 3107,
    FilamentRunout = 3108,
    MMULoadUnloadError = 3109,
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
