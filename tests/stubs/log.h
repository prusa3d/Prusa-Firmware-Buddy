#pragma once

/// This implements logging stubs to be used in tests

#define log_debug(...)
#define log_info(...)
#define log_error(...)
#define log_warning(...)
#define log_critical(...)

#define LOG_COMPONENT(name) FAKE_LOG_COMPONENT_##name
#define LOG_COMPONENT_DEF(name, severity)
#define LOG_COMPONENT_REF(component)

typedef int log_component_t;
