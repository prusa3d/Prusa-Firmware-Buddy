/// @file
#include "screen_menu_e2ee.hpp"

#include "ScreenHandler.hpp"
#include <common/e2ee/e2ee.hpp>
#include <common/e2ee/identity_check_levels.hpp>
#include <common/e2ee/key.hpp>
#include <img_resources.hpp>

MI_KEY::MI_KEY()
    : WI_INFO_t {
        _("Key Status"),
    } {
    Loop();
}

void MI_KEY::Loop() {
    ChangeInformation(e2ee::is_private_key_present() ? _("Initialized") : _("Uninitialized"));
}

MI_KEYGEN::MI_KEYGEN()
    : IWindowMenuItem {
        _("Generate Private Key"),
    } {}

void MI_KEYGEN::click(IWindowMenu &) {
    const auto closing_callback = [this] {
        if (!key_generation.is_active()) {
            Screens::Access()->Close();
        }
        osDelay(1);
    };

    if (e2ee::is_private_key_present() && MsgBoxWarning(_("Are you sure you want to overwrite the encryption key? Previously encrypted G-Codes for this printer won't work."), Responses_YesNo) == Response::No) {
        return;
    }

    key_generation.issue(&e2ee::generate_key);

    const auto msgbox_builder = MsgBoxBuilder { .text = _("Generating encryption key..."), .responses = { Response::Abort }, .loop_callback = closing_callback };
    if (msgbox_builder.exec() == Response::Abort) {
        key_generation.discard();
        return;
    }

    if (!key_generation.result()) {
        MsgBoxWarning(_("Failed to generate the encryption key."), Responses_Ok);
    }
}

MI_EXPORT::MI_EXPORT()
    : IWindowMenuItem {
        _("Export Public Key"),
    } {}

void MI_EXPORT::click(IWindowMenu &) {
    if (e2ee::export_key()) {
        MsgBox(_("The public key (pubkey.der) was exported to the USB Flash disk."), Responses_Ok);
    } else {
        MsgBoxWarning(_("Failed to export the public key. Make sure USB Flash disk is inserted."), Responses_Ok);
    }
}

static constexpr const char *identity_check_items[] = {
    N_("Known only"),
    N_("Ask"),
    N_("Accept all"),
};

MI_IDENTITY_CHECKING::MI_IDENTITY_CHECKING()
    : MenuItemSwitch {
        _("Identity Checking"),
        identity_check_items,
        static_cast<size_t>(config_store().identity_check.get())
    } {
    // The identity checking is not complete yet, so hide it from users for now.
    showDevOnly();
}

bool MI_IDENTITY_CHECKING::on_item_selected(const OnItemSelectedArgs &args) {
    config_store().identity_check.set(static_cast<e2ee::IdentityCheckLevel>(args.new_index));
    return true;
}

ScreenMenuE2ee::ScreenMenuE2ee()
    : ScreenMenuE2eeBase {
        _("ENCRYPTION"),
        &img::padlock_16x16,
    } {}
