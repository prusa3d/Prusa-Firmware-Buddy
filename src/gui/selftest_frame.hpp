/**
 * @file selftest_frame.hpp
 * @author Radek Vana
 * @brief Interface for selftest frames (part of screen)
 * @date 2021-11-30
 */
#pragma once
#include "window_frame.hpp"
#include <common/fsm_base_types.hpp>
#include "radio_button.hpp"
#include "radio_button_fsm.hpp"
#include <guiconfig/wizard_config.hpp>
#include "window_text.hpp"
#include "window_wizard_icon.hpp"

/**
 * @brief parents of all tests
 * even supports "wizard questions"
 */
class SelftestFrame : public window_frame_t {
protected:
    PhasesSelftest phase_current;
    PhasesSelftest phase_previous;
    fsm::PhaseData data_current;
    fsm::PhaseData data_previous;

    virtual void change() {};
    virtual void pre_change() {}; // something to do before change

public:
    SelftestFrame(window_t *parent, PhasesSelftest ph, fsm::PhaseData data);
    void Change(PhasesSelftest ph, fsm::PhaseData data);
};

/**
 * @brief parent of non-tests frame with buttons
 */
class SelftestFrameWithRadio : public SelftestFrame {
protected:
    RadioButtonFSM radio;
    virtual void pre_change() override; // update radio button
public:
    SelftestFrameWithRadio(window_t *parent, PhasesSelftest ph, fsm::PhaseData data, size_t lines_of_footer = 0);
};

/**
 * @brief parent of normal tests without buttons
 */
class SelftestFrameNamed : public SelftestFrame {
    window_text_t test_name;

public:
    SelftestFrameNamed(window_t *parent, PhasesSelftest ph, fsm::PhaseData data, const string_view_utf8 &name);
    void SetName(const string_view_utf8 &txt);
};

/**
 * @brief parent of tests with buttons
 */
class SelftestFrameNamedWithRadio : public SelftestFrameNamed {
protected:
    RadioButtonFSM radio;
    virtual void pre_change() override; // update radio button
public:
    SelftestFrameNamedWithRadio(window_t *parent, PhasesSelftest ph, fsm::PhaseData data, const string_view_utf8 &name, size_t lines_of_footer = 0);
};
