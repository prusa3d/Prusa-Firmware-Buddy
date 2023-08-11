/**
 * @file screen_prusa_link.hpp
 */

#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include <config_store/constants.hpp>

// ----------------------------------------------------------------
// GUI Prusa Link Password regenerate
class MI_PL_REGENERATE_PASSWORD : public WI_LABEL_t {
    constexpr static const char *const label = N_("Generate Password");

public:
    MI_PL_REGENERATE_PASSWORD();

public:
    enum EventMask { value = 1 << 18 };

protected:
    virtual void click(IWindowMenu &) override;
};

// ----------------------------------------------------------------
// GUI Prusa Link start after printer startup
class MI_PL_ENABLED : public WI_ICON_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Enabled");

public:
    MI_PL_ENABLED();

public:
    enum EventMask { value = 1 << 19 };

protected:
    virtual void OnChange(size_t old_index) override;
};

class MI_PL_PASSWORD_LABEL : public WI_LABEL_t {
    constexpr static const char *const label = N_("Password");

public:
    MI_PL_PASSWORD_LABEL();

protected:
    virtual void click([[maybe_unused]] IWindowMenu &window_menu) override {}
};

class MI_PL_PASSWORD_VALUE : public WI_LABEL_t {
    static constexpr size_t PASSWD_STR_LENGTH = config_store_ns::pl_password_size + 1; // don't need space for '%s' and '\0' since pl_password_size contains '\0' too

#ifdef USE_ST7789
    constexpr static const char *const label = N_("");
#else
    constexpr static const char *const label = N_("Password");
#endif
    char passwd_buffer[PASSWD_STR_LENGTH];

protected:
    virtual void printExtension(Rect16 extension_rect, color_t color_text, color_t color_back, ropfn raster_op) const override;
    virtual void click([[maybe_unused]] IWindowMenu &window_menu) override {
    }

public:
    void print_password(const char *passwd);
    MI_PL_PASSWORD_VALUE();
};

class MI_PL_USER : public WI_LABEL_t {
    constexpr static const char *const label = N_("User");

protected:
    virtual void printExtension(Rect16 extension_rect, color_t color_text, color_t color_back, ropfn raster_op) const override;
    virtual void click([[maybe_unused]] IWindowMenu &window_menu) override {
    }

public:
    MI_PL_USER();
};

using PLMenuContainer = WinMenuContainer<MI_RETURN, MI_PL_ENABLED, MI_PL_REGENERATE_PASSWORD, MI_PL_USER,
#ifdef USE_ST7789
    MI_PL_PASSWORD_LABEL,
#endif
    MI_PL_PASSWORD_VALUE>;

class ScreenMenuPrusaLink : public AddSuperWindow<screen_t> {
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
