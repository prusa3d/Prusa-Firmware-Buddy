/// @file
#include "window_icon.hpp"

#include <array>
#include "display.hpp"
#include "gui_time.hpp"
#include "img_resources.hpp"

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

    display::draw_img(point_ui16(Left(), Top()), *pImg, GetBackColor(), ropfn {});
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

constexpr std::array hourglass_stages {
    &img::hourglass0_26x39,
    &img::hourglass1_26x39,
    &img::hourglass2_26x39,
    &img::hourglass3_26x39,
    &img::hourglass4_26x39,
};

window_icon_hourglass_t::window_icon_hourglass_t(window_t *parent, point_i16_t pt)
    : window_icon_t {
        parent,
        hourglass_stages[0],
        pt,
        { 0, 0, 0, 0 },
        is_closed_on_click_t::no,
    } {}

void window_icon_hourglass_t::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::LOOP: {
        constexpr uint32_t stage_duration_ms = 500;
        const uint32_t gui_tick = gui::GetTick();
        const size_t stage = (gui_tick / stage_duration_ms) % hourglass_stages.size();
        SetRes(hourglass_stages[stage]);
        break;
    }

    default:
        break;
    }

    window_icon_t::windowEvent(sender, event, param);
}
