#include <freertos/critical_section.hpp>

// FreeRTOS.h must be included before task.h
#include <FreeRTOS.h>
#include <task.h>

namespace freertos {

CriticalSection::CriticalSection() { taskENTER_CRITICAL(); }

CriticalSection::~CriticalSection() { taskEXIT_CRITICAL(); }

} // namespace freertos
