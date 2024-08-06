#pragma once
#include "metric.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

enum {
    METRIC_HANDLER_SYSLOG_ID,
};

extern const metric_handler_t metric_handler_syslog;

/// Set up the metrics handlers based on config store
void metrics_reconfigure();

#ifdef __cplusplus
}
#endif //__cplusplus
