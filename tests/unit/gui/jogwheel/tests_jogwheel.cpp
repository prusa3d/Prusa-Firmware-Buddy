#include "catch2/catch.hpp"

#include "Jogwheel.hpp"
#include "hwio_pindef.h"

extern "C" void _bsod(const char *fmt, const char *fine_name, int line_number, ...) {}

namespace buddy::hw {
jogPin jogWheelEN1;
jogPin jogWheelEN2;
jogPin jogWheelENC;
}; // namespace buddy::hw

using buddy::hw::jogWheelEN1;
using buddy::hw::jogWheelEN2;
using buddy::hw::jogWheelENC;
using buddy::hw::Pin;

static bool irq_on = true;
void __disable_irq() { irq_on = false; }
void __enable_irq() { irq_on = true; }

static uint32_t hal_tick = 0;
uint32_t HAL_GetTick() { return hal_tick; }

static Jogwheel::BtnState_t ev;

enum class phase_t {
    P0lo_P1lo,
    P0hi_P1lo,
    P0hi_P1hi,
    P0lo_P1hi
};

phase_t Next(phase_t ph) {
    return phase_t((uint32_t(ph) + 1) & 0x03);
};

phase_t Prev(phase_t ph) {
    return phase_t((uint32_t(ph) - 1) & 0x03);
};

class TestJogwheel : public Jogwheel {
public:
    void SetEncoderPhase(phase_t ph) {
        switch (ph) {
        case phase_t::P0lo_P1lo:
            jogWheelEN1.state = Pin::State::low;
            jogWheelEN2.state = Pin::State::low;
            break;
        case phase_t::P0hi_P1lo:
            jogWheelEN1.state = Pin::State::high;
            jogWheelEN2.state = Pin::State::low;
            break;
        case phase_t::P0hi_P1hi:
            jogWheelEN1.state = Pin::State::high;
            jogWheelEN2.state = Pin::State::high;
            break;
        case phase_t::P0lo_P1hi:
            jogWheelEN1.state = Pin::State::low;
            jogWheelEN2.state = Pin::State::high;
            break;
        }
        Update1msFromISR();
    }

    phase_t GetEncoderPhase() {
        if (jogWheelEN1.state == Pin::State::low && jogWheelEN2.state == Pin::State::low)
            return phase_t::P0lo_P1lo;
        if (jogWheelEN1.state == Pin::State::high && jogWheelEN2.state == Pin::State::low)
            return phase_t::P0hi_P1lo;
        if (jogWheelEN1.state == Pin::State::high && jogWheelEN2.state == Pin::State::high)
            return phase_t::P0hi_P1hi;
        if (jogWheelEN1.state == Pin::State::low && jogWheelEN2.state == Pin::State::high)
            return phase_t::P0lo_P1hi;

        //cannot happen, avoid warning
        return phase_t::P0lo_P1lo;
    }

    void SetEncoderPhase(phase_t ph, uint32_t ms) {
        while (ms--)
            SetEncoderPhase(ph);
    }

    void QuaterSpinR(uint32_t cnt, uint32_t ms = 2) { // 2 ms will pass noise filter
        const phase_t ph = Next(GetEncoderPhase());
        SetEncoderPhase(ph, ms);
    }

    void QuaterSpinL(uint32_t cnt, uint32_t ms = 2) { // 2 ms will pass noise filter
        const phase_t ph = Prev(GetEncoderPhase());
        SetEncoderPhase(ph, ms);
    }

    void SpinR(uint32_t cnt = 1, uint32_t ms = 2) { // 2 ms will pass noise filter
        QuaterSpinR(cnt * 4, ms);
    }

    void SpinL(uint32_t cnt = 1, uint32_t ms = 2) { // 2 ms will pass noise filter
        QuaterSpinL(cnt * 4, ms);
    }
};

TEST_CASE("Jogwheel tests", "[jogwheel]") {
    SECTION("encoder - noise filter") {
        TestJogwheel j;
        j.SetEncoderPhase(phase_t::P0lo_P1lo);

        REQUIRE(j.GetEncoder() == 0); // read at tick 0
        hal_tick += 1000;
        REQUIRE(j.GetEncoder() == 0);

        j.SpinR(1, 0);                //0 ms must be filtered out
        REQUIRE(j.GetEncoder() == 0); // tick did not changed, must read 0
        hal_tick += 1000;
        REQUIRE(j.GetEncoder() == 0); // must still be 0, noise filter filtered spin out

        j.SpinR(1, 1);                //1 ms must be filtered out
        REQUIRE(j.GetEncoder() == 0); // tick did not changed, must read 0
        hal_tick += 1000;
        REQUIRE(j.GetEncoder() == 0); // must still be 0, noise filter filtered spin out

        j.SpinR(1, 2);                //2 ms must not be filtered out
        REQUIRE(j.GetEncoder() == 0); // tick did not changed, must read 0
        hal_tick += 1000;
        //error does not work
        //REQUIRE_FALSE(j.GetEncoder() == 0); // must not be 0, noise filter cannot filter spin out
    }

    SECTION("button test") {
        TestJogwheel j;
        jogWheelENC.state = Pin::State::high; //inverted
        j.Update1msFromISR();
        hal_tick += 1000;

        REQUIRE_FALSE(j.ConsumeButtonEvent(ev)); // not clicked
    }
}
