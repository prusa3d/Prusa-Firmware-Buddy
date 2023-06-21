/**
 * @file screen_menu_move.hpp
 */
#pragma once

#include "screen_menu.hpp"
#include "WindowItemFormatableLabel.hpp"
#include "MItem_filament.hpp"

class I_MI_AXIS : public WiSpinInt {
protected:
    int32_t lastQueuedPos;

    virtual invalidate_t change(int diff) override;
    void loop__(size_t index, int8_t long_seg, uint8_t buffer_len);

public:
    I_MI_AXIS(size_t index);
};

template <size_t INDEX, int8_t LONG_SEG, uint8_t BUFFER_LEN>
class MI_AXIS : public I_MI_AXIS {
protected:
public:
    MI_AXIS<INDEX, LONG_SEG, BUFFER_LEN>()
        : I_MI_AXIS(INDEX) {}

    // enqueue next moves according to value of spinners;
    virtual void Loop() override {
        loop__(INDEX, LONG_SEG, BUFFER_LEN);
    }
};

class MI_AXIS_E : public MI_AXIS<3, 5, 8> {

public:
    MI_AXIS_E()
        : MI_AXIS<3, 5, 8>() {}

    virtual void OnClick() override;
};

class DUMMY_AXIS_E : public WI_FORMATABLE_LABEL_t<int> {
    virtual void click(IWindowMenu &window_menu) override;
    virtual void touch(IWindowMenu &window_menu, point_ui16_t relative_touch_point) override;

public:
    static bool IsTargetTempOk();
    DUMMY_AXIS_E();

    // TODO call automatically in men loop
    void Update();
};

using MI_AXIS_X = MI_AXIS<0, 5, 8>;
using MI_AXIS_Y = MI_AXIS<1, 5, 8>;
#if PRINTER_IS_PRUSA_MINI
using MI_AXIS_Z = MI_AXIS<2, 1, 4>;
#else
using MI_AXIS_Z = MI_AXIS<2, 1, 12>;
#endif

using ScreenMenuMove__ = ScreenMenu<EFooter::On, MI_RETURN, MI_AXIS_X, MI_AXIS_Y, MI_AXIS_Z, MI_AXIS_E, DUMMY_AXIS_E, MI_COOLDOWN>;

class ScreenMenuMove : public ScreenMenuMove__ {
    float prev_accel;

    void checkNozzleTemp();

public:
    constexpr static const char *label = N_("MOVE AXIS");
    static constexpr int temp_ok_range = 10;
    static bool IsTempOk();

    ScreenMenuMove();
    ~ScreenMenuMove();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
