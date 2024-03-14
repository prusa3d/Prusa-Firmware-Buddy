/**
 * @file radio_button_preview.cpp
 */

#include "radio_button_preview.hpp"
#include "i18n.h"

RadioButtonPreview::RadioButtonPreview(window_t *parent, Rect16 rect)
    : AddSuperWindow<RadioButtonFsm<PhasesPrintPreview>>(parent, rect, PhasesPrintPreview::main_dialog) {
}
