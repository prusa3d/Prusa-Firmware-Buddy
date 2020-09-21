// liveadjustz.hpp
#pragma once

#include "IDialog.hpp"
#include "DialogRadioButton.hpp"
#include "window_text.hpp"
#include "window_icon.hpp"
#include "../../lang/i18n.h"
#include "client_response.hpp"

class LiveAdjustZ : public IDialog {
protected:
    window_text_t text;
    window_text_t number;
    window_icon_t nozzle_icon;
    window_frame_t bed;

    Response result;

public:
    is_closed_on_click_t closed_outside = is_closed_on_click_t::yes;

    LiveAdjustZ(Rect16 rect, is_closed_on_click_t outside_close);
    Response GetResult();
    void saveAndClose();

private:
    float z_offset;
    char numString[7];

protected:
    void Step(int diff);
    bool Change(int dif);
		void writeZoffsetLabel();

    Rect16 getTextRect();
    Rect16 getNumberRect();

    virtual void windowEvent(window_t *sender, uint8_t event, void *param) override;
};

Response LiveAdjustZOpen(Rect16 rect = GuiDefaults::RectScreenBody, is_closed_on_click_t outside_close = is_closed_on_click_t::yes);

