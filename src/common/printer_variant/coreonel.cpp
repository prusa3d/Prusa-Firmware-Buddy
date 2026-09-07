#include <common/printer_variant/printer_variant.hpp>

#include <common/aggregate_arity.hpp>
#include <config_store/store_instance.hpp>
#include <option/has_nozzle_cleaner_lite.h>
#include <utils/enum_array.hpp>

#include <optional>
#include <utility>

namespace {

/// Feature flags an edition prepicks. Unnamed fields in the designated initializers stay false.
struct Features {
    bool belts_15gt : 1 = false;
    bool auto_vent : 1 = false;
#if HAS_NOZZLE_CLEANER_LITE() // absent on the INDX variant
    bool nozzle_cleaner_lite : 1 = false;
#endif

    static Features from_config_store();

    /// \returns true if a restart is required for the change to take effect
    [[nodiscard]] bool apply_to_config_store() const;

    constexpr bool operator==(const Features &) const = default;
};

static_assert(aggregate_arity<Features>() == 2 + HAS_NOZZLE_CLEANER_LITE(), "Revise from_config_store/apply_to_config_store and variant_features");

Features Features::from_config_store() {
    auto &store = config_store();
    return {
        .belts_15gt = store.belts_15gt_installed.get(),
        .auto_vent = store.get_vent_control() == VentControl::automatic,
#if HAS_NOZZLE_CLEANER_LITE()
        .nozzle_cleaner_lite = store.nozzle_cleaner_lite_installed.get(),
#endif
    };
}

bool Features::apply_to_config_store() const {
    auto &store = config_store();
    // The belt type changes X/Y steps/mm; the planner reads those only at init, so a live change needs a restart to take effect (a boot-time apply does not).
    const bool restart_required = store.set_belts_15gt(belts_15gt);
    store.set_vent_control(auto_vent ? VentControl::automatic : VentControl::manual);
#if HAS_NOZZLE_CLEANER_LITE()
    store.nozzle_cleaner_lite_installed.set(nozzle_cleaner_lite);
#endif
    return restart_required;
}

constexpr EnumArray<PrinterVariant, Features, PrinterVariant::_cnt> variant_features {
    { PrinterVariant::base, { .auto_vent = true } },
    { PrinterVariant::plus, {
                                .belts_15gt = true,
                                .auto_vent = true,
#if HAS_NOZZLE_CLEANER_LITE()
                                .nozzle_cleaner_lite = true,
#endif
                            } },
};

} // namespace

bool apply_printer_variant_defaults(PrinterVariant variant) {
    auto transaction = config_store().get_backend().transaction_guard();
    return variant_features[variant].apply_to_config_store();
}

std::optional<PrinterVariant> printer_variant_from_config() {
    const Features current = Features::from_config_store();
    for (size_t i = 0; i < std::to_underlying(PrinterVariant::_cnt); i++) {
        const auto variant = static_cast<PrinterVariant>(i);
        if (variant_features[variant] == current) {
            return variant;
        }
    }
    return std::nullopt; // Custom - matches no edition
}
