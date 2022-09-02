#include "gui.hpp"
#include "screen_menu.hpp"
#include "screen_menus.hpp"
#include "WindowMenuItems.hpp"
#include "ScreenHandler.hpp"
#include "RAII.hpp"

#include "../../lib/WUI/wui.h"

#include <array>

#include "wui_api.h"
#include "configuration_store.hpp"

static constexpr size_t PASSWD_STR_LENGTH = PL_PASSWORD_SIZE + 1; // don't need space for '%s' and '\0' since PL_PASSWORD_SIZE contains '\0' too
using ApiKeyType = decltype(config_store().pl_password.get());

// ----------------------------------------------------------------
// GUI Prusa Link Password regenerate
class MI_PL_REGENERATE_PASSWORD : public WI_LABEL_t {
    constexpr static const char *const label = N_("Generate Password");

public:
    MI_PL_REGENERATE_PASSWORD()
        : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

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
        : WI_SWITCH_OFF_ON_t(config_store().pl_run.get(),
            string_view_utf8::MakeCPUFLASH((const uint8_t *)label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

public:
    enum EventMask { value = 1 << 19 };

protected:
    virtual void OnChange(size_t old_index) override {
        Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)(EventMask::value | this->index));
    }
};

class MI_PL_PASSWORD_LABEL : public WI_LABEL_t {
    constexpr static const char *const label = N_("Password");

public:
    MI_PL_PASSWORD_LABEL()
        : WI_LABEL_t(_(label), 0) {}

protected:
    void click(IWindowMenu &window_menu) override {
    }
};

class MI_PL_PASSWORD_VALUE : public WI_LABEL_t {

#ifdef USE_ST7789
    constexpr static const char *const label = N_("");
#else
    constexpr static const char *const label = N_("Password");
#endif
    char passwd_buffer[PASSWD_STR_LENGTH];

protected:
    void printExtension(Rect16 extension_rect, color_t color_text, color_t color_back, ropfn raster_op) const override {
        render_text_align(extension_rect, string_view_utf8::MakeRAM(reinterpret_cast<const uint8_t *>(passwd_buffer)), GuiDefaults::FontMenuSpecial, color_back, (IsFocused() && IsEnabled()) ? COLOR_DARK_GRAY : COLOR_SILVER, GuiDefaults::MenuPaddingItems, Align_t::RightCenter());
    }
    void click(IWindowMenu &window_menu) override {
    }

public:
    void print_password(const char *passwd) {
        snprintf(passwd_buffer, PASSWD_STR_LENGTH + 1, "%s", passwd);
        InValidateExtension();
    }
    MI_PL_PASSWORD_VALUE()
        : WI_LABEL_t(_(label), PASSWD_STR_LENGTH * GuiDefaults::FontMenuSpecial->w) {}
};

class MI_PL_USER : public WI_LABEL_t {
    constexpr static const char *const label = N_("User");

protected:
    void printExtension(Rect16 extension_rect, color_t color_text, color_t color_back, ropfn raster_op) const override {
        render_text_align(extension_rect, string_view_utf8::MakeRAM(reinterpret_cast<const uint8_t *>(PRUSA_LINK_USERNAME)), GuiDefaults::FontMenuSpecial, color_back, (IsFocused() && IsEnabled()) ? COLOR_DARK_GRAY : COLOR_SILVER, GuiDefaults::MenuPaddingItems, Align_t::RightCenter());
    }
    void click(IWindowMenu &window_menu) override {
    }

public:
    MI_PL_USER()
        : WI_LABEL_t(_(label), (sizeof(PRUSA_LINK_USERNAME) + 1) * GuiDefaults::FontMenuSpecial->w) {}
};

using PLMenuContainer = WinMenuContainer<MI_RETURN, MI_PL_ENABLED, MI_PL_REGENERATE_PASSWORD, MI_PL_USER,
#ifdef USE_ST7789
    MI_PL_PASSWORD_LABEL,
#endif
    MI_PL_PASSWORD_VALUE>;

class ScreenMenuPrusaLink : public AddSuperWindow<screen_t> {
    constexpr static const char *const label = N_("PRUSA LINK");
    static constexpr ResourceId canvas_font = IDR_FNT_SPECIAL;

    PLMenuContainer container;
    window_menu_t menu;
    window_header_t header;

    inline void display_passwd(const char *password) {
        container.Item<MI_PL_PASSWORD_VALUE>().print_password(password);
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
    , header(this) {
    header.SetText(_(label));
    CaptureNormalWindow(menu); // set capture to list
    display_passwd(wui_get_password());
    // The user might want to read the password from here, don't time it out on them.
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
        case MI_PL_REGENERATE_PASSWORD::EventMask::value: {
            wui_generate_password();
            display_passwd(wui_get_password());
            break;
        }
        case MI_PL_ENABLED::EventMask::value:
            config_store().pl_run.set(action);
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
