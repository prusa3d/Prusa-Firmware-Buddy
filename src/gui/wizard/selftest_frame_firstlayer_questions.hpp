/**
 * @file selftest_frame_firstlayer_questions.hpp
 * @brief part of screen containing questions before / after first layer calibration
 */

#pragma once

#include "selftest_frame.hpp"
#include "status_footer.hpp"

class SelftestFrameFirstLayerQuestions : public SelftestFrameWithRadio {
    StatusFooter footer;

    window_text_t text; // in middle of screen

    StringViewUtf8Parameters<10> params;

protected:
    virtual void change() override;

public:
    SelftestFrameFirstLayerQuestions(window_t *parent, PhasesSelftest ph, fsm::PhaseData data);
};
