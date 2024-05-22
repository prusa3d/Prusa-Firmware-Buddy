/**
 * Loading screen for bootstraping firmware.
 */
#pragma once

/**
 * @brief Show the screen, redraw when necessary and wait untill all bootstraps are done
 */
void gui_bootstrap_screen_run();

/**
 * @brief Update bootstrap screen state
 * @note This is expected to be called for different threads.
 * @warning str has to exist forever GUI will just store pointer to this string, and draw it even after this functione exits.
 * @return true if progress changed, false if this progress was already displayed
 */
bool gui_bootstrap_screen_set_state(unsigned percent, const char *str);
