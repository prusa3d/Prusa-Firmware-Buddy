/**
 * @file MItem_file_extensions.hpp
 * @brief file extension visibility control menu item
 */
#pragma once
#include "WindowMenuItems.hpp"

class MI_SHOW_FILE_EXTENSIONS : public WI_ICON_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Show File Extensions");

public:
    MI_SHOW_FILE_EXTENSIONS();

    virtual void OnChange(size_t old_index) override;
};
