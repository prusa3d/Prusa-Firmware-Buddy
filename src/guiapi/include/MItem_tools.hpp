/*****************************************************************************/
//menu items running tools
#pragma once
#include "WindowMenuItems.hpp"
#include "../lang/i18n.h"

class MI_WIZARD : public WI_LABEL_t {
    static constexpr const char *const label = N_("Wizard");

public:
    MI_WIZARD();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_AUTO_HOME : public WI_LABEL_t {
    static constexpr const char *const label = N_("Auto Home");

public:
    MI_AUTO_HOME();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_MESH_BED : public WI_LABEL_t {
    static constexpr const char *const label = N_("Mesh Bed Level.");

public:
    MI_MESH_BED();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_SELFTEST : public WI_LABEL_t {
    static constexpr const char *const label = N_("SelfTest");

public:
    MI_SELFTEST();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_CALIB_FIRST : public WI_LABEL_t {
    static constexpr const char *const label = N_("First Layer Cal.");

public:
    MI_CALIB_FIRST();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_DISABLE_STEP : public WI_LABEL_t {
    static constexpr const char *const label = N_("Disable Steppers");

public:
    MI_DISABLE_STEP();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_FACTORY_DEFAULTS : public WI_LABEL_t {
    static constexpr const char *const label = N_("Factory Reset");

public:
    MI_FACTORY_DEFAULTS();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_SAVE_DUMP : public WI_LABEL_t {
    static constexpr const char *const label = N_("Save Crash Dump");

public:
    MI_SAVE_DUMP();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_HF_TEST_0 : public WI_LABEL_t {
    static constexpr const char *const label = "HF0 test"; // intentionally not translated, only for debugging

public:
    MI_HF_TEST_0();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_HF_TEST_1 : public WI_LABEL_t {
    static constexpr const char *const label = "HF1 test"; // intentionally not translated, only for debugging

public:
    MI_HF_TEST_1();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_EE_LOAD_400 : public WI_LABEL_t {
    static constexpr const char *const label = "EE 4.0.0"; // intentionally not translated, only for debugging

public:
    MI_EE_LOAD_400();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_EE_LOAD_401 : public WI_LABEL_t {
    static constexpr const char *const label = "EE 4.0.1"; // intentionally not translated, only for debugging

public:
    MI_EE_LOAD_401();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_EE_LOAD_402 : public WI_LABEL_t {
    static constexpr const char *const label = "EE 4.0.2"; // intentionally not translated, only for debugging

public:
    MI_EE_LOAD_402();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_EE_LOAD_403RC1 : public WI_LABEL_t {
    static constexpr const char *const label = "EE 4.0.3-RC1"; // intentionally not translated, only for debugging

public:
    MI_EE_LOAD_403RC1();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_EE_LOAD_403 : public WI_LABEL_t {
    static constexpr const char *const label = "EE 4.0.3"; // intentionally not translated, only for debugging

public:
    MI_EE_LOAD_403();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_EE_LOAD : public WI_LABEL_t {
    static constexpr const char *const label = "EE load"; // intentionally not translated, only for debugging

public:
    MI_EE_LOAD();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_EE_SAVE : public WI_LABEL_t {
    static constexpr const char *const label = "EE save"; // intentionally not translated, only for debugging

public:
    MI_EE_SAVE();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_EE_SAVEXML : public WI_LABEL_t {
    static constexpr const char *const label = "EE save xml"; // intentionally not translated, only for debugging

public:
    MI_EE_SAVEXML();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_M600 : public WI_LABEL_t {
    static constexpr const char *const label = N_("Change Filament");

public:
    MI_M600();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_TIMEOUT : public WI_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Menu Timeout");

public:
    MI_TIMEOUT();
    virtual void OnChange(size_t old_index) override;
};

class MI_SOUND_MODE : public WI_SWITCH_t<4> {
    constexpr static const char *const label = N_("Sound Mode");

    constexpr static const char *str_Once = N_("Once");
    constexpr static const char *str_Loud = N_("Loud");
    constexpr static const char *str_Silent = N_("Silent");
    constexpr static const char *str_Assist = N_("Assist");
    size_t init_index() const;

public:
    MI_SOUND_MODE();
    virtual void OnChange(size_t old_index) override;
};

class MI_SOUND_TYPE : public WI_SWITCH_t<8> {
    constexpr static const char *const label = "Sound Type";

    constexpr static const char *str_ButtonEcho = "ButtonEcho";
    constexpr static const char *str_StandardPrompt = "StandardPrompt";
    constexpr static const char *str_StandardAlert = "StandardAlert";
    constexpr static const char *str_CriticalAlert = "CriticalAlert";
    constexpr static const char *str_EncoderMove = "EncoderMove";
    constexpr static const char *str_BlindAlert = "BlindAlert";
    constexpr static const char *str_Start = "Start";
    constexpr static const char *str_SingleBeep = "SingleBeep";

public:
    MI_SOUND_TYPE();
    virtual void OnChange(size_t old_index) override;
};

class MI_SOUND_VOLUME : public WI_SPIN_U08_t {
    constexpr static const char *const label = N_("Sound Volume");

public:
    MI_SOUND_VOLUME();
    virtual void OnClick() override;
    /* virtual void Change() override; */
};

class MI_TIMEZONE : public WI_SPIN_I08_t {
    constexpr static const char *const label = "TZ UTC(+/-)"; // intentionally not translated

public:
    MI_TIMEZONE();
    virtual void OnClick() override;
};
