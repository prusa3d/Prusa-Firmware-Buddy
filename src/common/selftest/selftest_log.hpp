/**
 * @file selftest_log.hpp
 * @author Radek Vana
 * @brief Workaround for logging to work inside selftest namespace
 * @date 2021-10-12

 */
#pragma once
#include "log.h"

namespace selftest {
#define LogInfo(...)  _log_event(LOG_SEVERITY_INFO, log_component_find("Selftest"), __VA_ARGS__);
#define LogDebug(...) _log_event(LOG_SEVERITY_DEBUG, log_component_find("Selftest"), __VA_ARGS__);
}
