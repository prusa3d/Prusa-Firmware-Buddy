#pragma once

#include "DialogStateful.hpp"

class DialogSelftestFansAxis : public DialogStateful<PhasesSelfTest> {
public:
    DialogSelftestFansAxis(string_view_utf8 name);
};
