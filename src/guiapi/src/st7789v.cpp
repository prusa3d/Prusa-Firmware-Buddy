// st7789v.cpp
#include "st7789v_impl.h"
#include "st7789v.h"
#include <functional>
#include <cmath>
#include "hwio_pindef.h"

void st7789v_set_cs(void) {
    cs.write(Pin::State::high);
    st7789v_flg |= FLG_CS;
}

void st7789v_clr_cs(void) {
    cs.write(Pin::State::low);
    st7789v_flg &= ~FLG_CS;
}

void st7789v_set_rs(void) {
    rs.write(Pin::State::high);
    st7789v_flg |= FLG_RS;
}

void st7789v_clr_rs(void) {
    rs.write(Pin::State::low);
    st7789v_flg &= ~FLG_RS;
}

void st7789v_set_rst(void) {
    rst.write(Pin::State::high);
    st7789v_flg |= FLG_RST;
}

void st7789v_clr_rst(void) {
    rst.write(Pin::State::low);
    st7789v_flg &= ~FLG_RST;
}

void st7789v_reset(void) {
    st7789v_clr_rst();
    st7789v_delay_ms(15);
    volatile uint16_t delay = 0;
    {
        InputEnabler rstInput(rst, Pull::up);
        int irq = __get_PRIMASK() & 1;
        if (irq)
            __disable_irq();
        while (rstInput.read() == Pin::State::low)
            delay++;
        if (irq)
            __enable_irq();
    }
    st7789v_set_rst();
    st7789v_reset_delay = delay;
}
