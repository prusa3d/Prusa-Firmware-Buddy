/**
 * @file screen_menu_move.cpp
 */

#include "screen_menu_move.hpp"
#include "screen_menu_move_utils.hpp"
#include "marlin_client.hpp"
#include "menu_spin_config.hpp"
#include "ScreenHandler.hpp"
#include "img_resources.hpp"
#include <option/has_toolchanger.h>
#include <config_store/store_instance.hpp>

static constexpr const char *const heating_str = N_("Heating");
static constexpr const char *const low_temp_str = N_("Low temp");

I_MI_AXIS::I_MI_AXIS(size_t index)
    : WiSpinInt(round(marlin_vars()->logical_curr_pos[index]),
        SpinCnf::axis_ranges[index], _(MenuVars::labels[index]), nullptr, is_enabled_t::yes, is_hidden_t::no)
    , axis_index(index)
    , last_queued_pos(GetVal()) {}

void I_MI_AXIS::Loop() {
    jog_axis(last_queued_pos, static_cast<float>(GetVal()), static_cast<AxisEnum>(axis_index));
}

bool I_MI_AXIS::is_move_finished() const {
    return last_queued_pos == GetVal();
}

void I_MI_AXIS::finish_move() {
    if (last_queued_pos == GetVal()) {
        return;
    }

    marlin_client::move_axis(GetVal(), MenuVars::GetManualFeedrate()[axis_index], axis_index);
}

void MI_AXIS_E::OnClick() {
    marlin_client::gcode("G90"); // Set to Absolute Positioning
    marlin_client::gcode("M82"); // Set extruder to absolute mode
    marlin_client::gcode("G92 E0"); // Reset position before change
    SetVal(0); // Reset spin before change
    last_queued_pos = 0; // zero it out so we wont go back when we exit the spinner
}

void DUMMY_AXIS_E::click([[maybe_unused]] IWindowMenu &window_menu) {
    marlin_client::gcode_printf("M1700 S E W2 B0"); // set filament, preheat to target, do not heat bed, return option
}

/**
 * @brief handle touch
 * it behaves the same as click, but only when extension was clicked
 */
void DUMMY_AXIS_E::touch(IWindowMenu &window_menu, point_ui16_t relative_touch_point) {
    Rect16::Width_t width = window_menu.GetRect().Width();
    if (width >= relative_touch_point.x && (width - extension_width) <= relative_touch_point.x) {
        click(window_menu);
    }
}

bool DUMMY_AXIS_E::IsTargetTempOk() {
    auto current_filament = config_store().get_filament_type(marlin_vars()->active_extruder);
    auto current_filament_nozzle_target = filament::get_description(current_filament).nozzle;
    return (current_filament != filament::Type::NONE) // filament is selected
        && (int(marlin_vars()->active_hotend().target_nozzle + 0.9F) >= current_filament_nozzle_target); // target temperature is high enough - +0.9 to avoid float round error
}

DUMMY_AXIS_E::DUMMY_AXIS_E()
    : WI_FORMATABLE_LABEL_t<int>(_(MenuVars::labels[MARLIN_VAR_INDEX_E]), nullptr, is_enabled_t::yes, is_hidden_t::no, 0,
        // this lambda is used during print, but does require item to be invalidated
        [&](char *buffer) {
            if (value) {
                _(heating_str).copyToRAM(buffer, GuiDefaults::infoDefaultLen);
            } else {
                _(low_temp_str).copyToRAM(buffer, GuiDefaults::infoDefaultLen);
            }
        }) {}

void DUMMY_AXIS_E::Update() {
    if (value != IsTargetTempOk()) {
        value = IsTargetTempOk();
        InValidateExtension();
    }
}

void MI_RETURN_ScreenMenuMove::click(IWindowMenu &window_menu) {
    assert(Screens::Access()->IsScreenOpened<ScreenMenuMove>());
    ScreenMenuMove *screen = static_cast<ScreenMenuMove *>(Screens::Access()->Get());

    const auto move_items = std::to_array<I_MI_AXIS *>({
        &screen->Item<MI_AXIS_X>(),
        &screen->Item<MI_AXIS_Y>(),
        &screen->Item<MI_AXIS_Z>(),
        &screen->Item<MI_AXIS_E>(),
    });

    const auto are_moves_finished = [&]() {
        return std::ranges::all_of(move_items.begin(), move_items.end(), [](auto *i) { return i->is_move_finished(); });
    };

    if (!are_moves_finished()) {
        Response msg_box_result;

        // Show question message box
        // In subblock to prevent screen repaint in dialog destructor
        {
            const auto txt = _("Target position not yet reached.\n\nCancel the movement immediately?");
            const PhaseResponses resp = { Response::Yes, Response::No, Response::Cancel };
            const PhaseTexts labels = { BtnResponse::GetText(resp[0]), BtnResponse::GetText(resp[1]), BtnResponse::GetText(resp[2]) };

            MsgBoxIconned msg_box(GuiDefaults::DialogFrameRect, resp, 0, &labels, txt, is_multiline::yes, &img::question_48x48);
            msg_box.MakeBlocking([&] {
                if (are_moves_finished()) {
                    Screens::Access()->Close();
                }
            });
            msg_box_result = msg_box.GetResult();
        }

        switch (msg_box_result) {

        case Response::No: // Finish moves
            for (auto i : move_items) {
                i->finish_move();
            }
            break;

        case Response::Yes: // Do not plan ending moves
        default: // Cancelled because moves finished
            break;

        case Response::Cancel: // Do not return to the previous menu
            return;
        }

        // We will be closing the screen immediately, so this is to prevent ugly screen redraw between dialog close and screen exit.
        screen->Validate();
    }

    MI_RETURN::click(window_menu);
}

void ScreenMenuMove::checkNozzleTemp() {
#if HAS_TOOLCHANGER()
    if (!prusa_toolchanger.is_toolchanger_enabled())
    // MI_COOLDOWN is always visible on multitool
#endif /*HAS_TOOLCHANGER()*/
    {
        DUMMY_AXIS_E::IsTargetTempOk() ? EnableItem<MI_COOLDOWN>() : DisableItem<MI_COOLDOWN>();
    }

    if (IsTempOk() == Item<MI_AXIS_E>().IsHidden()) {
        menu.SwapVisibility(Item<DUMMY_AXIS_E>(), Item<MI_AXIS_E>());
    }
}

// TODO make unit test
#if 0
    void checkNozzleTemp() {
        ClrMenuTimeoutClose();//dont timeout during test
        static int state  = 0;

        switch(state++) {
        case 0: // select axis e
            Hide<MI_AXIS_E>();
            Hide<MI_COOLDOWN>();
            menu.SetActiveItem(Item<DUMMY_AXIS_E>());
            break;
        case 1: // emulate click on e - show cooldown
            Show<MI_COOLDOWN>();
            break;
        case 2: // temp reached, enable working e, hide dummy part 0
            Show<MI_AXIS_E>();
            break;
        case 3: // temp reached, enable working e, hide dummy part 1
            if (menu.GetActiveItem() == &Item<DUMMY_AXIS_E>()) {
                menu.SetActiveItem(Item<MI_AXIS_E>());
            }
            break;
        case 4: // temp reached, enable working e, hide dummy part 2
            Hide<DUMMY_AXIS_E>();
            break;
        case 5: // cooldown click part 0
            Show<DUMMY_AXIS_E>();
            if (menu.GetActiveItem() == &Item<MI_AXIS_E>()) {
                menu.SetActiveItem(Item<DUMMY_AXIS_E>());
            }
             break;
        case 6: // cooldown click part 1
            Item<DUMMY_AXIS_E>().Update();
            break;

        case 7: // cooldown click part 2
            Hide<MI_AXIS_E>();
            break;
        case 8: // cooldown click part 3
            if (menu.GetActiveItem() == &Item<MI_COOLDOWN>()) {
                menu.Decrement(1);
            }
            Hide<MI_COOLDOWN>(); // now it is not focussed, so Hide() will succeed
            break;
        default:
            state = 0;
        }

        ClrMenuTimeoutClose();
    }
#endif // 0 .. make unit test

bool ScreenMenuMove::IsTempOk() {
    return DUMMY_AXIS_E::IsTargetTempOk() // target correctly set
        && (marlin_vars()->active_hotend().temp_nozzle >= temp_ok); // Temperature is above coldextrusion
}

ScreenMenuMove::ScreenMenuMove()
    : ScreenMenuMove__(_(label)) {
#if !PRINTER_IS_PRUSA_MINI
    header.SetIcon(&img::move_16x16);
#endif
    prev_accel = marlin_vars()->travel_acceleration;
    marlin_client::gcode("M204 T200");
    Hide<MI_AXIS_E>(); // one of pair MI_AXIS_E DUMMY_AXIS_E must be hidden for swap to work
    checkNozzleTemp();
    ClrMenuTimeoutClose(); // No timeout for move screen
}

ScreenMenuMove::~ScreenMenuMove() {
    char msg[20];
    snprintf(msg, sizeof(msg), "M204 T%f", (double)prev_accel);
    marlin_client::gcode(msg);
}

void ScreenMenuMove::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::CHILD_CLICK) {
        HOTEND_LOOP() {
            marlin_client::set_target_nozzle(0, e);
        }
        marlin_client::set_display_nozzle(0);
        marlin_client::set_target_bed(0);
    }

    if (event == GUI_event_t::LOOP) {
        checkNozzleTemp();
        Item<DUMMY_AXIS_E>().Update();
    }

    SuperWindowEvent(sender, event, param);
}
