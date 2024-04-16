#pragma once
#include "WindowMenuItems.hpp"
#include "WindowItemFormatableLabel.hpp"

class MI_ENCLOSURE : public IWindowMenuItem {
    constexpr static const char *label = N_("Enclosure Settings");

public:
    MI_ENCLOSURE();

protected:
    virtual void click(IWindowMenu &windowMenu) override;
};

class MI_ENCLOSURE_ENABLE : public WI_ICON_SWITCH_OFF_ON_t {
    static constexpr const char *const label = N_("Enclosure");
    static constexpr const char *const wait_str = N_("Testing enclosure fan");

public:
    MI_ENCLOSURE_ENABLE();

protected:
    void OnChange(size_t old_index) override;
};

class MI_ENCLOSURE_TEMP : public WI_FORMATABLE_LABEL_t<int> {
    static constexpr const char *const label = N_("Temperature");

public:
    MI_ENCLOSURE_TEMP();
};

class MI_ENCLOSURE_ALWAYS_ON : public WI_ICON_SWITCH_OFF_ON_t {
    static constexpr const char *const label = N_("Fan Always On");

public:
    MI_ENCLOSURE_ALWAYS_ON();

protected:
    void OnChange(size_t old_index);
};

class MI_ENCLOSURE_POST_PRINT : public WI_ICON_SWITCH_OFF_ON_t {
    static constexpr const char *const label = N_("Post Print Filtration");

public:
    MI_ENCLOSURE_POST_PRINT();

protected:
    void OnChange(size_t old_index);
};

class MI_ENCLOSURE_FILTER_COUNTER : public WI_INFO_t {
    static constexpr const char *const label = N_("HEPA Filter Check");

public:
    MI_ENCLOSURE_FILTER_COUNTER();
};

class MI_ENCLOSURE_FAN_SETTING : public WiSpinInt {
    static constexpr const char *const label = N_("Fan Rotation");

public:
    MI_ENCLOSURE_FAN_SETTING();

protected:
    virtual void OnClick() override;
};

class MI_ENCLOSURE_MANUAL_SETTINGS : public IWindowMenuItem {
    static constexpr const char *const label = N_("Manual Configuration");

public:
    MI_ENCLOSURE_MANUAL_SETTINGS();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_ENCLOSURE_FILTRATION : public IWindowMenuItem {
    static constexpr const char *const label = N_("Filtration");

public:
    MI_ENCLOSURE_FILTRATION();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_ENCLOSURE_FILTER_CHANGE : public IWindowMenuItem {
    static constexpr const char *const label = N_("Change Filter");

public:
    MI_ENCLOSURE_FILTER_CHANGE();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};
