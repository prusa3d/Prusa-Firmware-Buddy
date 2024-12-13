// liveadjustz.hpp
#pragma once

#include <screen.hpp>
#include "window_text.hpp"
#include "window_numb.hpp"
#include "window_icon.hpp"
#include "window_arrows.hpp"

class WindowScale : public window_frame_t {
    window_numb_t scaleNum0;
    window_numb_t scaleNum1;
    window_numb_t scaleNum2;

    // existing value represents state where old line was not erased yet
    std::optional<uint16_t> mark_old_y;
    uint16_t mark_new_y;

public:
    WindowScale(window_t *parent, point_i16_t pt);

    /**
     * @brief Set mark position.
     * @param relative position, 0 - top, 1 - bottom
     */
    void SetMark(float relative);

protected:
    virtual void unconditionalDraw() override;
    Rect16 getNumRect(point_i16_t pt) const;

private:
    void horizLine(uint16_t width_pad, uint16_t height, Color color);
    void horizLineWhite(uint16_t width_pad, uint16_t height) {
        horizLine(width_pad, height, COLOR_WHITE);
    }
};

// regular window bound to Z calib
class WindowLiveAdjustZ : public window_frame_t {
protected:
    window_numb_t number;
    WindowArrows arrows;

public:
    WindowLiveAdjustZ(window_t *parent, point_i16_t pt);
    void Save();
    ~WindowLiveAdjustZ() {
        Save();
    }

    float GetValue() const { return number.GetValue(); }

protected:
    void Change(int dif);

    static constexpr Rect16 getNumberRect(point_i16_t pt) {
        return Rect16(pt, 80, 25);
    }
    static constexpr point_i16_t getIconPoint(point_i16_t pt) {
        point_i16_t ret = getNumberRect(pt).TopEndPoint();
        return { ret.x, int16_t(ret.y + 0) };
    }

    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;
};

class WindowLiveAdjustZ_withText : public WindowLiveAdjustZ {
    window_text_t text;

public:
    static constexpr const char *text_str = N_("Z height:");
    WindowLiveAdjustZ_withText(window_t *parent, point_i16_t pt, size_t width);
    void Idle();
    void Activate();
    bool IsActive();

protected:
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;
};

class ScreenLiveAdjustZ : public screen_t {
    window_text_t text;
    window_icon_t nozzle_icon;
    WindowLiveAdjustZ adjuster;
    WindowScale scale;

public:
    ScreenLiveAdjustZ();

protected:
    void moveNozzle();
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;
};

void open_live_adjust_z_screen();
