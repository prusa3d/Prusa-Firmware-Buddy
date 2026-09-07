#pragma once

#include <puppies/BootloaderProtocol.hpp>

namespace buddy::puppies {

/// Bounded probe for an XL-CAN bridge: reports whether one is fitted, which is
/// what tells an XLS from a plain XL.
[[nodiscard]] bool xl_can_probe(BootloaderProtocol &);

/// Brings the bridge from its bootloader into its firmware. Fatal if the
/// bridge does not answer, so call it only once xl_can_probe() has confirmed
/// one is fitted.
void xl_can_bootstrap(BootloaderProtocol &);

} // namespace buddy::puppies
