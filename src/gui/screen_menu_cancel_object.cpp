/**
 * @file screen_menu_cancel_object.cpp
 */

#include "screen_menu_cancel_object.hpp"
#include "ScreenHandler.hpp"
#include <marlin_vars.hpp>
#include <marlin_client.hpp>

#if ENABLED(CANCEL_OBJECTS)

ScreenMenuCancelObject::ScreenMenuCancelObject()
    : ScreenMenuCancelObject__(_(label)) {}

MI_CO_CANCEL_OBJECT::MI_CO_CANCEL_OBJECT()
    : WI_LABEL_t(
        _(label), nullptr, marlin_vars()->cancel_object_count > 0 ? is_enabled_t::yes : is_enabled_t::no,
        is_hidden_t::no, expands_t::yes) {
}

void MI_CO_CANCEL_OBJECT::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuCancelObject>);
}

MI_CO_OBJECT_N::MI_CO_OBJECT_N(int ObjectId_)
    : WI_SWITCH_t(
        0, string_view_utf8::MakeRAM((uint8_t *)label_buffer), nullptr,
        is_enabled_t::yes, is_hidden_t::yes, _(str_printing), _(str_canceled))
    , ObjectId(ObjectId_) {

    UpdateName();
    UpdateState();
}

void MI_CO_OBJECT_N::UpdateState() {
    SetIndex((marlin_vars()->cancel_object_mask & (1 << ObjectId)) ? 1 : 0);
}

void MI_CO_OBJECT_N::UpdateName() {
    if (marlin_vars()->cancel_object_count > ObjectId) {
        bool empty; ///< True if object name from G-code is empty

        {           // Do all things in one lock
            MarlinVarsLockGuard lock;

            // Check if object name is empty
            char first_char[2]; // Needs to be 2 to get 1 character and a valid null-terminator
            marlin_vars()->cancel_object_names[ObjectId].copy_to(first_char, 2, lock);
            empty = (first_char[0] == '\0');

            // Check if object name is known and (either was unknown before or changed)
            if (!empty && (backup_label_used || (!marlin_vars()->cancel_object_names[ObjectId].equals(label_buffer, lock)))) {
                marlin_vars()->cancel_object_names[ObjectId].copy_to(label_buffer, sizeof(label_buffer), lock);
                backup_label_used = false;
                Invalidate(); // The string memory is the same, so it needs to be invalidated manually
            }
        }                     // Unlock marlin_vars

        // Switch from object name to backup_label
        if (empty && !backup_label_used) {
            char temporary_buffer[sizeof(label_buffer) / sizeof(label_buffer[0])];
            _(backup_label).copyToRAM(temporary_buffer, std::size(temporary_buffer));
            snprintf(label_buffer, std::size(label_buffer), temporary_buffer, ObjectId); // Source string backup_label needs to have exactly one %i
            backup_label_used = true;
            Invalidate();                                                                // The string memory is the same, so it needs to be invalidated manually
        }

        show();
    } else {
        if (!IsHidden()) {
            hide();
            Invalidate();
        }
    }
}

void MI_CO_OBJECT_N::OnChange(size_t old_index) {
    if (old_index == 0) {                                  // is printing
        marlin_client::gcode_printf("M486 P%i", ObjectId); // Cancel object N
    } else {
        marlin_client::gcode_printf("M486 U%i", ObjectId); // Uncancel object N
    }

    SetIndex(old_index); // Keep previous index and wait for UpdateState()
}

MI_CO_CANCEL_CURRENT::MI_CO_CANCEL_CURRENT()
    : WI_LABEL_t(_(label), &img::arrow_right_10x16, is_enabled_t::yes, marlin_vars()->cancel_object_count > 0 ? is_hidden_t::no : is_hidden_t::yes) {}

void MI_CO_CANCEL_CURRENT::click(IWindowMenu & /*window_menu*/) {
    marlin_client::gcode("M486 C"); // Cancel current object
}

#endif /* ENABLED(CANCEL_OBJECTS) */
