// errors.h
#pragma once

#include "error_codes.hpp"

extern void set_actual_error(ErrCode error_code);
extern const ErrDesc *get_actual_error(void);
