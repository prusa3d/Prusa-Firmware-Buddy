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
#include <common/fsm_base_types.hpp>
#include "selftest_frame_axis.hpp"
#include "selftest_frame_fans.hpp"
#include "selftest_frame_fsensor.hpp"
#include "selftest_frame_gears_calib.hpp"
#include "selftest_frame_loadcell.hpp"
#include "selftest_frame_calib_z.hpp"
#include "selftest_frame_temp.hpp"
#include "selftest_frame_firstlayer.hpp"
#include "selftest_frame_firstlayer_questions.hpp"
#include "selftest_frame_result.hpp"
#include "selftest_frame_wizard_prologue.hpp"
#include "selftest_frame_wizard_epilogue.hpp"
#include "selftest_frame_dock.hpp"
#include "selftest_frame_tool_offsets.hpp"
#include "selftest_invalid_state.hpp"
#include "static_alocation_ptr.hpp"
#include "printer_selftest.hpp" // SelftestMask_t

class ScreenSelftest : public screen_t {
    using mem_space = std::aligned_union<0, ScreenSelftestInvalidState, SelftestFrametAxis, SelftestFrameFans, SelftestFrameFSensor, SelftestFrameGearsCalib, SelftestFrameLoadcell, ScreenSelftestTemp, SelftestFrameCalibZ, SelftestFrameFirstLayerQuestions, SelftestFrameResult
#if BOARD_IS_BUDDY
        ,
        SelftestFrameFirstLayer
#endif
        >::type;

    mem_space all_tests;

    template <typename T>
    static static_unique_ptr<SelftestFrame> creator(ScreenSelftest &rThs, PhasesSelftest phase, fsm::PhaseData data) {
        static_assert(sizeof(T) <= sizeof(rThs.all_tests), "Error selftest part does not fit");
        return make_static_unique_ptr<T>(&rThs.all_tests, &rThs, phase, data);
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
