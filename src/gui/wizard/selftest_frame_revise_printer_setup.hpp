#pragma once

#include <selftest_frame.hpp>
#include <window_icon.hpp>
#include <window_text.hpp>

class SelftestFrameRevisePrinterSetup : public SelftestFrameWithRadio {

public:
    SelftestFrameRevisePrinterSetup(window_t *parent, PhasesSelftest ph, fsm::PhaseData data);

protected:
    void change() override;

private:
    window_text_t text;
};
