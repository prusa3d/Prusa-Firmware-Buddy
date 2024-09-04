/**
 * @file radio_button.hpp
 *
 * @brief RadioButton designed to be used in dialogs, it is bound to Response
 * created from array of responses and array of labels
 * responses are counted and stored into btn_count
 * if there is more labels than buttons, "additional buttons" are not accessible
 * if there is less labels than buttons, "remaining buttons" have no labels
 * has optional icons, in that case number of buttons is fixed to 3
 * in iconned mode can validate background
 */

#pragma once
#include "i_radio_button.hpp"

class RadioButton : public IRadioButton {
    Responses_t responses;
    const PhaseTexts *texts; // nullptr == autoset texts with default strings

public:
    /**
     * @brief Construct a new Radio Button object
     *
     * @param parent window containing this object
     * @param rect   rectangle enclosing all buttons
     */
    RadioButton(window_t *parent, Rect16 rect);

// TODO: REMOVEME completely BFW-6028
#if MAX_RESPONSES != 4
    /**
     * @brief Construct a new Radio Button object
     *
     * @param parent window containing this object
     * @param rect   rectangle enclosing all buttons
     * @param resp   array of responses bound to buttons, has response == buttons enabled, only first four are used, rest is discarded
     * @param labels array of button labels, if is set to nullptr, strings are assigned as default ones from given responses
     */
    RadioButton(window_t *parent, Rect16 rect, const PhaseResponses &resp, const PhaseTexts *labels = nullptr);
#endif

    /**
     * @brief Construct a new Radio Button object
     *
     * @param parent window containing this object
     * @param rect   rectangle enclosing all buttons
     * @param resp   array of responses bound to buttons, has response == buttons enabled
     * @param labels array of button labels, if is set to nullptr, strings are assigned as default ones from given responses
     */
    RadioButton(window_t *parent, Rect16 rect, Responses_t resp, const PhaseTexts *labels = nullptr);

    virtual std::optional<size_t> IndexFromResponse(Response btn) const override;

// TODO: REMOVEME completely BFW-6028
#if MAX_RESPONSES != 4
    void Change(const PhaseResponses &resp, const PhaseTexts *txts = nullptr); // nullptr generates texts automatically, only first four responses are used, rest is discarded
#endif

    void Change(Responses_t resp, const PhaseTexts *txts = nullptr); // nullptr generates texts automatically

protected:
    virtual Response responseFromIndex(size_t index) const override;
    virtual const PhaseTexts *getAlternativeTexts() const override { return texts; }
};
