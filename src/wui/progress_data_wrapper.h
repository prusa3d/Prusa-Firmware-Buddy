#ifndef PROGRESS_DATA_WRAPPER_H
#define PROGRESS_DATA_WRAPPER_H

#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct ClProgressData ClProgressData;

bool is_percentage_valid(uint32_t print_dur);

uint8_t progress_get_percentage();

void progress_format_time2end(char *dest, uint16_t feedrate);

void print_dur_to_string(char *buffer, uint32_t print_dur);

#ifdef __cplusplus
}
#endif

#endif //PROGRESS_DATA_WRAPPER_H
