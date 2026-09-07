#pragma once

#include <bitset>

#include <utils/enum_array.hpp>
#include <option/has_e2ee_support.h>

class FactoryReset {

public:
    using ItemFlag = config_store_ns::ItemFlag;

    /// Specific items that can be kept or reset
    enum class Item : uint8_t {
        calibrations,
        common_misconfigurations,
        network,
        user_interface,
        stats,
        hw_config,
        features,
        printer_state,
        user_profiles,
#if HAS_E2EE_SUPPORT()
        security,
#endif

        _cnt
    };

    using ItemBitset = std::bitset<std::to_underlying(Item::_cnt)>;

    struct ItemConfig {
        const char *title;

        /// What config store items are related to this item
        journal::ItemFlags item_flags;
    };

    static constexpr EnumArray<Item, FactoryReset::ItemConfig, FactoryReset::Item::_cnt> items_config {
        { Item::calibrations, { N_("Calibrations"), ItemFlag::calibrations } },
            { Item::common_misconfigurations, { N_("Common Misconfigurations"), ItemFlag::common_misconfigurations | ItemFlag::dev_items } },
            { Item::network, { N_("Network Settings"), ItemFlag::network } },
            { Item::user_interface, { N_("UI Settings"), ItemFlag::user_interface } },
            { Item::stats, { N_("Statistics"), ItemFlag::stats } },

            // The calibrations are done for some hw_config (for example belts -> XY test/calibrations)
            // Therefore when resetting HW configuration, we need to reset the calibrations as well
            { Item::hw_config, { N_("HW Configuration"), ItemFlag::hw_config | ItemFlag::calibrations } },

            { Item::features, { N_("Printer Functions"), ItemFlag::features } },
            { Item::printer_state, { N_("Printer State"), ItemFlag::printer_state } },
            { Item::user_profiles, { N_("User Presets"), ItemFlag::user_presets } },
#if HAS_E2EE_SUPPORT()
            { Item::security, { N_("Security"), ItemFlag::security } },
#endif
    };

public:
    /// Executes a previously requested factory reset after reboot.
    /// Reads parameters from the crash dump message.
    [[noreturn]] static void perform_internal();

    /// Performs a factory reset with the specified parameters.
    /// !!! Has to be executed on the GUI thread
    [[noreturn]] static void perform(bool hard_reset, ItemBitset items_to_keep);

public:
    /// Constructs ItemBitset with the specified items set
    static constexpr ItemBitset item_bitset(const std::initializer_list<Item> &lst) {
        ItemBitset result;
        for (auto i : lst) {
            result.set(std::to_underlying(i));
        }
        return result;
    }

    /// Constructs ItemBitset with all EXCEPT the specified items set
    static constexpr ItemBitset item_bitset_exclude(const std::initializer_list<Item> &lst) {
        ItemBitset result(static_cast<uint32_t>(-1));
        static_assert(std::to_underlying(Item::_cnt) <= 32);
        for (auto i : lst) {
            result.reset(std::to_underlying(i));
        }
        return result;
    }

public:
    /// Remove all personal configuration, keep HW config, stats, calibrations
    static const ItemBitset ownership_transfer;

    /// Keep most things, reset only possible trouble makers (experimental settings, tweaks, ...)
    static const ItemBitset fix_common_misconfigurations;

    /// Reset everything, keep just HW configuration
    static const ItemBitset keep_hw_config;

    /// For the factory guys to run after the printer is assembled and tested
    static const ItemBitset shipping_reset;

    /// Reset everything
    static const ItemBitset full_reset;

    /// Printer type changed - keep only what is not tied to the printer type
    static const ItemBitset printer_type_change;
};

inline constexpr FactoryReset::ItemBitset FactoryReset::ownership_transfer = item_bitset({ Item::hw_config, Item::printer_state, Item::stats, Item::calibrations, Item::common_misconfigurations });
inline constexpr FactoryReset::ItemBitset FactoryReset::fix_common_misconfigurations = item_bitset_exclude({ Item::common_misconfigurations });
inline constexpr FactoryReset::ItemBitset FactoryReset::keep_hw_config = item_bitset({ Item::hw_config });
inline constexpr FactoryReset::ItemBitset FactoryReset::shipping_reset = item_bitset({ Item::hw_config, Item::calibrations });
inline constexpr FactoryReset::ItemBitset FactoryReset::full_reset = {};
inline constexpr FactoryReset::ItemBitset FactoryReset::printer_type_change = item_bitset({
    Item::network,
        Item::stats,
        Item::user_interface,
        Item::user_profiles,
#if HAS_E2EE_SUPPORT()
        Item::security,
#endif
});
