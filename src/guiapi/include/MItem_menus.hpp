/*****************************************************************************/
//Screen openning menu items
#pragma once
#include "WindowMenuItems.hpp"
#include "i18n.h"
#include "eeprom.h"

class MI_VERSION_INFO : public WI_LABEL_t {
    static constexpr const char *const label = N_("Version Info");

public:
    MI_VERSION_INFO();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_SENSOR_INFO : public WI_LABEL_t {
    static constexpr const char *const label = N_("Sensor Info");

public:
    MI_SENSOR_INFO();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_FILAMENT : public WI_LABEL_t {
    static constexpr const char *const label = N_("Filament");

public:
    MI_FILAMENT();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_SYS_INFO : public WI_LABEL_t {
    static constexpr const char *const label = N_("System Info");

public:
    MI_SYS_INFO();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_STATISTIC_disabled : public WI_LABEL_t {
    static constexpr const char *const label = N_("Statistic");

public:
    MI_STATISTIC_disabled();

protected:
    virtual void click(IWindowMenu &window_menu) override {}
};

class MI_FAIL_STAT_disabled : public WI_LABEL_t {
    static constexpr const char *const label = N_("Fail Stats");

public:
    MI_FAIL_STAT_disabled();

protected:
    virtual void click(IWindowMenu &window_menu) override {}
};

class MI_SUPPORT_disabled : public WI_LABEL_t {
    static constexpr const char *const label = N_("Support");

public:
    MI_SUPPORT_disabled();

protected:
    virtual void click(IWindowMenu &window_menu) override {}
};

class MI_TEMPERATURE : public WI_LABEL_t {
    static constexpr const char *const label = N_("Temperature");

public:
    MI_TEMPERATURE();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_MOVE_AXIS : public WI_LABEL_t {
    static constexpr const char *const label = N_("Move Axis");

public:
    MI_MOVE_AXIS();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_SERVICE : public WI_LABEL_t {
    static constexpr const char *const label = N_("Service");

public:
    MI_SERVICE();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_TEST : public WI_LABEL_t {
    static constexpr const char *const label = N_("Test");

public:
    MI_TEST();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_FW_UPDATE : public WI_LABEL_t {
    static constexpr const char *const label = N_("FW Update");

public:
    MI_FW_UPDATE();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_LAN_SETTINGS : public WI_LABEL_t {
    static constexpr const char *const label = N_("Lan Settings");

public:
    MI_LAN_SETTINGS();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_MESSAGES : public WI_LABEL_t {
    static constexpr const char *const label = N_("Messages");

public:
    MI_MESSAGES();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_LANGUAGE : public WI_LABEL_t {
    static constexpr const char *const label = N_("Language");

public:
    MI_LANGUAGE();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_HW_SETUP : public WI_LABEL_t {
    static constexpr const char *const label = N_("HW Setup");

public:
    MI_HW_SETUP();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_CURRENT_PROFILE : public WI_LABEL_t {
    static constexpr const char *const label = N_("Current Profile");
    char name[MAX_SHEET_NAME_LENGTH + 3];

public:
    MI_CURRENT_PROFILE();

    void UpdateLabel();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_EEPROM : public WI_LABEL_t {
    static constexpr const char *const label = "Eeprom";

public:
    MI_EEPROM();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_DEVHASH_IN_QR : public WI_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Device hash in QR");

public:
    MI_DEVHASH_IN_QR();
    virtual void OnChange(size_t old_index) override;
};
