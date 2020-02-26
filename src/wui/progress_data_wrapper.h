#ifndef PROGRESS_DATA_WRAPPER_H
#define PROGRESS_DATA_WRAPPER_H

#include <stdbool.h>
#include <inttypes.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct ClProgressData ClProgressData;

bool is_percentage_valid(uint32_t print_dur);

uint8_t progress_get_percentage();

void progress_format_time2end(char *dest, uint16_t feedrate);

/*void to_string(char *buffer) const {
        int d = this->day(),
            h = this->hour() % 24,
            m = this->minute() % 60,
            s = this->second() % 60;

        if (d) {
            sprintf(buffer, "%3id %2ih %2im", d, h, m);
        } else if (h) {
            sprintf(buffer, "     %2ih %2im", h, m);
        } else if (m) {
            sprintf(buffer, "     %2im %2is", m, s);
        } else {
            sprintf(buffer, "         %2is", s);
        }
    }
*/

#ifdef __cplusplus
}
#endif

#endif //PROGRESS_DATA_WRAPPER_H
