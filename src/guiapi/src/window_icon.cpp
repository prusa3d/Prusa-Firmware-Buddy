/**
 * @file window_icon.cpp
 */

#include <unistd.h>
#include "window_icon.hpp"
#include "display.hpp"
#include "gui.hpp"
#include "ScreenHandler.hpp"
#include "img_resources.hpp"
#include "gui_invalidate.hpp"
#include "timing.h"

window_icon_t::window_icon_t(window_t *parent, Rect16 rect, const img::Resource *res, is_closed_on_click_t close)
    : window_aligned_t(parent, rect, win_type_t::normal, close)
    , pRes(res) {
    SetAlignment(Align_t::Center());
}

// Icon rect is increased by padding, icon is centered inside it
window_icon_t::window_icon_t(window_t *parent, const img::Resource *res, point_i16_t pt, padding_ui8_t padding, is_closed_on_click_t close)
    : window_icon_t(
        parent,
        [pt, res, padding] {
            if (!res || !res->h || !res->w) {
                return Rect16();
            }

            return Rect16(pt,
                res->w + padding.left + padding.right,
                res->h + padding.top + padding.bottom);
        }(),
        res, close) {
}

window_icon_t::window_icon_t(window_t *parent, const img::Resource *res, point_i16_t pt, Center center, size_t center_size, is_closed_on_click_t close)
    : window_icon_t(
        parent,
        [pt, res, center, center_size] {
            if (!res || !res->h || !res->w) {
                return Rect16();
            }

            Rect16 rc(pt, res->w, res->h);
            switch (center) {
            case Center::x:
                if (int(center_size) > (res->w + 1)) {
                    rc += Rect16::Left_t((center_size - res->w) / 2);
                }
                break;
            case Center::y:
                if (int(center_size) > (res->h + 1)) {
                    rc += Rect16::Top_t((center_size - res->h) / 2);
                }
                break;
            }

            return rc;
        }(),
        res, close) {
}

void window_icon_t::unconditional_draw(window_aligned_t *window, const img::Resource *image) {
    ropfn raster_op;
    raster_op.shadow = window->IsShadowed() ? is_shadowed::yes : is_shadowed::no;
    raster_op.swap_bw = window->IsFocused() ? has_swapped_bw::yes : has_swapped_bw::no;

    Rect16 rc_ico = Rect16(0, 0, image->w, image->h);
    rc_ico.Align(window->GetRect(), window->GetAlignment());
    rc_ico = rc_ico.Intersection(window->GetRect());
    display::draw_img(point_ui16(rc_ico.Left(), rc_ico.Top()), *image, window->GetBackColor(), raster_op);
}

void window_icon_t::unconditionalDraw() {
    // no image assigned
    if (!pRes) {
        return;
    }

    if (pRes->w < Width() || pRes->h < Height()) {
        window_aligned_t::unconditionalDraw(); // draw background
    }

    unconditional_draw(this, pRes);
}

void window_icon_t::set_layout(ColorLayout lt) {
    window_aligned_t::set_layout(lt);
    if (lt == ColorLayout::black) {
        ClrHasIcon(); // normal icon
    } else {
        SetHasIcon(); // alternative icon
    }
}

/*****************************************************************************/
// window_icon_button_t
window_icon_button_t::window_icon_button_t(window_t *parent, Rect16 rect, const img::Resource *res, ButtonCallback cb)
    : window_icon_t(parent, rect, res)
    , callback(cb) {
    SetRoundCorners();
    SetBackColor(GuiDefaults::ClickableIconColorScheme);
    Enable();
}

void window_icon_button_t::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    switch (event) {

    case GUI_event_t::CLICK:
    case GUI_event_t::TOUCH_CLICK:
        callback(*this);
        break;

    default:
        window_icon_t::windowEvent(sender, event, param);
        break;
    }
}

/*****************************************************************************/
// WindowMultiIconButton
WindowMultiIconButton::WindowMultiIconButton(window_t *parent, point_i16_t pt, const Pngs *res, ButtonCallback cb)
    : WindowMultiIconButton(
        parent,
        [pt, res] {
            if (!res || !res->normal.h || !res->normal.w) {
                return Rect16();
            }

            return Rect16(pt, res->normal.w, res->normal.h);
        }(),
        res, cb) {
}

WindowMultiIconButton::WindowMultiIconButton(window_t *parent, Rect16 rc, const Pngs *res, ButtonCallback cb)
    : window_t(parent, rc)
    , pRes(res)
    , callback(cb) {
    Enable();
}

void WindowMultiIconButton::unconditionalDraw() {
    if (!pRes) {
        return;
    }

    const img::Resource *pImg = &pRes->normal;
    if (IsFocused()) {
        pImg = &pRes->focused;
    }
    if (IsShadowed()) {
        pImg = &pRes->disabled;
    }

    display::draw_img(point_ui16(Left(), Top()), *pImg, GetBackColor());
}

void WindowMultiIconButton::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    switch (event) {

    case GUI_event_t::CLICK:
    case GUI_event_t::TOUCH_CLICK:
        callback(*this);
        break;

    default:
        window_t::windowEvent(sender, event, param);
        break;
    }
}

/*****************************************************************************/
// window_icon_hourglass_t
window_icon_hourglass_t::window_icon_hourglass_t(window_t *parent, point_i16_t pt, padding_ui8_t padding, is_closed_on_click_t close)
    : window_icon_t(parent, &img::hourglass_26x39, pt, padding, close)
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
    Color color;
    constexpr LineColored(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, Color clr)
        : Line(x0, y0, x1, y1)
        , color(clr) {
    }
};

void window_icon_hourglass_t::unconditionalDraw() {

    static constexpr Color animation_color = COLOR_ORANGE;
    static constexpr Color back_color = COLOR_BLACK;

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
        display::draw_line(point_ui16(Left() + it->first.x, Top() + it->first.y), point_ui16(Left() + it->last.x, Top() + it->last.y), it->color);
    }
}

void window_icon_hourglass_t::windowEvent([[maybe_unused]] window_t *sender, [[maybe_unused]] GUI_event_t event, [[maybe_unused]] void *param) {
    uint8_t phs = ((gui::GetTick() - start_time) / ANIMATION_STEP_MS);
    phs %= ANIMATION_STEPS;
    if (phase != phs) {
        phase = phs;
        // do not want to call invalidate or Invalidate, it would reset phase to 0
        flags.invalid = true;
        gui_invalidate();
    }
}

void window_icon_hourglass_t::invalidate(Rect16 validation_rect) {
    phase = 0;
    window_icon_t::invalidate(validation_rect);
}
