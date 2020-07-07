// errors.h
#pragma once

#include "errors_ext.h"

extern void set_actual_error(err_num_t err_item);
extern const err_t *get_actual_error(void);
