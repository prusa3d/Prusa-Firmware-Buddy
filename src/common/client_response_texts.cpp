// client_response_texts.cpp
// texts for all types of response any Dialog can return

#include "client_response_texts.hpp"

/*****************************************************************************/
// clang-format off
const PhaseTexts ph_txt_stop          = { get_response_text(Response::Stop),     get_response_text(Response::_none), get_response_text(Response::_none), get_response_text(Response::_none) };
const PhaseTexts ph_txt_continue      = { get_response_text(Response::Continue), get_response_text(Response::_none), get_response_text(Response::_none), get_response_text(Response::_none) };
const PhaseTexts ph_txt_continue_stop = { get_response_text(Response::Continue), get_response_text(Response::Stop),  get_response_text(Response::_none), get_response_text(Response::_none) };
const PhaseTexts ph_txt_none          = { get_response_text(Response::_none),    get_response_text(Response::_none), get_response_text(Response::_none), get_response_text(Response::_none) };
const PhaseTexts ph_txt_yesno         = { get_response_text(Response::Yes),      get_response_text(Response::No),    get_response_text(Response::_none), get_response_text(Response::_none) };
// clang-format on
/*****************************************************************************/
