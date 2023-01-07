/**
 * @file window_icon.cpp
 */

#include <unistd.h>
#include "window_icon.hpp"
#include "gui.hpp"
#include "ScreenHandler.hpp"
#include "png_resources.hpp"
#include "gcode_thumb_decoder.h"
#include "gcode_file.h"
#include "gui_invalidate.hpp"
#include "syslog.h"
#include "timing.h"

LOG_COMPONENT_REF(GUI);

window_icon_t::window_icon_t(window_t *parent, Rect16 rect, const png::Resource *res, is_closed_on_click_t close)
    : AddSuperWindow<window_aligned_t>(parent, rect, win_type_t::normal, close)
    , pRes(res) {
    SetAlignment(Align_t::Center());
}

//Icon rect is increased by padding, icon is centered inside it
window_icon_t::window_icon_t(window_t *parent, const png::Resource *res, point_i16_t pt, padding_ui8_t padding, is_closed_on_click_t close)
    : window_icon_t(
        parent,
        [pt, res, padding] {
            if (!res || !res->h || !res->w)
                return Rect16();

            return Rect16(pt,
                res->w + padding.left + padding.right,
                res->h + padding.top + padding.bottom);
        }(),
        res, close) {
}

void window_icon_t::unconditionalDraw() {
    // no PNG assigned
    if (!pRes)
        return;

    ropfn raster_op;
    raster_op.shadow = IsShadowed() ? is_shadowed::yes : is_shadowed::no;
    raster_op.swap_bw = IsFocused() ? has_swapped_bw::yes : has_swapped_bw::no;

    FILE *file = pRes->Get();
    if (!file)
        return;

    point_ui16_t wh_ico = { pRes->w, pRes->h };

    // measure dimensions if unknown
    if (wh_ico.x == 0 || wh_ico.y == 0) {
        fseek(file, pRes->offset, SEEK_SET);
        uint8_t data[32] { 0 };
        const uint8_t *ptr = data;
        {
            size_t sz = fread(&data[0], 1, 32, file);
            if (sz < 32)
                return;
        }
        wh_ico = icon_meas(ptr); // set measured
    }

    if (wh_ico.x < Width() || wh_ico.y < Height()) {
        super::unconditionalDraw(); // draw background
    }

    if (wh_ico.x == 0 || wh_ico.y == 0) {
        log_debug(GUI, "Drawing Icon failed");
    }

    Rect16 rc_ico = Rect16(0, 0, wh_ico.x, wh_ico.y);
    rc_ico.Align(GetRect(), GetAlignment());
    rc_ico = rc_ico.Intersection(GetRect());
    display::DrawPng(point_ui16(rc_ico.Left(), rc_ico.Top()), *pRes, GetBackColor(), raster_op);
}

void window_icon_t::setRedLayout() {
    super::setRedLayout();
    SetHasIcon(); // alternative icon
}
void window_icon_t::setBlackLayout() {
    super::setBlackLayout();
    ClrHasIcon(); // normal icon
}

/*****************************************************************************/
//window_icon_button_t
window_icon_button_t::window_icon_button_t(window_t *parent, Rect16 rect, const png::Resource *res, ButtonCallback cb)
    : AddSuperWindow<window_icon_t>(parent, rect, res)
    , callback(cb) {
    SetBackColor(GuiDefaults::ClickableIconColorScheme);
    Enable();
}

void window_icon_button_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::CLICK) {
        callback();
    } else {
        SuperWindowEvent(sender, event, param);
    }
}

/*****************************************************************************/
//WindowMultiIconButton
WindowMultiIconButton::WindowMultiIconButton(window_t *parent, point_i16_t pt, const Pngs *res, ButtonCallback cb)
    : WindowMultiIconButton(
        parent,
        [pt, res] {
            if (!res || !res->normal.h || !res->normal.w)
                return Rect16();

            return Rect16(pt, res->normal.w, res->normal.h);
        }(),
        res, cb) {
}

WindowMultiIconButton::WindowMultiIconButton(window_t *parent, Rect16 rc, const Pngs *res, ButtonCallback cb)
    : AddSuperWindow<window_t>(parent, rc)
    , pRes(res)
    , callback(cb) {
    Enable();
}

void WindowMultiIconButton::unconditionalDraw() {
    // no PNG assigned
    if (!pRes)
        return;

    const png::Resource *pPng = &pRes->normal;
    if (IsShadowed())
        pPng = &pRes->disabled;
    if (IsFocused())
        pPng = &pRes->focused;

    FILE *file = pPng->Get();
    if (!file)
        return;

    display::DrawPng(point_ui16(Left(), Top()), *pPng, GetBackColor());
}

void WindowMultiIconButton::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::CLICK) {
        callback();
    } else {
        SuperWindowEvent(sender, event, param);
    }
}

/*****************************************************************************/
//window_icon_hourglass_t
window_icon_hourglass_t::window_icon_hourglass_t(window_t *parent, point_i16_t pt, padding_ui8_t padding, is_closed_on_click_t close)
    : AddSuperWindow<window_icon_t>(parent, &png::hourglass_26x39, pt, padding, close)
    , start_time(gui::GetTick())
    , animation_color(COLOR_ORANGE)
    , phase(0) {
}

struct Line {
    point_ui16_t first;
    point_ui16_t last;
    constexpr Line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
        : first({ x0, y0 })
        , last({ x1, y1 }) {
    }
};

struct LineColored : public Line {
    color_t color;
    constexpr LineColored(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, color_t clr)
        : Line(x0, y0, x1, y1)
        , color(clr) {
    }
};

void window_icon_hourglass_t::unconditionalDraw() {

    static constexpr color_t animation_color = COLOR_ORANGE;
    static constexpr color_t back_color = COLOR_BLACK;

    static constexpr LineColored lines[] = {
        { 13, 24, 13, 28, animation_color },
        { 11, 33, 14, 33, animation_color },
        { 9, 13, 17, 13, back_color },
        { 13, 19, 12, 23, animation_color },
        { 13, 29, 13, 33, animation_color },
        { 10, 33, 17, 33, animation_color },
        { 9, 13, 17, 13, back_color },
        { 10, 14, 16, 14, back_color },
        { 13, 24, 13, 28, animation_color },
        { 4, 33, 21, 33, animation_color },
        { 10, 32, 15, 32, animation_color },
        { 9, 13, 17, 13, back_color },
        { 10, 14, 16, 14, back_color },
        { 11, 15, 15, 15, back_color },
        { 11, 16, 14, 16, back_color },
        { 12, 17, 14, 17, back_color },
        { 12, 18, 13, 18, back_color },
        { 13, 26, 13, 33, animation_color },
        { 8, 31, 17, 31, animation_color },
        { 4, 32, 21, 32, animation_color },
        { 4, 33, 21, 33, animation_color }
    };

    auto begin = std::begin(lines);
    auto end = std::end(lines);

    switch (phase) {
    case 1:
        begin = &lines[0];
        end = &lines[2];
        break;
    case 2:
        begin = &lines[2];
        end = &lines[6];
        break;
    case 3:
        begin = &lines[6];
        end = &lines[11];
        break;
    case 4:
        begin = &lines[11];
        end = &lines[21];
        break;
    default:
        window_icon_t::unconditionalDraw();
        begin = &lines[0];
        end = &lines[0];
        break;
    }

    for (auto it = begin; it != end; ++it) {
        display::DrawLine(point_ui16(Left() + it->first.x, Top() + it->first.y), point_ui16(Left() + it->last.x, Top() + it->last.y), it->color);
    }
}

void window_icon_hourglass_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    uint8_t phs = ((gui::GetTick() - start_time) / ANIMATION_STEP_MS);
    phs %= ANIMATION_STEPS;
    if (phase != phs) {
        phase = phs;
        //do not want to call invalidate or Invalidate, it would reset phase to 0
        flags.invalid = true;
        gui_invalidate();
    }
}

void window_icon_hourglass_t::invalidate(Rect16 validation_rect) {
    phase = 0;
    super::invalidate(validation_rect);
}

/*****************************************************************************/
//WindowIcon_OkNg

//dash ok and nok must be same size
constexpr const png::Resource &id_res_na = png::dash_18x18;
constexpr const png::Resource &id_res_ok = png::ok_color_18x18;
constexpr const png::Resource &id_res_ng = png::nok_color_18x18;
constexpr const std::array<const png::Resource *, 4> id_res_ip = { { &png::spinner0_16x16, &png::spinner1_16x16, &png::spinner2_16x16, &png::spinner3_16x16 } };

//Icon rect is increased by padding, icon is centered inside it
WindowIcon_OkNg::WindowIcon_OkNg(window_t *parent, point_i16_t pt, SelftestSubtestState_t state, padding_ui8_t padding)
    : AddSuperWindow<window_aligned_t>(
        parent,
        [pt, padding] {
            return Rect16(pt,
                id_res_na.w + padding.left + padding.right,
                id_res_na.h + padding.top + padding.bottom);
        }())
    , state(state) {
    SetAlignment(Align_t::Center());
}

SelftestSubtestState_t WindowIcon_OkNg::GetState() const {
    return state;
}

void WindowIcon_OkNg::SetState(SelftestSubtestState_t s) {
    if (s != state) {
        state = s;
        Invalidate();
    }
}

void WindowIcon_OkNg::unconditionalDraw() {
    const png::Resource *id_res = nullptr;
    switch (GetState()) {
    case SelftestSubtestState_t::ok:
        id_res = &id_res_ok;
        break;
    case SelftestSubtestState_t::not_good:
        id_res = &id_res_ng;
        break;
    case SelftestSubtestState_t::undef:
        id_res = &id_res_na;
        break;
    case SelftestSubtestState_t::running: {
        const size_t blink_state = (flags.blink1 << 1) | flags.blink0; //sets 2 lowest bits guaranted to be 0 .. 3
        id_res = id_res_ip[blink_state];                               // no need to check index out of array range
    } break;
    }

    render_icon_align(GetRect(), id_res, GetBackColor(), GetAlignment());
}

void WindowIcon_OkNg::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (GetState() == SelftestSubtestState_t::running) {
        bool b0 = (gui::GetTick() / uint32_t(ANIMATION_STEP_MS)) & 0b01;
        bool b1 = (gui::GetTick() / uint32_t(ANIMATION_STEP_MS)) & 0b10;
        if (flags.blink0 != b0 || flags.blink1 != b1) {
            flags.blink0 = b0;
            flags.blink1 = b1;
            Invalidate();
        }
    }
}
