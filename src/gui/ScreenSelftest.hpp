#pragma once

#include <array>

#include "gui.hpp"
#include "screen.hpp"
#include "window_header.hpp"
#include "status_footer.hpp"
#include <common/fsm_base_types.hpp>
#include "static_alocation_ptr.hpp"
#include "printer_selftest.hpp" // SelftestMask_t
#include <selftest_frame.hpp>

class ScreenSelftest : public screen_t {
#if PRINTER_IS_PRUSA_XL()
    static constexpr size_t storage_size = 2048;
#else
    static constexpr size_t storage_size = 1560;
#endif
    alignas(std::max_align_t) std::array<uint8_t, storage_size> storage;

    template <typename T>
    static static_unique_ptr<SelftestFrame> creator(ScreenSelftest &rThs, PhasesSelftest phase, fsm::PhaseData data) {
        static_assert(sizeof(T) <= storage_size, "Error selftest part does not fit");
        return make_static_unique_ptr<T>(rThs.storage.data(), &rThs, phase, data);
    }

    using fnc = static_unique_ptr<SelftestFrame> (*)(ScreenSelftest &rThs, PhasesSelftest phase, fsm::PhaseData data); // function pointer definition
    fnc Get(SelftestParts part); // returns factory method

    static_unique_ptr<SelftestFrame> ptr;

    static string_view_utf8 getCaption(SelftestParts part);
    static const img::Resource *getIconId(SelftestParts part);

private:
    static constexpr const char *en_wizard = N_("WIZARD");
    static constexpr const char *en_wizard_ok = N_("WIZARD - OK");
    static constexpr const char *en_wizard_nok = N_("WIZARD - NOK");
    static constexpr const char *en_selftest = N_("SELFTEST");
    static constexpr const char *en_firstlay = N_("FIRST LAYER CALIBRATION");
    static constexpr const char *error = "ERROR"; // do not translate

    window_header_t header;
    static ScreenSelftest *ths;

    SelftestParts part_current;
    SelftestParts part_previous;

public:
    ScreenSelftest();
    void Change(fsm::BaseData data);

    virtual void InitState(screen_init_variant var) override; // opens selftest, needed for synchronization
    // GetCurrentState is not overriden, to prevent selftest reopen
    // default behavior (not saving state) is valid
};
