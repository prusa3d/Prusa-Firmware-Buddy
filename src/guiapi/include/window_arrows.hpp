#pragma once

#include "window.hpp"

class WindowArrows : public window_aligned_t {
public:
    enum class State_t : uint8_t { undef,
        up,
        down };
    WindowArrows(window_t *parent, point_i16_t pt, padding_ui8_t padding = { 0, 0, 0, 0 });
    WindowArrows::State_t GetState() const;
    void SetState(WindowArrows::State_t s);

protected:
    virtual void unconditionalDraw() override;

private:
    State_t state;
};
