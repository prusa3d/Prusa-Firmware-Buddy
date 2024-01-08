#include "screen_touch_playground.hpp"

#include "str_utils.hpp"
#include <hw/touchscreen/touchscreen.hpp>
#include <ScreenHandler.hpp>

ScreenTouchPlayground::ScreenTouchPlayground()
    : text(this, Rect16(4, 4, display::GetW() - 8, 64), is_multiline::yes)
    , touch_rect(this, {}) {

    text.set_font(Font::normal);
    text.SetText(string_view_utf8::MakeRAM(reinterpret_cast<const uint8_t *>(text_content.data())));
}

void ScreenTouchPlayground::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::LOOP) {
        auto last_event = touchscreen.get_last_event();

        {
            decltype(text_content) new_text;
            StringBuilder sb(new_text);

            sb.append_printf("Last event: %s (%i %i)\n", GUI_event_prt(last_event.type), last_event.pos_x, last_event.pos_y);
            sb.append_printf("Err count: %li\n", (long)touchscreen.total_read_error_count());

            if (strcmp(text_content.data(), new_text.data())) {
                text_content = new_text;
                text.Invalidate();
                Invalidate();
            }
        }

        {
            static constexpr int radius = 4;
            const Rect16 new_touch_rect(last_event.pos_x - radius, last_event.pos_y - radius, radius * 2 + 1, radius * 2 + 1);

            if (touch_rect.GetRectWithoutTransformation() != new_touch_rect) {
                Invalidate(touch_rect.GetRect());
                touch_rect.SetRect(new_touch_rect);
                touch_rect.Invalidate();
            }
        }
    }

    else if (event == GUI_event_t::CLICK) {
        Screens::Access()->Close();
        return;
    }

    SuperWindowEvent(sender, event, param);
}
