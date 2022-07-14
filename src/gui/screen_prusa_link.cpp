#include "gui.hpp"
#include "screen_menu.hpp"
#include "screen_menus.hpp"
#include "WindowMenuItems.hpp"
#include "ScreenHandler.hpp"
#include "RAII.hpp"

#include "../../lib/WUI/wui.h"

#include <array>

#include "wui_api.h"

#define api_key_format "Current Api Key\n    %s"
static constexpr size_t API_KEY_STR_LENGTH = PL_API_KEY_SIZE + sizeof(api_key_format) - sizeof("%s"); // don't need space for '%s' and '\0' since PL_API_KEY_SIZE contains '\0' too
typedef char api_key_str_t[API_KEY_STR_LENGTH];

// ----------------------------------------------------------------
// GUI Prusa Link X-Api_Key regenerate
class MI_PL_REGENERATE_API_KEY : public WI_LABEL_t {
    constexpr static const char *const label = N_("Generate Api key");

public:
    MI_PL_REGENERATE_API_KEY()
        : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {}

public:
    enum EventMask { value = 1 << 18 };

protected:
    virtual void click(IWindowMenu &) override {
        Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)EventMask::value);
    }
};

// ----------------------------------------------------------------
// GUI Prusa Link start after printer startup
class MI_PL_ENABLED : public WI_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Enabled");

public:
    MI_PL_ENABLED()
        : WI_SWITCH_OFF_ON_t(eeprom_get_ui8(EEVAR_PL_RUN),
            string_view_utf8::MakeCPUFLASH((const uint8_t *)label), 0, is_enabled_t::yes, is_hidden_t::no) {}

public:
    enum EventMask { value = 1 << 19 };

protected:
    virtual void OnChange(size_t old_index) override {
        Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)(EventMask::value | this->index));
    }
};

using PLMenuContainer = WinMenuContainer<MI_RETURN, MI_PL_ENABLED, MI_PL_REGENERATE_API_KEY>;

class ScreenMenuPrusaLink : public AddSuperWindow<screen_t> {
    constexpr static const char *const label = N_("PRUSA LINK");
    static constexpr int canvas_font = IDR_FNT_SPECIAL;

    PLMenuContainer container;
    window_menu_t menu;
    window_header_t header;
    window_text_t canvas;

    api_key_str_t api_key_str;

    inline void display_api_key(const char *api_key) {
        snprintf(api_key_str, API_KEY_STR_LENGTH, api_key_format, api_key);
        canvas.text = string_view_utf8::MakeRAM((const uint8_t *)api_key_str);
        canvas.Invalidate();
    }

public:
    ScreenMenuPrusaLink();

    static inline uint16_t canvas_font_height() {
        return resource_font(canvas_font)->h;
    }

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};

ScreenMenuPrusaLink::ScreenMenuPrusaLink()
    : AddSuperWindow<screen_t>(nullptr, win_type_t::normal, is_closed_on_timeout_t::no)
    , menu(this, GuiDefaults::RectScreenBody - Rect16::Height_t(canvas_font_height()), &container)
    , header(this)
    , canvas(this, Rect16(GuiDefaults::RectScreen.Left(), uint16_t(GuiDefaults::RectScreen.Height()) - 12 * canvas_font_height(), GuiDefaults::RectScreen.Width(), 3 * canvas_font_height()), is_multiline::yes) {
    header.SetText(_(label));
    canvas.font = resource_font(canvas_font);
    CaptureNormalWindow(menu); // set capture to list
    display_api_key(wui_get_api_key());
    // The user might want to read the API key from here, don't time it out on them.
    ClrMenuTimeoutClose();
}

ScreenFactory::UniquePtr GetScreenPrusaLink() {
    return ScreenFactory::Screen<ScreenMenuPrusaLink>();
}

void ScreenMenuPrusaLink::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::CHILD_CLICK: {
        uint32_t action = ((uint32_t)param) & 0xFFFF;
        uint32_t type = ((uint32_t)param) & 0xFFFF0000;
        switch (type) {
        case MI_PL_REGENERATE_API_KEY::EventMask::value: {
            char api_key[PL_API_KEY_SIZE] = { 0 };
            wui_generate_api_key(api_key, PL_API_KEY_SIZE);
            wui_store_api_key(api_key, PL_API_KEY_SIZE);
            display_api_key(api_key);
            break;
        }
        case MI_PL_ENABLED::EventMask::value:
            eeprom_set_ui8(EEVAR_PL_RUN, action);
            notify_reconfigure();
            break;
        default:
            break;
        }
    } break;
    default:
        SuperWindowEvent(sender, event, param);
        break;
    }
}
