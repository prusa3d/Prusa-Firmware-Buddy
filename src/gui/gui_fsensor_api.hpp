/**
 * @file gui_fsensor_api.hpp
 * @brief
 */

#pragma once

namespace GuiFSensor {

// HOT FIX - TODO(InputShaperBeta)
bool is_calib_dialog_open();

// do not call directly
// msgbox calls guiloop and it would open another msgbox
// use validate_for_cyclical_calls() instead
void validate();

bool validate_for_cyclical_calls();

} // namespace GuiFSensor
