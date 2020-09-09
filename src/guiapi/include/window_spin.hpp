// window_spin.hpp

#pragma once

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

    void SetMin(float min_val);
    void SetMax(float max_val);
    void SetStep(float val);
    void SetMinMax(float min_val, float max_val);
    void SetMinMaxStep(float min_val, float max_val, float step_val);
    void SetValMinMaxStep(float val, float min_val, float max_val, float step_val);

    float GetMin() const { return min; }
    float GetMax() const { return max; }
    float GetStep() const { return step; }
    float GetCount() const { return count; }
    float GetIndex() const { return index; }

    window_spin_t(window_t *parent, Rect16 rect);

protected:
    void setValMinMaxStep(float val, float min_val, float max_val, float step_val);
    virtual void setValue(float val) override;
};
