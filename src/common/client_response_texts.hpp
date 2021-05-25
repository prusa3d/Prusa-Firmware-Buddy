// client_response_texts.hpp
// texts for all types of response any Dialog can return

#pragma once

#include "client_response.hpp" //MAX_RESPONSES
#include <array>

using PhaseTexts = std::array<const char *, MAX_RESPONSES>;

//todo make some automatic checks names vs enum
//list of all button types
class BtnTexts {
    static const std::array<const char *, static_cast<size_t>(Response::_last) + 1> texts;

public:
    static constexpr const char *Get(Response resp) {
        return texts[static_cast<size_t>(resp)];
    }
};

extern const PhaseTexts ph_txt_stop;
extern const PhaseTexts ph_txt_continue;
extern const PhaseTexts ph_txt_none;
extern const PhaseTexts ph_txt_yesno;
