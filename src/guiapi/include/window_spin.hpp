// window_spin.hpp

#pragma once

#include "window.hpp"
#include "window_numb.hpp"

struct window_spin_t : public window_numb_t {
    float min;
    float max;
    float step;
    int count;
    int index;

    int GetItemCount() const { return count; }
    void SetItemIndex(int idx);
    int GetItemIndex() const { return index; }

    void SetValue(float val); //todo erase me, virtual methods does not work yet - stupid memcpy
protected:
    //todo use this virtual methods does not work yet - stupid memcpy
    //virtual void setValue(float val) override;
};

struct window_class_spin_t {
    window_class_numb_t cls;
};

extern const window_class_spin_t window_class_spin;
