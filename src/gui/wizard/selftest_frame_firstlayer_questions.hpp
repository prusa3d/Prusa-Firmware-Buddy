/**
 * @file selftest_frame_firstlayer_questions.hpp
 * @brief part of screen containing questions before / after first layer calibration
 */

#pragma once

#include "selftest_frame.hpp"
#include "status_footer.hpp"

class SelftestFrameFirstLayerQuestions : public AddSuperWindow<SelftestFrameWithRadio> {
    StatusFooter footer;

    window_text_t text; // in middle of screen

    std::array<char, 21 * 9 + 1> txt_buff; // 21 columns, 9 lines + '\0' char
protected:
    virtual void change() override;

public:
    SelftestFrameFirstLayerQuestions(window_t *parent, PhasesSelftest ph, fsm::PhaseData data);
};
