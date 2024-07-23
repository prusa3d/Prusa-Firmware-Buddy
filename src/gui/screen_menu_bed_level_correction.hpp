#pragma once

#include <screen_menu.hpp>
#include <i_window_menu_item.hpp>
#include <WindowMenuSpin.hpp>

enum CorrectionIndex : uint8_t {
    LEFT,
    RIGHT,
    FRONT,
    REAR,
};

static constexpr int8_t get_correction_value(CorrectionIndex index) {
    switch (index) {
    case LEFT:
        return config_store().left_bed_correction.get();
    case RIGHT:
        return config_store().right_bed_correction.get();
    case FRONT:
        return config_store().front_bed_correction.get();
    case REAR:
        return config_store().rear_bed_correction.get();
    }
    return 0;
}

static constexpr void set_correction_value(CorrectionIndex index, int8_t new_value) {
    switch (index) {
    case LEFT:
        config_store().left_bed_correction.set(new_value);
        break;
    case RIGHT:
        config_store().right_bed_correction.set(new_value);
        break;
    case FRONT:
        config_store().front_bed_correction.set(new_value);
        break;
    case REAR:
        config_store().rear_bed_correction.set(new_value);
        break;
    }
}

class I_MI_CORRECT : public WiSpin {
    static constexpr const char *correction_label(CorrectionIndex index) {
#if PRINTER_IS_PRUSA_MK3_5()
        switch (index) {
        case LEFT:
            return N_("Left Side");
        case RIGHT:
            return N_("Right Side");
        case FRONT:
            return N_("Front Side");
        case REAR:
            return N_("Rear Side");
        }
#endif

#if PRINTER_IS_PRUSA_MINI()
        switch (index) {
        case LEFT:
            return N_("Left Side [um]");
        case RIGHT:
            return N_("Right Side [um]");
        case FRONT:
            return N_("Front Side [um]");
        case REAR:
            return N_("Rear Side [um]");
        }
#endif
        return "";
    }

    CorrectionIndex index;

public:
    I_MI_CORRECT(CorrectionIndex);

    void OnClick() override;
    void Reset();
};

template <CorrectionIndex INDEX>
class MI_CORRECT : public I_MI_CORRECT {
public:
    MI_CORRECT()
        : I_MI_CORRECT(INDEX) {}
};

class MI_RESET : public IWindowMenuItem {
    static constexpr const char *label = N_("Reset");

public:
    MI_RESET();
    void click(IWindowMenu &window_menu) override;
};

using ScreenMenuBedLevelCorrection__ = ScreenMenu<EFooter::On, MI_RETURN, MI_CORRECT<LEFT>, MI_CORRECT<RIGHT>, MI_CORRECT<FRONT>, MI_CORRECT<REAR>, MI_RESET>;

class ScreenMenuBedLevelCorrection : public ScreenMenuBedLevelCorrection__ {
    static constexpr const char *label = N_("BED LEVEL CORRECTION");

public:
    ScreenMenuBedLevelCorrection();

private:
    void windowEvent(window_t *sender, GUI_event_t event, void *param) override;
};
