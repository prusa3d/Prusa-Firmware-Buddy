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

    static constexpr const uint16_t vertical_buttons_width = 94; // Needs to be at least 94px for translations to fit to labels

private:
    Rect16 getVerticalIconRect(uint8_t idx) const;
    Rect16 getVerticalLabelRect(uint8_t idx) const;

protected:
    virtual void unconditionalDraw() override;
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;
};
