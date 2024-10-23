///@file

namespace hal {

/**
 * Initialize hardware abstraction layer module.
 */
void init();

/**
 * Enter infinite loop.
 */
[[noreturn]] void panic();

} // namespace hal
