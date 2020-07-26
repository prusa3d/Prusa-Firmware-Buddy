// dialog_response.hpp
// texts for all types of response any Dialog can return

#pragma once

#include "general_response.hpp"
#include <array>

//todo make some automatic checks names vs enum
//list of all button types
class BtnTexts {
    static const std::array<const char *, static_cast<size_t>(Response::_last) + 1> texts;

public:
    static const char *Get(Response resp);
};
