#pragma once

#include <printers.h>

// Some printers ship in feature "editions" that share one firmware and one PrinterModel identity.
// PrinterVariant selects which edition's default features are enabled.
//
// Choosing a mechanism when a printer comes in multiple hardware flavors:
// - Must the difference be visible outside the printer (Connect printer type, M862 answer, USB PID,
//   error-code prefix)? Then it is a separate PrinterModel selected via ExtendedPrinterType, with
//   the ecosystem coordination cost that entails.
// - Is it bolt-on, field-upgradable equipment that users may mix freely? Then each piece is a
//   config-store feature flag and the edition is only a non-persisted preset over those flags;
//   it never reaches Connect / g-code / USB.
// A printer gets at most one of the two mechanisms (enforced in printer_model.cpp). Do not add
// a third one.

#if PRINTER_IS_PRUSA_COREONE()
    #define HAS_PRINTER_VARIANT() 1
    #include <common/printer_variant/coreone.hpp>
#elif PRINTER_IS_PRUSA_COREONEL()
    #define HAS_PRINTER_VARIANT() 1
    #include <common/printer_variant/coreonel.hpp>
#else
    #define HAS_PRINTER_VARIANT() 0
#endif

#if HAS_PRINTER_VARIANT()
    #include <optional>

/// Applies the given edition's default feature flags to the config store (in one transaction).
/// \returns true if a restart is required for the change to take effect
/// - the runtime caller should restart, the boot path can ignore it.
[[nodiscard]] bool apply_printer_variant_defaults(PrinterVariant variant);

/// \returns the edition whose defaults match the current config flags, or nullopt for a "Custom" config.
std::optional<PrinterVariant> printer_variant_from_config();
#endif
