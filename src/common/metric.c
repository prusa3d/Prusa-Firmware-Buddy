#include "metric.h"
#include "cmsis_os.h"
#include "timing.h"
#include "log.h"
#include "stm32f4xx_hal.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

static void metric_system_task_run();

// task definition
osThreadDef(metric_system_task, metric_system_task_run, osPriorityAboveNormal,
    0, 375);
static osThreadId metric_system_task;

// queue definition
osMailQDef(metric_system_queue, 5, metric_point_t);
static osMessageQId metric_system_queue;

// internal variables
static metric_handler_t **metric_system_handlers;
static bool metric_system_initialized = false;
static uint16_t dropped_points_count = 0;
static metric_t *metric_linked_list_root = NULL;

// logging component
LOG_COMPONENT_DEF(Metrics, LOG_SEVERITY_INFO);

// internal metrics
metric_t metric_dropped_points = METRIC("points_dropped", METRIC_VALUE_INTEGER, 1000, METRIC_HANDLER_ENABLE_ALL);

void metric_system_init(metric_handler_t *handlers[]) {
    if (metric_system_initialized)
        return;
    metric_system_handlers = handlers;
    metric_system_task = osThreadCreate(osThread(metric_system_task), NULL);
    metric_system_queue = osMailCreate(osMailQ(metric_system_queue), metric_system_task);
    metric_system_initialized = true;
}

metric_handler_t **metric_get_handlers() {
    return metric_system_handlers;
}

metric_t *metric_get_linked_list() {
    return metric_linked_list_root;
}

void metric_linked_list_append(metric_t *metric) {
    metric_t **pointer_next = &metric_linked_list_root;
    while (*pointer_next != NULL)
        pointer_next = &((*pointer_next)->next);
    *pointer_next = metric;
}

static void metric_system_task_run() {
    for (;;) {
        osEvent event = osMailGet(metric_system_queue, osWaitForever);
        metric_point_t *point = (metric_point_t *)event.value.p;

        for (metric_handler_t **handlers = metric_system_handlers; *handlers != NULL; handlers++) {
            metric_handler_t *handler = *handlers;
            bool handler_enabled = point->metric->enabled_handlers & (1 << handler->identifier);
            if (handler_enabled)
                handler->handle_fn(point);
        }

        osMailFree(metric_system_queue, point);
        metric_record_integer(&metric_dropped_points, dropped_points_count);
    }
}

static bool check_min_interval(metric_t *metric) {
    if (metric->min_interval_ms)
        return ticks_diff(ticks_ms(), metric->_last_update_timestamp) >= (int32_t)metric->min_interval_ms;
    else
        return true;
}

static void update_min_interval(metric_t *metric) {
    metric->_last_update_timestamp = ticks_ms();
}

static metric_point_t *point_check_and_prepare(metric_t *metric, metric_value_type_t type) {
    if (!metric_system_initialized)
        return NULL;
    if (!metric->_registered)
        metric_register(metric);
    if (!check_min_interval(metric))
        return NULL;
    if (metric->type != type) {
        log_error(Metrics, "Attempt to record an invalid value type for metric %s", metric->name);
        metric_record_error(metric, "invalid type");
        return NULL;
    }
    if (!metric->enabled_handlers)
        return NULL; // don't try to enqueue if nobody is listening

    metric_point_t *point = (metric_point_t *)osMailAlloc(metric_system_queue, 0);
    if (!point) {
        dropped_points_count += 1;
        return NULL;
    }
    point->metric = metric;
    point->error = false;
    point->timestamp = ticks_ms();
    return point;
}

static void point_enqueue(metric_point_t *recording) {
    metric_t *metric = recording->metric;
    if (osMailPut(metric_system_queue, (void *)recording) == osOK) {
        update_min_interval(metric);
    } else {
        osMailFree(metric_system_queue, recording);
        dropped_points_count += 1;
    }
}

void metric_register(metric_t *metric) {
    if (metric->_registered)
        return;
    for (metric_handler_t **handlers = metric_system_handlers; *handlers != NULL; handlers++) {
        metric_handler_t *handler = *handlers;
        if (handler->on_metric_registered_fn)
            handler->on_metric_registered_fn(metric);
    }
    metric->_registered = true;
    metric_linked_list_append(metric);
}

void metric_record_float(metric_t *metric, float value) {
    metric_point_t *recording = point_check_and_prepare(metric, METRIC_VALUE_FLOAT);
    if (!recording)
        return;
    recording->value_float = value;
    point_enqueue(recording);
}

void metric_record_string(metric_t *metric, const char *fmt, ...) {
    metric_point_t *recording = point_check_and_prepare(metric, METRIC_VALUE_STRING);
    if (!recording)
        return;
    va_list args;
    va_start(args, fmt);
    vsnprintf(recording->value_str, sizeof(recording->value_str), fmt, args);
    va_end(args);
    point_enqueue(recording);
}

void metric_record_custom(metric_t *metric, const char *fmt, ...) {
    metric_point_t *recording = point_check_and_prepare(metric, METRIC_VALUE_CUSTOM);
    if (!recording)
        return;
    va_list args;
    va_start(args, fmt);
    vsnprintf(recording->value_custom, sizeof(recording->value_custom), fmt, args);
    va_end(args);
    point_enqueue(recording);
}

void metric_record_event(metric_t *metric) {
    metric_point_t *recording = point_check_and_prepare(metric, METRIC_VALUE_EVENT);
    if (!recording)
        return;
    point_enqueue(recording);
}

void metric_record_integer(metric_t *metric, int value) {
    metric_point_t *recording = point_check_and_prepare(metric, METRIC_VALUE_INTEGER);
    if (!recording)
        return;
    recording->value_int = value;
    point_enqueue(recording);
}

void metric_record_error(metric_t *metric, const char *fmt, ...) {
    // TODO: we might want separate throttling for errors
    metric_point_t *recording = point_check_and_prepare(metric, metric->type);
    if (!recording)
        return;
    recording->error = true;
    va_list args;
    va_start(args, fmt);
    vsnprintf(recording->error_msg, sizeof(recording->error_msg), fmt, args);
    va_end(args);
    point_enqueue(recording);
}

void metric_enable_for_handler(metric_t *metric, metric_handler_t *handler) {
    metric->enabled_handlers |= (1 << handler->identifier);
}

void metric_disable_for_handler(metric_t *metric, metric_handler_t *handler) {
    metric->enabled_handlers &= ~(1 << handler->identifier);
}
