/**
 * @file ifooter.hpp
 * @author Radek Vana
 * @brief abstaract footer
 * @date 2021-04-14
 */

#pragma once
#include "window_frame.hpp"
#include "footer_line.hpp"
#include "footer_item_union.hpp"

class IFooter : public AddSuperWindow<window_frame_t> {
public:
    IFooter(window_t *parent);

    // sets line 1, line 0 is const
    static bool SetSlot(FooterLine &line, size_t slot_id, footer::Item item);
    static bool SetSlotInit(size_t slot_id, footer::Item item);
    static footer::Item GetSlotInit(size_t slot_id);
};
