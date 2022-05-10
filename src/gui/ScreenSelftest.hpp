/**
 * @file ScreenSelftest.hpp
 * @author Radek Vana
 * @brief parent of all selftest screens
 * @date 2021-11-25
 */
#pragma once
#include "gui.hpp"
#include "screen.hpp"
#include "window_header.hpp"
#include "status_footer.hpp"
#include "fsm_base_types.hpp"
#include "selftest_frame_esp.hpp"
#include "selftest_frame_esp_progress.hpp"
#include "selftest_frame_esp_qr.hpp"
#include "selftest_invalid_state.hpp"
#include "static_alocation_ptr.hpp"

class ScreenSelftest : public AddSuperWindow<screen_t> {
    using mem_space = std::aligned_union<0, SelftestFrameESP, SelftestFrameESP_progress, SelftestFrameESP_qr, ScreenSelftestInvalidState>::type;
    mem_space all_tests;

    //safer than make_static_unique_ptr, checks storage size
    template <class T, class... Args>
    static_unique_ptr<SelftestFrame> makePtr(Args &&... args) {
        static_assert(sizeof(T) <= sizeof(all_tests), "Error selftest part does not fit");
        return make_static_unique_ptr<T>(&all_tests, std::forward<Args>(args)...);
    }

    using fnc = static_unique_ptr<SelftestFrame> (*)(ScreenSelftest &rThs, PhasesSelftest phase, fsm::PhaseData data); //function pointer definition

    //define factory methods for all dialogs here
    static static_unique_ptr<SelftestFrame> creator_esp(ScreenSelftest &rThs, PhasesSelftest phase, fsm::PhaseData data);
    static static_unique_ptr<SelftestFrame> creator_esp_progress(ScreenSelftest &rThs, PhasesSelftest phase, fsm::PhaseData data);
    static static_unique_ptr<SelftestFrame> creator_esp_qr(ScreenSelftest &rThs, PhasesSelftest phase, fsm::PhaseData data);
    static static_unique_ptr<SelftestFrame> creator_invalid(ScreenSelftest &rThs, PhasesSelftest phase, fsm::PhaseData data);

    fnc Get(SelftestParts part); //returns factory method

    static_unique_ptr<SelftestFrame> ptr;

    static string_view_utf8 getCaption(SelftestParts part);
    static uint16_t getIconId(SelftestParts part);

private:
    static constexpr const char *en_wizard_ok = N_("WIZARD - OK");
    static constexpr const char *en_selftest = N_("SELFTEST");
    static constexpr const char *en_esp = N_("WI-FI MODULE");
    static constexpr const char *error = "ERROR"; // do not translate

    window_header_t header;
    static ScreenSelftest *ths;

    SelftestParts part_current;
    SelftestParts part_previous;

public:
    ScreenSelftest();
    ~ScreenSelftest();
    static ScreenSelftest *GetInstance();
    void Change(fsm::BaseData data);

    virtual void InitState(screen_init_variant var) override; // opens selftest, needed for synchronization
    // GetCurrentState is not overriden, to prevent selftest reopen
    // default behavior (not saving state) is valid
};
