/// @file
#pragma once

#include <async_job/async_job.hpp>
#include <basic_screen_menu.hpp>
#include <WindowMenuInfo.hpp>
#include <WindowMenuSwitch.hpp>

class MI_KEY final : public WI_INFO_t {
public:
    MI_KEY();
    virtual void Loop() override;
};

class MI_KEYGEN final : public IWindowMenuItem {
    AsyncJobWithResult<bool> key_generation;

public:
    MI_KEYGEN();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_EXPORT final : public IWindowMenuItem {
public:
    MI_EXPORT();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_IDENTITY_CHECKING final : public MenuItemSwitch {
public:
    MI_IDENTITY_CHECKING();

protected:
    bool on_item_selected(const OnItemSelectedArgs &args) override;
};

using ScreenMenuE2eeBase = BasicScreenMenu<
    MI_KEY,
    MI_KEYGEN,
    MI_EXPORT,
    MI_IDENTITY_CHECKING>;

class ScreenMenuE2ee final : public ScreenMenuE2eeBase {
public:
    ScreenMenuE2ee();
};
