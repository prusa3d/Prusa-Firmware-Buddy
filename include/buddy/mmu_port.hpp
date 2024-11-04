#pragma once

#include <device/board.h>

static_assert(BOARD_IS_XBUDDY(), "Only viable for xBuddy boards. There is no need to include this anywhere else");

/// Basic helper module for xBuddy MMU port that helps with safely turning the power on and correctly reseting the connected device based on xBuddy revision.
namespace mmu_port {

/// Depending on what version of buddy board we are running, we need to reconfigure the reset pin to function correctly
void setup_reset_pin();

/// Does bitbanging for older board revisions to safely turn on the power to connected device (unless connected device is powered externaly)
void power_on();

/// Just for parity, but just simply turns the power off
void power_off();

/// Activates reset pin correctly based on the xBuddy revision
void activate_reset();

/// Deactivates reset pin correctly based on the xBuddy revision
void deactivate_reset();

} // namespace mmu_port
