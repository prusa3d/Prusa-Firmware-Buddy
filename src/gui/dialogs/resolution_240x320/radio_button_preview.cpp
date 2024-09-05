/**
 * @file radio_button_preview.cpp
 */

#include "radio_button_preview.hpp"
#include "i18n.h"

RadioButtonPreview::RadioButtonPreview(window_t *parent, Rect16 rect)
    : RadioButtonFSM(parent, rect, PhasesPrintPreview::main_dialog) {
}
