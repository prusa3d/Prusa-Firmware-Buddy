/**
 * @file screen_menu_cancel_object.cpp
 */

#include "screen_menu_cancel_object.hpp"
#include "ScreenHandler.hpp"
#include <img_resources.hpp>
#include <marlin_vars.hpp>
#include <marlin_client.hpp>

#if ENABLED(CANCEL_OBJECTS)

ScreenMenuCancelObject::ScreenMenuCancelObject()
    : detail::ScreenMenuCancelObject(_(label)) {}

MI_CO_CANCEL_OBJECT::MI_CO_CANCEL_OBJECT()
    : IWindowMenuItem(
        _(label), nullptr, marlin_vars().cancel_object_count > 0 ? is_enabled_t::yes : is_enabled_t::no,
        is_hidden_t::no, expands_t::yes) {
}

void MI_CO_CANCEL_OBJECT::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuCancelObject>);
}

static constexpr const char *cancel_object_n_values[] = {
    N_("Printing"),
    N_("Canceled"),
};

MI_CO_OBJECT_N::MI_CO_OBJECT_N(int ObjectId_)
    : MenuItemSwitch(string_view_utf8::MakeRAM(label_buffer), cancel_object_n_values)
    , ObjectId(ObjectId_) {

    UpdateName();
    UpdateState();
}

void MI_CO_OBJECT_N::UpdateState() {
    size_t new_index = (marlin_vars().get_cancel_object_mask() & (static_cast<uint64_t>(1) << ObjectId)) ? 1 : 0;
    if (GetIndex() != new_index) {
        SetIndex(new_index);
    }
}

void MI_CO_OBJECT_N::UpdateName() {
    if (marlin_vars().cancel_object_count > ObjectId) {
        bool empty; ///< True if object name from G-code is empty

        { // Do all things in one lock
            MarlinVarsLockGuard lock;

            // Check if object name is empty
            char first_char[2]; // Needs to be 2 to get 1 character and a valid null-terminator
            marlin_vars().cancel_object_names[ObjectId].copy_to(first_char, 2, lock);
            empty = (first_char[0] == '\0');

            // Check if object name is known and (either was unknown before or changed)
            if (!empty && (backup_label_used || (!marlin_vars().cancel_object_names[ObjectId].equals(label_buffer, lock)))) {
                marlin_vars().cancel_object_names[ObjectId].copy_to(label_buffer, sizeof(label_buffer), lock);
                backup_label_used = false;
                Invalidate(); // The string memory is the same, so it needs to be invalidated manually
            }
        } // Unlock marlin_vars

        // Switch from object name to backup_label
        if (empty && !backup_label_used) {
            char temporary_buffer[sizeof(label_buffer) / sizeof(label_buffer[0])];
            _("Object %i").copyToRAM(temporary_buffer, std::size(temporary_buffer));
            snprintf(label_buffer, std::size(label_buffer), temporary_buffer, ObjectId); // Source string backup_label needs to have exactly one %i
            backup_label_used = true;
            Invalidate(); // The string memory is the same, so it needs to be invalidated manually
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
    if (old_index == 0) { // is printing
        marlin_client::cancel_object(ObjectId);
    } else {
        marlin_client::uncancel_object(ObjectId);
    }

    SetIndex(old_index); // Keep previous index and wait for UpdateState()
}

MI_CO_CANCEL_CURRENT::MI_CO_CANCEL_CURRENT()
    : IWindowMenuItem(_(label), &img::arrow_right_10x16, is_enabled_t::yes, marlin_vars().cancel_object_count > 0 ? is_hidden_t::no : is_hidden_t::yes) {}

void MI_CO_CANCEL_CURRENT::click(IWindowMenu & /*window_menu*/) {
    marlin_client::cancel_current_object();
}

void ScreenMenuCancelObject::windowEvent(window_t *, GUI_event_t event, void *) {
    if (event == GUI_event_t::LOOP) {
        if (loop_index++ > 20) { // Approx once a second
            loop_index = 0;
            // Update all object names and visibility
            Item<MI_CO_OBJECT<0>>().UpdateName();
            Item<MI_CO_OBJECT<1>>().UpdateName();
            Item<MI_CO_OBJECT<2>>().UpdateName();
            Item<MI_CO_OBJECT<3>>().UpdateName();
            Item<MI_CO_OBJECT<4>>().UpdateName();
            Item<MI_CO_OBJECT<5>>().UpdateName();
            Item<MI_CO_OBJECT<6>>().UpdateName();
            Item<MI_CO_OBJECT<7>>().UpdateName();
            Item<MI_CO_OBJECT<8>>().UpdateName();
            Item<MI_CO_OBJECT<9>>().UpdateName();
            Item<MI_CO_OBJECT<10>>().UpdateName();
            Item<MI_CO_OBJECT<11>>().UpdateName();
            Item<MI_CO_OBJECT<12>>().UpdateName();
            Item<MI_CO_OBJECT<13>>().UpdateName();
            Item<MI_CO_OBJECT<14>>().UpdateName();
            Item<MI_CO_OBJECT<15>>().UpdateName();
        }
        // Update state of all items
        Item<MI_CO_OBJECT<0>>().UpdateState();
        Item<MI_CO_OBJECT<1>>().UpdateState();
        Item<MI_CO_OBJECT<2>>().UpdateState();
        Item<MI_CO_OBJECT<3>>().UpdateState();
        Item<MI_CO_OBJECT<4>>().UpdateState();
        Item<MI_CO_OBJECT<5>>().UpdateState();
        Item<MI_CO_OBJECT<6>>().UpdateState();
        Item<MI_CO_OBJECT<7>>().UpdateState();
        Item<MI_CO_OBJECT<8>>().UpdateState();
        Item<MI_CO_OBJECT<9>>().UpdateState();
        Item<MI_CO_OBJECT<10>>().UpdateState();
        Item<MI_CO_OBJECT<11>>().UpdateState();
        Item<MI_CO_OBJECT<12>>().UpdateState();
        Item<MI_CO_OBJECT<13>>().UpdateState();
        Item<MI_CO_OBJECT<14>>().UpdateState();
        Item<MI_CO_OBJECT<15>>().UpdateState();
    }
}
#endif /* ENABLED(CANCEL_OBJECTS) */
