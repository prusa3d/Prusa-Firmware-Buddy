#pragma once

void start_flash_esp_task();

/// Pretend that the ESP flash task has been run and finished.
/// This is to skip the esp flashing in blue and redscreens.
void skip_esp_flashing();
