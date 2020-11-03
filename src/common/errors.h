// errors.h
#pragma once

/// for MINI
#include "../../../Prusa-Error-Codes/12_MINI/errors_list.h"

extern void set_actual_error(err_num_t err_item);
extern const err_t *get_actual_error(void);
