#include "app_metrics.h"
#include "metric.h"
#include "version.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "malloc.h"
#include "heap.h"
#include "media.h"
#include "timing.h"

#include "../Marlin/src/module/temperature.h"
#include "../Marlin/src/module/planner.h"
#include "../Marlin/src/module/stepper.h"
#include "../Marlin/src/feature/power.h"

void Buddy::Metrics::RecordRuntimeStats() {
    static metric_t fw_version = METRIC("fw_version", METRIC_VALUE_STRING, 1 * 1000, METRIC_HANDLER_ENABLE_ALL);
    metric_record_string(&fw_version, "%s", project_version_full);

    static metric_t stack = METRIC("stack", METRIC_VALUE_CUSTOM, 0, METRIC_HANDLER_ENABLE_ALL);
    static TaskStatus_t task_statuses[11];
    static uint32_t last_recorded_ticks = 0;
    if (ticks_diff(ticks_ms(), last_recorded_ticks) > 3000) {
        int count = uxTaskGetSystemState(task_statuses, sizeof(task_statuses) / sizeof(task_statuses[1]), NULL);
        for (int idx = 0; idx < count; idx++) {
            const char *stack_base = (char *)task_statuses[idx].pxStackBase;
            size_t s = 0;
            /* We can only report free stack space for heap-allocated stack frames. */
            if (mem_is_heap_allocated(stack_base))
                s = malloc_usable_size((void *)stack_base);
            const char *task_name = task_statuses[idx].pcTaskName;
            if (strcmp(task_name, "Tmr Svc") == 0)
                task_name = "TmrSvc";
            metric_record_custom(&stack, ",n=%.7s t=%i,m=%hu", task_name, s, task_statuses[idx].usStackHighWaterMark);
        }
        last_recorded_ticks = ticks_ms();
    }

    static metric_t heap = METRIC("heap", METRIC_VALUE_CUSTOM, 503, METRIC_HANDLER_ENABLE_ALL);
    metric_record_custom(&heap, " free=%ii,total=%ii", xPortGetFreeHeapSize(), heap_total_size);
}

void Buddy::Metrics::RecordMarlinVariables() {
    static metric_t is_printing = METRIC("is_printing", METRIC_VALUE_INTEGER, 5000, METRIC_HANDLER_ENABLE_ALL);
    metric_record_integer(&is_printing, printingIsActive() ? 1 : 0);

#if HAS_TEMP_BOARD
    static metric_t board
        = METRIC("temp_brd", METRIC_VALUE_FLOAT, 1000 - 9, METRIC_HANDLER_DISABLE_ALL);
    metric_record_float(&board, thermalManager.degBoard());
#endif
    static metric_t bed = METRIC("temp_bed", METRIC_VALUE_FLOAT, 2000 + 23, METRIC_HANDLER_DISABLE_ALL);
    metric_record_float(&bed, thermalManager.degBed());

    static metric_t target_bed = METRIC("ttemp_bed", METRIC_VALUE_INTEGER, 1000, METRIC_HANDLER_DISABLE_ALL);
    metric_record_integer(&target_bed, thermalManager.degTargetBed());

    static metric_t nozzle = METRIC("temp_noz", METRIC_VALUE_FLOAT, 1000 - 10, METRIC_HANDLER_DISABLE_ALL);
    metric_record_float(&nozzle, thermalManager.degHotend(0));

    static metric_t target_nozzle = METRIC("ttemp_noz", METRIC_VALUE_INTEGER, 1000 + 7, METRIC_HANDLER_DISABLE_ALL);
    metric_record_integer(&target_nozzle, thermalManager.degTargetHotend(0));

#if FAN_COUNT >= 1
    static metric_t fan_speed = METRIC("fan_speed", METRIC_VALUE_INTEGER, 501, METRIC_HANDLER_DISABLE_ALL);
    metric_record_integer(&fan_speed, thermalManager.fan_speed[0]);
#endif

    static metric_t ipos_x = METRIC("ipos_x", METRIC_VALUE_INTEGER, 10, METRIC_HANDLER_DISABLE_ALL);
    metric_record_integer(&ipos_x, stepper.position_from_startup(AxisEnum::X_AXIS));
    static metric_t ipos_y = METRIC("ipos_y", METRIC_VALUE_INTEGER, 10, METRIC_HANDLER_DISABLE_ALL);
    metric_record_integer(&ipos_y, stepper.position_from_startup(AxisEnum::Y_AXIS));
    static metric_t ipos_z = METRIC("ipos_z", METRIC_VALUE_INTEGER, 10, METRIC_HANDLER_DISABLE_ALL);
    metric_record_integer(&ipos_z, stepper.position_from_startup(AxisEnum::Z_AXIS));

    static metric_t pos_x = METRIC("pos_x", METRIC_VALUE_FLOAT, 11, METRIC_HANDLER_DISABLE_ALL);
    metric_record_float(&pos_x, planner.get_axis_position_mm(AxisEnum::X_AXIS));
    static metric_t pos_y = METRIC("pos_y", METRIC_VALUE_FLOAT, 11, METRIC_HANDLER_DISABLE_ALL);
    metric_record_float(&pos_y, planner.get_axis_position_mm(AxisEnum::Y_AXIS));
    static metric_t pos_z = METRIC("pos_z", METRIC_VALUE_FLOAT, 11, METRIC_HANDLER_DISABLE_ALL);
    metric_record_float(&pos_z, planner.get_axis_position_mm(AxisEnum::Z_AXIS));
}

void Buddy::Metrics::RecordPrintFilename() {
    static metric_t file_name = METRIC("print_filename", METRIC_VALUE_STRING, 5000, METRIC_HANDLER_ENABLE_ALL);
    if (media_print_get_state() != media_print_state_t::media_print_state_NONE) {
        //The docstring for media_print_filename() advises against using this function; however, there is currently no replacement for it.
        metric_record_string(&file_name, "%s", media_print_filename());
    } else {
        metric_record_string(&file_name, "");
    }
}
