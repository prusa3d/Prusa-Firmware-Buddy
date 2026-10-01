#pragma once

namespace hal {

void init();

[[noreturn]] void panic();
[[noreturn]] void reset();

} // namespace hal
