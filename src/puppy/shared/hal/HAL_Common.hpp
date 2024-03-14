#pragma once

#define CRITICAL_SECTION_START              \
    uint32_t tmp_primask = __get_PRIMASK(); \
    __set_PRIMASK(1)

#define CRITICAL_SECTION_END __set_PRIMASK(tmp_primask)

extern "C" {

void Error_Handler(void);
}
