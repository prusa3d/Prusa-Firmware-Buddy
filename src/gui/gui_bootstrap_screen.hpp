/**
 * Loading screen for bootstraping firmware.
 */
#pragma once

/**
 * Initialize the screen - have to be called before everything else
 */
void gui_bootstrap_screen_init();

/**
 * @brief Show the screen, redraw when necessary and wait untill all bootstraps are done
 */
void gui_bootstrap_screen_run();

/**
 * @brief Update bootstrap screen state
 * @note This is expected to be called for different threads.
 * @warning str has to exist forever GUI will just store pointer to this string, and draw it even after this functione exits.
 */
void gui_bootstrap_screen_set_state(unsigned percent, const char *str);

/**
 * @brief Return last displayed percent
 */
unsigned gui_bootstrap_screen_get_percent();

/**
 * @brief Delete resources used by bootstrap screen
 */
void gui_bootstrap_screen_delete();
