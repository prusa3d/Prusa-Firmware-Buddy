#pragma once

typedef int metric_t;

#define METRIC(...) 0

#define metric_record_float(X, ...)   (void)X
#define metric_record_integer(X, ...) (void)X
#define metric_record_string(X, ...)  (void)X
#define metric_record_event(X, ...)   (void)X
#define metric_record_custom(X, ...)  (void)X
#define metric_record_error(X, ...)   (void)X
