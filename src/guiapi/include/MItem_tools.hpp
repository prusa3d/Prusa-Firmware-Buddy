/*****************************************************************************/
//menu items running tools
#pragma once
#include "WindowMenuItems.hpp"
#pragma pack(push, 1)

class MI_WIZARD : public WI_LABEL_t {
    static constexpr const char *const label = "Wizard";

public:
    MI_WIZARD();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_AUTO_HOME : public WI_LABEL_t {
    static constexpr const char *const label = "Auto Home";

public:
    MI_AUTO_HOME();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_MESH_BED : public WI_LABEL_t {
    static constexpr const char *const label = "Mesh Bed Level.";

public:
    MI_MESH_BED();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_SELFTEST : public WI_LABEL_t {
    static constexpr const char *const label = "SelfTest";

public:
    MI_SELFTEST();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_CALIB_FIRST : public WI_LABEL_t {
    static constexpr const char *const label = "First Layer Cal.";

public:
    MI_CALIB_FIRST();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_DISABLE_STEP : public WI_LABEL_t {
    static constexpr const char *const label = "Disable Steppers";

public:
    MI_DISABLE_STEP();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_FACTORY_DEFAULTS : public WI_LABEL_t {
    static constexpr const char *const label = "Factory Reset";

public:
    MI_FACTORY_DEFAULTS();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_SAVE_DUMP : public WI_LABEL_t {
    static constexpr const char *const label = "Save Crash Dump";

public:
    MI_SAVE_DUMP();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_HF_TEST_0 : public WI_LABEL_t {
    static constexpr const char *const label = "HF0 test";

public:
    MI_HF_TEST_0();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_HF_TEST_1 : public WI_LABEL_t {
    static constexpr const char *const label = "HF1 test";

public:
    MI_HF_TEST_1();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_EE_LOAD_400 : public WI_LABEL_t {
    static constexpr const char *const label = "EE 4.0.0";

public:
    MI_EE_LOAD_400();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_EE_LOAD_401 : public WI_LABEL_t {
    static constexpr const char *const label = "EE 4.0.1";

public:
    MI_EE_LOAD_401();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_EE_LOAD_402 : public WI_LABEL_t {
    static constexpr const char *const label = "EE 4.0.2";

public:
    MI_EE_LOAD_402();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_EE_LOAD_403RC1 : public WI_LABEL_t {
    static constexpr const char *const label = "EE 4.0.3-RC1";

public:
    MI_EE_LOAD_403RC1();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_EE_LOAD_403 : public WI_LABEL_t {
    static constexpr const char *const label = "EE 4.0.3";

public:
    MI_EE_LOAD_403();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_EE_LOAD : public WI_LABEL_t {
    static constexpr const char *const label = "EE load";

public:
    MI_EE_LOAD();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_EE_SAVE : public WI_LABEL_t {
    static constexpr const char *const label = "EE save";

public:
    MI_EE_SAVE();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_EE_SAVEXML : public WI_LABEL_t {
    static constexpr const char *const label = "EE save xml";

public:
    MI_EE_SAVEXML();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

class MI_M600 : public WI_LABEL_t {
    static constexpr const char *const label = "Change Filament";

public:
    MI_M600();

protected:
    virtual void click(Iwindow_menu_t &window_menu);
};

#pragma pack(pop)
