// st7789v.cpp
#include "st7789v_impl.h"
#include "st7789v.h"
#include <functional>
#include <cmath>
#include "Pin.hpp"

static OutputPin cs(IoPort::C, IoPin::p9, InitState::set, OMode::pushPull, OSpeed::high);
static OutputPin rs(IoPort::D, IoPin::p11, InitState::set, OMode::pushPull, OSpeed::high);
static OutputInputPin rst(IoPort::C, IoPin::p8, InitState::set, OMode::pushPull, OSpeed::low);

void st7789v_set_cs(void) {
    cs.write(GPIO_PinState::GPIO_PIN_SET);
    st7789v_flg |= FLG_CS;
}

void st7789v_clr_cs(void) {
    cs.write(GPIO_PinState::GPIO_PIN_RESET);
    st7789v_flg &= ~FLG_CS;
}

void st7789v_set_rs(void) {
    rs.write(GPIO_PinState::GPIO_PIN_SET);
    st7789v_flg |= FLG_RS;
}

void st7789v_clr_rs(void) {
    rs.write(GPIO_PinState::GPIO_PIN_RESET);
    st7789v_flg &= ~FLG_RS;
}

void st7789v_set_rst(void) {
    rst.write(GPIO_PinState::GPIO_PIN_SET);
    st7789v_flg |= FLG_RST;
}

void st7789v_clr_rst(void) {
    rst.write(GPIO_PinState::GPIO_PIN_RESET);
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
        while (!rstInput.read())
            delay++;
        if (irq)
            __enable_irq();
    }
    st7789v_set_rst();
    st7789v_reset_delay = delay;
}
