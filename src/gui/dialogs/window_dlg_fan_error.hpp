//window_dlg_fan_error.hpp

#pragma once

#include "IDialog.hpp"
#include "window_text.hpp"
#include "i18n.h"

//Singleton dialog for messages
class window_dlg_fan_error_t : public AddSuperWindow<IDialog> {
    static constexpr const char *text_en = N_("A very important text telling user what to do ... ");
    window_text_t text;

    window_dlg_fan_error_t();
    window_dlg_fan_error_t(const window_dlg_fan_error_t &) = delete;

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

public:
    static void Show();
};
