// window_icon.c
#include "window_icon.hpp"
#include "gui.hpp"
#include "ScreenHandler.hpp"
#include "guitypes.hpp"
#include "resource.h"

void window_icon_init(window_icon_t *window) {
    window->color_back = COLOR_BLACK;
    window->id_res = 0;
    window->alignment = ALIGN_CENTER;
}

void window_icon_draw(window_icon_t *window) {
    if (window->IsInvalid()) {
        //point_ui16_t pt = {window->rect.Left(), window->rect.Top()};
        //display::DrawIcon(pt, window->id_res, window->color_back, (window->IsFocused())?ROPFN_SWAPBW:0);
        uint8_t ropfn = 0;
        if ((window->IsShadowed())) { // that could not be set, but what if
            ropfn |= ROPFN_DISABLE;
        }
        if ((window->IsFocused())) {
            ropfn |= ROPFN_SWAPBW;
        }

        render_icon_align(window->rect, window->id_res, window->color_back,
            RENDER_FLG(window->alignment, ropfn));
        window->Validate();
        ;
    }
}

void window_icon_t::SetIdRes(int16_t id) {
    id_res = id;
    Invalidate();
}

window_icon_t::window_icon_t(window_t *parent, Rect16 rect, uint16_t id_res, is_closed_on_click_t close)
    : window_t(parent, rect, is_dialog_t::no, close)
    , id_res(id_res)
    , alignment(ALIGN_CENTER) {
}
//Icon rect is increased by padding, icon is centered inside it
window_icon_t::window_icon_t(window_t *parent, uint16_t id_res, point_i16_t pt, padding_ui8_t padding, is_closed_on_click_t close)
    : window_icon_t(
        parent,
        [pt, id_res, padding] {
            size_ui16_t sz = CalculateMinimalSize(id_res);
            if (!(sz.h && sz.w))
                return Rect16();
            return Rect16(pt,
                sz.w + padding.left + padding.right,
                sz.h + padding.top + padding.bottom);
        }(),
        id_res, close) {
}

void window_icon_t::unconditionalDraw() {
    uint8_t ropfn = 0;
    if (IsShadowed()) { // that could not be set, but what if
        ropfn |= ROPFN_DISABLE;
    }
    if (IsFocused()) {
        ropfn |= ROPFN_SWAPBW;
    }

    render_icon_align(rect, id_res, color_back, RENDER_FLG(alignment, ropfn));
}

bool window_icon_t::IsShadowed() const { return flag_custom0 == true; }
void window_icon_t::Shadow() { flag_custom0 = true; }
void window_icon_t::Unshadow() { flag_custom0 = false; }

size_ui16_t window_icon_t::CalculateMinimalSize(uint16_t id_res) {
    size_ui16_t ret = size_ui16(0, 0);
    if (!id_res)
        return ret;
    const uint8_t *p_icon = resource_ptr(id_res);
    if (!p_icon)
        return ret;
    ret = icon_size(p_icon);
    return ret;
}

/*****************************************************************************/
//window_icon_button_t
window_icon_button_t::window_icon_button_t(window_t *parent, Rect16 rect, uint16_t id_res, ButtonCallback cb)
    : window_icon_t(parent, rect, id_res)
    , callback(cb) {
    Enable();
}

void window_icon_button_t::windowEvent(window_t *sender, uint8_t event, void *param) {
    if (event == WINDOW_EVENT_CLICK) {
        callback();
    } else {
        window_icon_t::windowEvent(sender, event, param);
    }
}

/*****************************************************************************/
//window_icon_hourglass_t
window_icon_hourglass_t::window_icon_hourglass_t(window_t *parent, point_i16_t pt, padding_ui8_t padding, is_closed_on_click_t close)
    : window_icon_t(parent, IDR_PNG_wizard_icon_hourglass, pt, padding, close)
    , start_time(HAL_GetTick())
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
        display::DrawLine(point_ui16(rect.Left() + it->first.x, rect.Top() + it->first.y), point_ui16(rect.Left() + it->last.x, rect.Top() + it->last.y), it->color);
    }
}

void window_icon_hourglass_t::windowEvent(window_t *sender, uint8_t event, void *param) {
    uint8_t phs = ((HAL_GetTick() - start_time) / ANIMATION_STEP_MS);
    phs %= ANIMATION_STEPS;
    if (phase != phs) {
        phase = phs;
        Invalidate();
    }
}
