#pragma once

#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

extern volatile bool dwarf_init_done;

void startup_task_run();

#ifdef __cplusplus
}
#endif
