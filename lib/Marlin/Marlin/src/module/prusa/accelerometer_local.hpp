#pragma once

// One-time boot init
void prusa_accelerometer_local_init();

void prusa_accelerometer_handle_polling();
void prusa_accelerometer_handle_spi_finish();
