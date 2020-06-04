/*****************************************************************************/
//Screen openning menu items
#pragma once
#include "WindowMenuItems.hpp"
#pragma pack(push, 1)

class MI_VERSION_INFO : public WI_LABEL_t {
    static constexpr const char *const label = "Version Info";

public:
    MI_VERSION_INFO();

protected:
    virtual void click(Iwindow_menu_t &window_menu) override;
};

class MI_FILAMENT : public WI_LABEL_t {
    static constexpr const char *const label = "Filament";

public:
    MI_FILAMENT();

protected:
    virtual void click(Iwindow_menu_t &window_menu) override;
};

class MI_SYS_INFO : public WI_LABEL_t {
    static constexpr const char *const label = "System Info";

public:
    MI_SYS_INFO();

protected:
    virtual void click(Iwindow_menu_t &window_menu) override;
};

class MI_STATISTIC_disabled : public WI_LABEL_t {
    static constexpr const char *const label = "Statistic";

public:
    MI_STATISTIC_disabled();

protected:
    virtual void click(Iwindow_menu_t &window_menu) override {}
};

class MI_FAIL_STAT_disabled : public WI_LABEL_t {
    static constexpr const char *const label = "Fail Stats";

public:
    MI_FAIL_STAT_disabled();

protected:
    virtual void click(Iwindow_menu_t &window_menu) override {}
};

class MI_SUPPORT_disabled : public WI_LABEL_t {
    static constexpr const char *const label = "Support";

public:
    MI_SUPPORT_disabled();

protected:
    virtual void click(Iwindow_menu_t &window_menu) override {}
};

class MI_QR_test : public WI_LABEL_t {
    static constexpr const char *const label = "QR test";

public:
    MI_QR_test();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_QR_info : public WI_LABEL_t {
    static constexpr const char *const label = "Send Info by QR";

public:
    MI_QR_info();

protected:
    virtual void click(Iwindow_menu_t &window_menu) override;
};

class MI_TEMPERATURE : public WI_LABEL_t {
    static constexpr const char *const label = "Temperature";

public:
    MI_TEMPERATURE();

protected:
    virtual void click(Iwindow_menu_t &window_menu) override;
};

class MI_MOVE_AXIS : public WI_LABEL_t {
    static constexpr const char *const label = "Move Axis";

public:
    MI_MOVE_AXIS();

protected:
    virtual void click(Iwindow_menu_t &window_menu) override;
};

class MI_SERVICE : public WI_LABEL_t {
    static constexpr const char *const label = "Service";

public:
    MI_SERVICE();

protected:
    virtual void click(Iwindow_menu_t &window_menu) override;
};

class MI_TEST : public WI_LABEL_t {
    static constexpr const char *const label = "Test";

public:
    MI_TEST();

protected:
    virtual void click(Iwindow_menu_t &window_menu) override;
};

class MI_FW_UPDATE : public WI_LABEL_t {
    static constexpr const char *const label = "FW Update";

public:
    MI_FW_UPDATE();

protected:
    virtual void click(Iwindow_menu_t &window_menu) override;
};

class MI_LAN_SETTINGS : public WI_LABEL_t {
    static constexpr const char *const label = "Lan settings";

public:
    MI_LAN_SETTINGS();

protected:
    virtual void click(Iwindow_menu_t &window_menu) override;
};

class MI_MESSAGES : public WI_LABEL_t {
    static constexpr const char *const label = "Messages";

public:
    MI_MESSAGES();

protected:
    virtual void click(Iwindow_menu_t &window_menu) override;
};

#pragma pack(pop)
