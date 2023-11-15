// client_response_texts.hpp
// texts for all types of response any Dialog can return

#pragma once

#include "client_response.hpp" //MAX_RESPONSES
#include "guitypes.hpp"
#include <array>
#include <utility_extensions.hpp>

using PhaseTexts = std::array<const char *, MAX_RESPONSES>;
using BtnResource = std::pair<const char *, const img::Resource *>;

// todo make some automatic checks names vs enum
// list of all button types
class BtnResponse {
    static const std::array<BtnResource, ftrstd::to_underlying(Response::_count)> texts_and_icons;

public:
    static constexpr const char *GetText(Response resp) {
        return texts_and_icons[ftrstd::to_underlying(resp)].first;
    }
    static constexpr const img::Resource *GetIconId(Response resp) {
        return texts_and_icons[ftrstd::to_underlying(resp)].second;
    }
};

extern const PhaseTexts ph_txt_stop;
extern const PhaseTexts ph_txt_continue;
extern const PhaseTexts ph_txt_continue_stop;
extern const PhaseTexts ph_txt_none;
extern const PhaseTexts ph_txt_yesno;
