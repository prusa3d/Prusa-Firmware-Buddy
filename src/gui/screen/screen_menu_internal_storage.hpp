#pragma once

#include <basic_screen_menu.hpp>
#include <WindowMenuItems.hpp>

class MI_FORMAT_INTERNAL_STORAGE : public IWindowMenuItem {
public:
    MI_FORMAT_INTERNAL_STORAGE();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

using ScreenMenuInternalStorageBase = BasicScreenMenu<MI_FORMAT_INTERNAL_STORAGE>;

class ScreenMenuInternalStorage : public ScreenMenuInternalStorageBase {
public:
    ScreenMenuInternalStorage();
};
