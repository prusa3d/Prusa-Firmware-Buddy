#include "catch2/catch.hpp"

#include "Jogwheel.hpp"
#include "hwio_pindef.h"
#include "gui_time.hpp" //  gui::GetTick

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
        if (jogWheelEN1.state == Pin::State::low && jogWheelEN2.state == Pin::State::low) {
            return phase_t::P0lo_P1lo;
        }
        if (jogWheelEN1.state == Pin::State::high && jogWheelEN2.state == Pin::State::low) {
            return phase_t::P0hi_P1lo;
        }
        if (jogWheelEN1.state == Pin::State::high && jogWheelEN2.state == Pin::State::high) {
            return phase_t::P0hi_P1hi;
        }
        if (jogWheelEN1.state == Pin::State::low && jogWheelEN2.state == Pin::State::high) {
            return phase_t::P0lo_P1hi;
        }

        // cannot happen, avoid warning
        return phase_t::P0lo_P1lo;
    }

    void SetEncoderPhase(phase_t ph, uint32_t ms) {
        while (ms--) {
            SetEncoderPhase(ph);
        }
    }

    void QuaterSpinR(uint32_t cnt, uint32_t ms = 2) { // 2 ms will pass noise filter
        while (cnt--) {
            const phase_t ph = Next(GetEncoderPhase());
            SetEncoderPhase(ph, ms);
        }
    }

    void QuaterSpinL(uint32_t cnt, uint32_t ms = 2) { // 2 ms will pass noise filter
        while (cnt--) {
            const phase_t ph = Prev(GetEncoderPhase());
            SetEncoderPhase(ph, ms);
        }
    }

    void SpinR(uint32_t cnt = 1, uint32_t ms = 2) { // 2 ms will pass noise filter
        QuaterSpinR(cnt * 4, ms);
    }

    void SpinL(uint32_t cnt = 1, uint32_t ms = 2) { // 2 ms will pass noise filter
        QuaterSpinL(cnt * 4, ms);
    }
};

TEST_CASE("Jogwheel tests", "[jogwheel]") {
    BtnState_t ev;
    TestJogwheel j;

    // without ConsumeButtonEvent there should be no click or encoder change
    SECTION("uninitialized jogwheel test") {

        j.SetEncoderPhase(phase_t::P0lo_P1lo);

        REQUIRE(j.GetEncoder() == 0); // read at tick 0

        j.SpinR(1, 2); // 2 ms  not filtered out
        REQUIRE(j.GetEncoder() == 0); // not initialized by read, must return 0

        jogWheelENC.state = Pin::State::high; // inverted
        j.Update1msFromISR();

        jogWheelENC.state = Pin::State::low; // inverted
        j.Update1msFromISR();

        jogWheelENC.state = Pin::State::high; // inverted
        j.Update1msFromISR();

        jogWheelENC.state = Pin::State::low; // inverted
        j.Update1msFromISR();

        REQUIRE_FALSE(j.ConsumeButtonEvent(ev)); // clicks before first read must be discarded
    }

    j.ConsumeButtonEvent(ev); // this call will initialize queue

    SECTION("encoder - noise filter") {
        j.SetEncoderPhase(phase_t::P0lo_P1lo);

        REQUIRE(j.GetEncoder() == 0); // read at tick 0

        j.SpinR(1, 0); // 0 ms must be filtered out
        REQUIRE(j.GetEncoder() == 0); // must still be 0, noise filter filtered spin out

        j.SpinR(1, 1); // 1 ms must be filtered out
        REQUIRE(j.GetEncoder() == 0); // must still be 0, noise filter filtered spin out

        j.SpinR(1, 2); // 2 ms must not be filtered out
        REQUIRE_FALSE(j.GetEncoder() == 0); // tick did not changed, must read 0
    }

    SECTION("button test") {

        jogWheelENC.state = Pin::State::high; // inverted
        j.Update1msFromISR();

        jogWheelENC.state = Pin::State::low; // inverted
        j.Update1msFromISR();

        // REQUIRE(j.ConsumeButtonEvent(ev)); // clicked
    }
}
