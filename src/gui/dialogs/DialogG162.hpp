#pragma once

#include "DialogStateful.hpp"

class DialogG162 : public DialogStateful<PhasesG162> {
public:
    DialogG162(string_view_utf8 name);
};
