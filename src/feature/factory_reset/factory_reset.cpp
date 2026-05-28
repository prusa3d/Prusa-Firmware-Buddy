#include "factory_reset.hpp"

#include <common/w25x.hpp>
#include <option/has_internal_storage_flash.h>
#if HAS_INTERNAL_STORAGE_FLASH()
    #include <common/mt29f_flash.hpp>
#endif
#include <common/st25dv64k.h>
#include <buddy/filesystem_littlefs.hpp>
#include <common/sys.hpp>
#include <crash_dump/dump.hpp>
#include <bootloader/bootloader.hpp>
#include <common/visit_all_struct_fields.hpp>
#include <config_store/store_definition.hpp>
#include <config_store/store_journal.hpp>
#include <option/bootloader.h>
#include <gui.hpp>
#include <display_helper.h>
#include <utils/progress_mapper.hpp>

#include <option/has_phase_stepping.h>
#if HAS_PHASE_STEPPING()
    #include <feature/phase_stepping/phase_stepping.hpp>
#endif
#include <option/has_e2ee_support.h>
#include <bsod/bsod.h>
#if HAS_E2EE_SUPPORT()
    #include <e2ee/key.hpp>
#endif

// Check that our presets cover all the item flags
static constexpr auto store_flags = [] {
    journal::ItemFlags store_flags = 0;
    config_store_ns::CurrentStore s {};
    visit_all_struct_fields(s, [&](auto &item) {
        store_flags |= item.flags;
    });

    // Exclude special items, they are special
    store_flags &= ~config_store_ns::ItemFlag::special;

    return store_flags;
}();
static constexpr auto preset_flags = [] {
    journal::ItemFlags preset_flags = 0;
    for (const auto &item : FactoryReset::items_config) {
        preset_flags |= item.item_flags;
    }

    return preset_flags;
}();
static_assert(store_flags == preset_flags);

static uint16_t encode_params(bool hard_reset, FactoryReset::ItemBitset items_to_keep) {
    static_assert(std::to_underlying(FactoryReset::Item::_cnt) < 16, "ItemBitset must fit in 15 bits");
    return static_cast<uint16_t>(items_to_keep.to_ulong()) | (hard_reset ? (1 << 15) : 0);
}

static bool decode_hard_reset(uint16_t encoded_params) {
    return encoded_params & (1 << 15);
}

static FactoryReset::ItemBitset decode_items_to_keep(uint16_t encoded_params) {
    return FactoryReset::ItemBitset(encoded_params & ~(1 << 15));
}

[[noreturn]] void FactoryReset::perform(bool hard_reset, ItemBitset items_to_keep) {
    crash_dump::save_message(crash_dump::MsgType::FACTORY_RESET, encode_params(hard_reset, items_to_keep), nullptr, nullptr);
    sys_reset();
}

[[noreturn]] void FactoryReset::perform_internal() {
    const uint16_t encoded_params = crash_dump::load_message_error_code();
    crash_dump::message_set_displayed();

    const bool hard_reset = decode_hard_reset(encoded_params);
    const ItemBitset items_to_keep = decode_items_to_keep(encoded_params);

    // Render the screen
    Rect16 progress_rect;
    {
        // Fill background
        render_rect(GuiDefaults::RectScreen, COLOR_BLACK);

        constexpr int l = 16;
        constexpr int r = GuiDefaults::ScreenWidth - 16;

        int y = 16;
        int b = y;

        // Draw the message
        {

            constexpr Font font = HAS_MINI_DISPLAY() ? Font::small : Font::normal;

            auto rect = Rect16::fromLTRB(l, y, r, y);
            const auto text = _("Performing factory reset.\nDo not turn off the printer.");

            rect = Rect16::Height_t(RectTextLayout::compute_line_count(text, rect.Width() / resource_font_size(font).w) * resource_font_size(font).h);
            b += rect.Height();

            render_text_align(rect, text, font, COLOR_BLACK, COLOR_WHITE, {}, text_flags { Align_t::Center(), is_multiline::yes });

            b += 4; // Spacing between the title and the progress bar
            y = b;
        }

        {
            b += 12; // Height of the progress bar

            progress_rect = Rect16::fromLTRB(l, y, r, b);
            render_rect(progress_rect, COLOR_GRAY);

            b += 12; // Spacing below the progress bar
            y = b;
        }

        // Show what items we're keeping and deleting
        if (hard_reset) {
            constexpr Font font = Font::normal;
            constexpr int line_h = resource_font_size(font).h;

            b += line_h;
            render_text_align(Rect16::fromLTRB(l, y, r, b), _("HARD RESET"), font, COLOR_BLACK, COLOR_RED);
            y = b;

        } else {
            constexpr Font font = Font::small;
            constexpr int line_h = resource_font_size(font).h;
            constexpr int char_w = resource_font_size(font).w;

            const auto keep_text = _("Keep");
            const auto reset_text = _("Reset");

            const auto state_column_width = std::max(keep_text.computeNumUtf8Chars(), reset_text.computeNumUtf8Chars()) * char_w;

            for (int i = 0; i < std::to_underlying(Item::_cnt); i++) {
                const auto title = _(items_config[i].title);
                auto title_rect = Rect16::fromLTRB(l + state_column_width + 8, y, r, y);
                title_rect = Rect16::Height_t(RectTextLayout::compute_line_count(title, title_rect.Width() / char_w) * line_h);

                b += title_rect.Height();
                const bool keep = items_to_keep.test(i);
                render_text_align(Rect16::fromLTRB(l, y, l + state_column_width, b), keep ? keep_text : reset_text, font, COLOR_BLACK, keep ? COLOR_GREEN : COLOR_RED);
                render_text_align(title_rect, title, font, COLOR_BLACK, COLOR_WHITE, {}, text_flags { Align_t::LeftTop(), is_multiline::yes });

                b += 4; // A bit of extra spacing betwen items
                y = b;
            }
        }
    }

    int last_progress_width = 0;
    const auto render_progress = [&](ProgressPercent progress_pct) {
        const int width = progress_rect.Width() * progress_pct / 100;
        if (width == last_progress_width) {
            return;
        }
        last_progress_width = width;
        render_rect(Rect16::fromLTWH(progress_rect.Left(), progress_rect.Top(), width, progress_rect.Height()), COLOR_BRAND);
    };

    enum class FactoryResetStep : uint8_t {
        wipe_specific_xflash_files,
        wipe_eeprom,
        wipe_xflash,
#if HAS_INTERNAL_STORAGE_FLASH()
        wipe_internal_storage,
#endif
        finalize,
    };

    constexpr bool wipe_eeprom = true;
    const bool wipe_xflash = hard_reset;
    const bool wipe_specific_xflash_files = !hard_reset;
#if HAS_INTERNAL_STORAGE_FLASH()
    // Never erase an unidentified chip
    const bool wipe_internal_storage = (hard_reset || !items_to_keep.test(std::to_underlying(Item::internal_storage)))
        && Mt29fFlash::instance().is_initialized();
#endif

    // Scales are relative phase durations (in seconds) measured on HW.
    using Scale = ProgressMapperWorkflowStep<FactoryResetStep>::Scale;
    const ProgressMapperWorkflowArray factory_reset_workflow {
        ProgressMapperWorkflow<FactoryResetStep>::Runtime {},
        std::to_array<ProgressMapperWorkflowStep<FactoryResetStep>>({
            { FactoryResetStep::wipe_specific_xflash_files, 1 },
                { FactoryResetStep::wipe_eeprom, 25 },
                { FactoryResetStep::wipe_xflash, wipe_xflash ? Scale(30) : Scale(0) },
#if HAS_INTERNAL_STORAGE_FLASH()
                { FactoryResetStep::wipe_internal_storage, wipe_internal_storage ? Scale(300) : Scale(0) },
#endif
                { FactoryResetStep::finalize, 1 },
        }),
    };
    ProgressMapper<FactoryResetStep> progress_mapper(factory_reset_workflow);

    const auto indicate_progress = [&](FactoryResetStep step, float normalized_progress) {
        render_progress(progress_mapper.update_progress(step, normalized_progress));
    };

    if (wipe_specific_xflash_files) {
#if HAS_PHASE_STEPPING()
        // Phase stepping is a calibration that is stored on xFlash, not in the config store -> it needs special handling
        // On hard reset, we're clearing the whole xFlash anyway, no point in doing this separately
        if (!items_to_keep.test(std::to_underlying(Item::calibrations))) {

            // Formally speaking, we should access phase_stepping only from the marlin thread.
            // But all this function does is unlink files from the xFlash.
            // These files are only accessed in specific phstep gcodes
            // and it's not worth ensuring that they would happen to overwrite before we enter the final factory reset critical section a few lines below
            phase_stepping::remove_from_persistent_storage();
        }
#endif

#if HAS_E2EE_SUPPORT()
        if (!items_to_keep.test(std::to_underlying(Item::security))) {
            e2ee::remove_all_identities();
            e2ee::remove_key();
        }
#endif

        // Configurations that are out of config store should be wiped here
    }
    indicate_progress(FactoryResetStep::wipe_specific_xflash_files, 1.f);

    if (wipe_eeprom) {
        constexpr uint16_t eeprom_size = 8192;
        for (uint16_t address = 0; address < eeprom_size; address += 4) {
            static constexpr uint32_t empty = ~0;
            st25dv64k_user_write_bytes(address, &empty, 4);
            indicate_progress(FactoryResetStep::wipe_eeprom, float(address) / eeprom_size);
        }
    }
    indicate_progress(FactoryResetStep::wipe_eeprom, 1.f);

    if (wipe_xflash) {
        auto &w25x_flash = W25xFlash::instance();
        const uint32_t xflash_size = w25x_flash.block_count() * W25xFlash::block_size;
        for (uint32_t address = 0; address < xflash_size; address += W25xFlash::block64_size) {
            w25x_flash.block64_erase(address);
            indicate_progress(FactoryResetStep::wipe_xflash, float(address) / xflash_size);
        }
    }
    indicate_progress(FactoryResetStep::wipe_xflash, 1.f);

#if HAS_INTERNAL_STORAGE_FLASH()
    if (wipe_internal_storage) {
        internal_storage_unmount();
        // Failed mount reformats the erased chip on next boot
        auto &nand = Mt29fFlash::instance();
        const uint32_t nand_blocks = nand.block_count();
        for (uint32_t block = 0; block < nand_blocks; block++) {
            nand.erase_block(block * Mt29fFlash::block_size);
            indicate_progress(FactoryResetStep::wipe_internal_storage, float(block) / nand_blocks);
        }
    }
    indicate_progress(FactoryResetStep::wipe_internal_storage, 1.f);
#endif

    if (hard_reset) {
#if BOOTLOADER()
        // Invalidate firmware by erasing part of it
        if (!buddy::bootloader::fw_invalidate()) {
            bsod("Error invalidating firmware");
        }
#endif

    } else if (items_to_keep.any()) {
        // Initialize a blank config store and save there our selection of items.
        // Values of these items are being kept in the RAM.
        config_store_journal().init();

        // Do not do default config_sotre().load_all(), it would overwrite our items in the RAM.
        // Instead we provide a stub loader and migrators, so we only end up initializing the EEPROM structure properly.
        config_store().get_backend().load_all([](const auto &, const auto &) {}, {});

        debug_assert(config_store().get_backend().get_journal_state() == journal::Backend::JournalState::ColdStart);

        journal::ItemFlags exclude_flags = 0;
        for (size_t i = 0; i < std::to_underlying(Item::_cnt); i++) {
            if (!items_to_keep.test(i)) {
                exclude_flags |= items_config[i].item_flags;
            }
        }
        config_store_journal().dump_items(exclude_flags);

        // If we're resetting hw config, make sure that we perform first run config setup
        if (exclude_flags & config_store_ns::ItemFlag::hw_config) {
            config_store().force_default_hw_config.set(true);
        }

        config_store().config_version.set(config_store_ns::CurrentStore::newest_config_version);
    }

    indicate_progress(FactoryResetStep::finalize, 1.f);

    // Wait a few seconds so that the user can visually verify that the factory reset is complete
    HAL_Delay(3000);

    // Just restart the system
    sys_reset();
}
