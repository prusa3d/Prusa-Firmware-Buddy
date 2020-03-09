#pragma once

#include <stdint.h>
#include "dialogs.h" //dialog_t

#ifdef __cplusplus
//******************
// Begin of C++ code

// End of C++ code
//******************
extern "C" {
#endif
//******************
// Begin of C code
void dialog_open_cb(dialog_t dialog, uint8_t data);
void dialog_close_cb(dialog_t dialog);
void dialog_change_cb(dialog_t dialog, uint8_t phase, uint8_t progress_tot, uint8_t progress);
// End of C code
//******************
#ifdef __cplusplus
}
#endif
