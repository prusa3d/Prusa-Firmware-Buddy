// window_spin.c
#include "window_spin.hpp"
#include "gui.hpp"
#include "ScreenHandler.hpp"
#include <algorithm>

extern osThreadId displayTaskHandle;

void window_spin_inc(window_spin_t *window, int dif);
void window_spin_dec(window_spin_t *window, int dif);

void window_spin_event(window_spin_t *window, uint8_t event, void *param) {
    switch (event) {
    case WINDOW_EVENT_BTN_DN:
        if (window->IsEnabled())
            Screens::Access()->ScreenEvent((window_t *)window, WINDOW_EVENT_CHANGE, nullptr);
        if (window->GetParent())
            window->GetParent()->SetCapture();
        break;
    case WINDOW_EVENT_ENC_DN:
        window_spin_dec(window, (int)param);
        break;
    case WINDOW_EVENT_ENC_UP:
        window_spin_inc(window, (int)param);
        break;
    case WINDOW_EVENT_CAPT_0:
    case WINDOW_EVENT_CAPT_1:
        window->Invalidate();
        break;
    }
}

void window_spin_inc(window_spin_t *window, int dif) {
    window->index += dif;
    if (window->index >= window->count)
        window->index = window->count - 1;
    window->value = window->min + window->index * window->step;
    window->Invalidate();
}

void window_spin_dec(window_spin_t *window, int dif) {
    window->index -= dif;
    if (window->index < 0)
        window->index = 0;
    window->value = window->min + window->index * window->step;
    window->Invalidate();
}

void window_spin_t::SetItemIndex(int idx) {
    if (count > idx) {
        index = idx;
        value = min + step * index;
    }
    Invalidate();
}

void window_spin_t::setValue(float val) {
    SetValMinMaxStep(val, min, max, step);
}

void window_spin_t::SetMin(float min_val) {
    SetValMinMaxStep(value, min_val, max, step);
    Invalidate();
}

void window_spin_t::SetMax(float max_val) {
    SetValMinMaxStep(value, min, max_val, step);
    Invalidate();
}

void window_spin_t::SetStep(float step_val) {
    SetValMinMaxStep(value, min, max, step_val);
    Invalidate();
}

void window_spin_t::SetMinMax(float min_val, float max_val) {
    SetValMinMaxStep(value, min_val, max_val, step);
    Invalidate();
}

void window_spin_t::SetMinMaxStep(float min_val, float max_val, float step_val) {
    SetValMinMaxStep(value, min_val, max_val, step_val);
    Invalidate();
}

void window_spin_t::SetValMinMaxStep(float val, float min_val, float max_val, float step_val) {
    setValMinMaxStep(val, min_val, max_val, step_val);
    Invalidate();
}

void window_spin_t::setValMinMaxStep(float val, float min_val, float max_val, float step_val) {
    min = min_val;
    max = max_val;
    step = step_val;
    value = std::max(min, std::min(max, val)); //do not have C++ 17, cannot use clamp
    //value = std::clamp(val, min,max); // need C++ 17
    count = (int)((max - min) / step + 1.5F);
    index = (int)((value - min) / step);
}

window_spin_t::window_spin_t(window_t *parent, Rect16 rect)
    : window_numb_t(parent, rect)
    , min(0.0)
    , max(100.0F)
    , step(1.0F)
    , count(101)
    , index(0) {
    Enable();
}
