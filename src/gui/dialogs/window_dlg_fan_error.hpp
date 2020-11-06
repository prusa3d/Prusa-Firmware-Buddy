//window_dlg_fan_error.hpp

#pragma once

#include "IDialog.hpp"
#include "window_text.hpp"
#include "i18n.h"

//Singleton dialog for messages
class window_dlg_fan_error_t : public AddSuperWindow<IDialog> {
    window_text_t text;

    window_dlg_fan_error_t();
    window_dlg_fan_error_t(const window_dlg_fan_error_t &) = delete;

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

public:
    static void Show(string_view_utf8 txt);
};
