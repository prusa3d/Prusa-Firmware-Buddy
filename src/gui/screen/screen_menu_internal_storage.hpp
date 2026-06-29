#pragma once

#include <basic_screen_menu.hpp>
#include <WindowMenuItems.hpp>

class MI_CHECK_INTERNAL_STORAGE : public IWindowMenuItem {
public:
    MI_CHECK_INTERNAL_STORAGE();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_TEST_INTERNAL_STORAGE : public IWindowMenuItem {
public:
    MI_TEST_INTERNAL_STORAGE();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_FORMAT_INTERNAL_STORAGE : public IWindowMenuItem {
public:
    MI_FORMAT_INTERNAL_STORAGE();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

using ScreenMenuInternalStorageBase = BasicScreenMenu<
    MI_CHECK_INTERNAL_STORAGE, MI_TEST_INTERNAL_STORAGE, MI_FORMAT_INTERNAL_STORAGE>;

class ScreenMenuInternalStorage : public ScreenMenuInternalStorageBase {
public:
    ScreenMenuInternalStorage();
};
