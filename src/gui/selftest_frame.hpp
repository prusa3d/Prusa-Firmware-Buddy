/**
 * @file selftest_frame.hpp
 * @author Radek Vana
 * @brief Interface for selftest frames (part of screen)
 * @date 2021-11-30
 */
#pragma once
#include "window_frame.hpp"
#include "fsm_base_types.hpp"
#include "DialogRadioButton.hpp"
#include "wizard_config.hpp"
#include "window_text.hpp"

/**
 * @brief parents of all tests
 * even supports "wizard questions"
 */
class SelftestFrame : public AddSuperWindow<window_frame_t> {
protected:
    PhasesSelftest phase_current;
    PhasesSelftest phase_previous;
    fsm::PhaseData data_current;
    fsm::PhaseData data_previous;

    virtual void change() {};
    virtual void pre_change() {}; //something to do before change

public:
    SelftestFrame(window_t *parent, PhasesSelftest ph, fsm::PhaseData data);
    void Change(PhasesSelftest ph, fsm::PhaseData data);
};

/**
 * @brief parent of non-tests frame with buttons
 */
class SelftestFrameWithRadio : public AddSuperWindow<SelftestFrame> {
protected:
    RadioButton radio;
    virtual void windowEvent(EventLock /*has private ctor*/, window_t * /*sender*/, GUI_event_t event, void *param);
    virtual void pre_change() override; //update radio button
public:
    SelftestFrameWithRadio(window_t *parent, PhasesSelftest ph, fsm::PhaseData data);
};

/**
 * @brief parent of normal tests without buttons
 */
class SelftestFrameNamed : public AddSuperWindow<SelftestFrame> {
    window_text_t test_name;

public:
    SelftestFrameNamed(window_t *parent, PhasesSelftest ph, fsm::PhaseData data, string_view_utf8 name);
};

/**
 * @brief parent of tests with buttons
 */
class SelftestFrameNamedWithRadio : public AddSuperWindow<SelftestFrameNamed> {
protected:
    RadioButton radio;
    virtual void windowEvent(EventLock /*has private ctor*/, window_t * /*sender*/, GUI_event_t event, void *param);
    virtual void pre_change() override; //update radio button
public:
    SelftestFrameNamedWithRadio(window_t *parent, PhasesSelftest ph, fsm::PhaseData data, string_view_utf8 name);
};
