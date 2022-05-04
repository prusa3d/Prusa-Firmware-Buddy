#pragma once
#include <tuple>
#include <optional>
#include <functional>

namespace buddy::bootloader {

struct Version {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
} __attribute__((packed));

/// Return version of installed bootloader.
Version get_version();

/// Return true if the bootloader needs to be updated/reflashed
bool needs_update();

enum class UpdateStage {
    LookingForBbf,
    PreparingUpdate,
    Updating,
};

using ProgressHook = std::function<void(int percent_done, UpdateStage stage)>;

/// Update the bootloader (if needed together with preboot) on the MCU
void update(ProgressHook progress);

};
