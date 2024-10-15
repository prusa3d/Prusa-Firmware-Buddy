/**
 * @file MItem_basic_selftest.hpp
 * @author Radek Vana
 * @brief general (all printer types) selftest menu items
 * @date 2021-10-04
 */

#pragma once
#include "WindowMenuItems.hpp"
#include "i18n.h"
#include "WindowItemFormatableLabel.hpp"
#include "WindowItemFanLabel.hpp"
#include <option/has_toolchanger.h>

#if HAS_TOOLCHANGER()

class MI_RESTORE_CALIBRATION_FROM_USB : public IWindowMenuItem {
    static constexpr const char *const label = N_("Restore Calibration from USB");

public:
    MI_RESTORE_CALIBRATION_FROM_USB();

protected:
    virtual void click(IWindowMenu &window_menu) override;

private:
    bool restore_fs_calibration();
};

class MI_BACKUP_CALIBRATION_TO_USB : public IWindowMenuItem {
    static constexpr const char *const label = N_("Backup Calibration to USB");

public:
    MI_BACKUP_CALIBRATION_TO_USB();

protected:
    virtual void click(IWindowMenu &window_menu) override;

private:
    bool backup_fs_calibration();
};
#endif
