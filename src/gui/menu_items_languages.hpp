/**
 * @file menu_items_languages.hpp
 * @brief language related menu items
 * has 2 mutually exclusive source files menu_items_languages.cpp / menu_items_no_languages.cpp
 */
#pragma once
#include "WindowMenuItems.hpp"
#include "i18n.h"

class MI_LANGUAGE : public IWindowMenuItem {
    static constexpr const char *const label = N_("Language");

public:
    MI_LANGUAGE();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_LANGUAGUE_USB : public IWindowMenuItem {
    static constexpr const char *const label = "Load Lang from USB";

public:
    MI_LANGUAGUE_USB();

protected:
    virtual void click(IWindowMenu &windowMenu) override;
};

class MI_LOAD_LANG : public IWindowMenuItem {
    static constexpr const char *const label = "Load Lang to XFLASH";

public:
    MI_LOAD_LANG();

protected:
    virtual void click(IWindowMenu &windowMenu) override;
};

class MI_LANGUAGUE_XFLASH : public IWindowMenuItem {
    static constexpr const char *const label = "Load Lang from XFLASH";

public:
    MI_LANGUAGUE_XFLASH();

protected:
    virtual void click(IWindowMenu &windowMenu) override;
};
