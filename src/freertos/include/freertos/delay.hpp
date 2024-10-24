#pragma once

#include <cstddef>

namespace freertos {

/** Suspend execution of current task for given number of milliseconds. */
void delay(size_t milliseconds);

/** Return milliseconds since some fixed arbitrary point in time. */
size_t millis();

} // namespace freertos
