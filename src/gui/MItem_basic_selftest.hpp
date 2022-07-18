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

class MI_WIZARD : public WI_LABEL_t {
    static constexpr const char *const label = N_("Wizard");

public:
    MI_WIZARD();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_SELFTEST : public WI_LABEL_t {
    static constexpr const char *const label = N_("Run Full Selftest");

public:
    MI_SELFTEST();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_SELFTEST_RESULT : public WI_LABEL_t {
    static constexpr const char *const label = N_("Show Selftest Result");

public:
    MI_SELFTEST_RESULT();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_CALIB_FIRST : public WI_LABEL_t {
    static constexpr const char *const label = N_("First Layer Calibration");

public:
    MI_CALIB_FIRST();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_TEST_FANS : public WI_LABEL_t {
    static constexpr const char *const label = N_("Test Fans");

public:
    MI_TEST_FANS();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_TEST_XYZ : public WI_LABEL_t {
    static constexpr const char *const label = N_("Test XYZ-Axis");

public:
    MI_TEST_XYZ();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_TEST_X : public WI_LABEL_t {
    static constexpr const char *const label = "Test X-Axis"; // dev mode - not translated

public:
    MI_TEST_X();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_TEST_Y : public WI_LABEL_t {
    static constexpr const char *const label = "Test Y-Axis"; // dev mode - not translated

public:
    MI_TEST_Y();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_TEST_Z : public WI_LABEL_t {
    static constexpr const char *const label = "Test Z-Axis"; // dev mode - not translated

public:
    MI_TEST_Z();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_TEST_HEAT : public WI_LABEL_t {
    static constexpr const char *const label = N_("Test Heaters");

public:
    MI_TEST_HEAT();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_TEST_HOTEND : public WI_LABEL_t {
    static constexpr const char *const label = "Test Hotend"; // debug only - not translated

public:
    MI_TEST_HOTEND();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_TEST_BED : public WI_LABEL_t {
    static constexpr const char *const label = "Test Bed"; // debug only - not translated

public:
    MI_TEST_BED();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_ADVANCED_FAN_TEST : public WI_LABEL_t {
    static constexpr const char *const label = "Advanced fan test"; // debug only - not translated

public:
    MI_ADVANCED_FAN_TEST();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};
