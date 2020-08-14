/*  window_roll_text.hpp
*   \brief used in texts that are too long for standart display width
*
*  Created on: May 6, 2020
*      Author: Migi - michal.rudolf<at>prusa3d.cz
*/

#pragma once

#include "window_text.hpp"
#include "display_helper.h"

struct window_roll_text_t : window_text_t {
    txtroll_t roll;
    window_roll_text_t(window_t *parent, Rect16 rect);
};
