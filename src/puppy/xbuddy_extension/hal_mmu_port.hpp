/// @file
#pragma once

#include "option/extension_variant.h"

#define HAS_MMU_POWER_PIN() HAS_GPIO_EXPANDER()

// Separate from hal_mmu - the port is used for other devices than just MMU (XLCAN -> ModularBed, INDX -> INDX_HEAD)
namespace hal::mmu_port {

void init();

#if HAS_MMU_POWER_PIN()
/// Control the power pin of the MMU.
void power_pin_set(bool);

/// Read the power pin of the MMU.
bool power_pin_get();
#endif

/// Control the nreset pin of the MMU.
void nreset_pin_set(bool);

/// Read the nreset pin of the MMU.
bool nreset_pin_get();

} // namespace hal::mmu_port
