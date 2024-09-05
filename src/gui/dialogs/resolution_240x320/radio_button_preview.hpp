/**
 * @file radio_button_preview.hpp
 */

#pragma once
#include "radio_button_fsm.hpp"

/**
 * @brief Radio button bound to fsm with vertical alignment (for print preview screen)
 * Unlike normal radio button it does not store responses but fsm phase
 * responses are generated from it at run time
 * this behavior allows to handle click automatically
 */
class RadioButtonPreview : public RadioButtonFSM {
public:
    /**
     * @brief Construct a new Radio Button object
     *
     * @param parent window containing this object
     * @param rect   rectangle enclosing all buttons
     */
    RadioButtonPreview(window_t *parent, Rect16 rect);
};
