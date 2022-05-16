/*****************************************************************************/
//menu items running tools
#pragma once
#include "WindowMenuItems.hpp"
#include "i18n.h"
#include "filament.hpp"
#include "WindowItemFormatableLabel.hpp"
#include "config.h"

class MI_WIZARD : public WI_LABEL_t {
    static constexpr const char *const label = N_("Wizard");

public:
    MI_WIZARD();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_FILAMENT_SENSOR : public WI_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Filament Sensor");

    bool init_index() const;

public:
    MI_FILAMENT_SENSOR()
        : WI_SWITCH_OFF_ON_t(init_index(), _(label), 0, is_enabled_t::yes, is_hidden_t::no) {}

protected:
    virtual void OnChange(size_t old_index) override;
};

class MI_LIVE_ADJUST_Z : public WI_LABEL_t {
    static constexpr const char *const label = N_("Live Adjust Z");

public:
    MI_LIVE_ADJUST_Z();

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
    static constexpr const char *const label = N_("Mesh Bed Leveling");

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

class MI_SELFTEST_RESULT : public WI_LABEL_t {
    static constexpr const char *const label = N_("Show SelfTest result");

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
    static constexpr const char *const label = N_("Test FANs");

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

class MI_TEST_HEAT : public WI_LABEL_t {
    static constexpr const char *const label = N_("Test Heaters");

public:
    MI_TEST_HEAT();

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

class MI_TEST_ABORT : public WI_LABEL_t {
    static constexpr const char *const label = "Test Abort"; // debug only - not translated

public:
    MI_TEST_ABORT();

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

#ifdef BUDDY_ENABLE_DFU_ENTRY
class MI_ENTER_DFU : public WI_LABEL_t {
    static constexpr const char *const label = "Enter DFU";

public:
    MI_ENTER_DFU();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};
#endif

class MI_SAVE_DUMP : public WI_LABEL_t {
    static constexpr const char *const label = N_("Save Crash Dump");

public:
    MI_SAVE_DUMP();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_XFLASH_DELETE : public WI_LABEL_t {
    static constexpr const char *const label = "XFLASH DELETE"; // intentionally not translated, only for debugging

public:
    MI_XFLASH_DELETE();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_XFLASH_RESET : public WI_LABEL_t {
    static constexpr const char *const label = "XFLASH RESET"; // intentionally not translated, only for debugging

public:
    MI_XFLASH_RESET();

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

#ifdef _DEBUG
static constexpr size_t MI_SOUND_MODE_COUNT = 5;
#else
static constexpr size_t MI_SOUND_MODE_COUNT = 4;
#endif
class MI_SOUND_MODE : public WI_SWITCH_t<MI_SOUND_MODE_COUNT> {
    constexpr static const char *const label = N_("Sound Mode");

    constexpr static const char *str_Once = N_("Once");
    constexpr static const char *str_Loud = N_("Loud");
    constexpr static const char *str_Silent = N_("Silent");
    constexpr static const char *str_Assist = N_("Assist");
    constexpr static const char *str_Debug = "Debug";

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

class MI_SORT_FILES : public WI_SWITCH_t<2> {
    constexpr static const char *const label = N_("Sort Files by");

    constexpr static const char *str_name = N_("Name");
    constexpr static const char *str_time = N_("Time");

public:
    MI_SORT_FILES();
    virtual void OnChange(size_t old_index) override;
};

class MI_SOUND_VOLUME : public WiSpinInt {
    constexpr static const char *const label = N_("Sound Volume");

public:
    MI_SOUND_VOLUME();
    virtual void OnClick() override;
    /* virtual void Change() override; */
};

class MI_TIMEZONE : public WiSpinInt {
    constexpr static const char *const label = "TZ UTC(+/-)"; // intentionally not translated

public:
    MI_TIMEZONE();
    virtual void OnClick() override;
};

class I_MI_Filament : public WI_LABEL_t {
public:
    I_MI_Filament(string_view_utf8 long_name)
        : WI_LABEL_t(long_name, 0, is_enabled_t::yes, is_hidden_t::no) {}

protected:
    void click_at(filament_t filament_index);
};

template <filament_t T>
class MI_Filament : public I_MI_Filament {
public:
    MI_Filament()
        : I_MI_Filament(_(Filaments::Get(T).long_name)) {}

protected:
    virtual void click(IWindowMenu & /*window_menu*/) override {
        click_at(T);
    }
};

class MI_FILAMENT_SENSOR_STATE : public WI_SWITCH_0_1_NA_t {
    static constexpr const char *const label = N_("Filament Sensor");
    static state_t get_state();

public:
    MI_FILAMENT_SENSOR_STATE();
    bool StateChanged();
    virtual void OnChange(size_t old_index) override {}
};

class MI_MINDA : public WI_SWITCH_0_1_NA_t {
    static constexpr const char *const label = N_("M.I.N.D.A.");
    static state_t get_state();

public:
    MI_MINDA();
    bool StateChanged();
    virtual void OnChange(size_t old_index) override {}
};

class MI_FAN_CHECK : public WI_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Fan Check");

public:
    MI_FAN_CHECK();
    virtual void OnChange(size_t old_index) override;
};

/******************************************************************/
// WI_INFO_t
class MI_FS_AUTOLOAD : public WI_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("FS Autoload");

public:
    MI_FS_AUTOLOAD();
    virtual void OnChange(size_t old_index) override;
};
class MI_ODOMETER_DIST : public WI_FORMATABLE_LABEL_t<float> {
public:
    MI_ODOMETER_DIST(string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden, float initVal);
};

class MI_ODOMETER_DIST_X : public MI_ODOMETER_DIST {
    constexpr static const char *const label = N_("X axis");

public:
    MI_ODOMETER_DIST_X();
};
class MI_ODOMETER_DIST_Y : public MI_ODOMETER_DIST {
    constexpr static const char *const label = N_("Y axis");

public:
    MI_ODOMETER_DIST_Y();
};
class MI_ODOMETER_DIST_Z : public MI_ODOMETER_DIST {
    constexpr static const char *const label = N_("Z axis");

public:
    MI_ODOMETER_DIST_Z();
};
class MI_ODOMETER_DIST_E : public MI_ODOMETER_DIST {
    constexpr static const char *const label = N_("Filament");

public:
    MI_ODOMETER_DIST_E();
};
class MI_ODOMETER_TIME : public WI_FORMATABLE_LABEL_t<uint32_t> {
    constexpr static const char *const label = N_("Print time");

public:
    MI_ODOMETER_TIME();
};

class MI_FOOTER_RESET : public WI_LABEL_t {
    static constexpr const char *const label = N_("Reset");

public:
    MI_FOOTER_RESET();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};
