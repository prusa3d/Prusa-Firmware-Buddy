// marlin_vars.c

#include "marlin_vars.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

// variable name constants (dbg)
const char *__var_name[] = {
    "MOTION",
    "GQUEUE",
    "PQUEUE",
    "IPOS_X",
    "IPOS_Y",
    "IPOS_Z",
    "IPOS_E",
    "POS_X",
    "POS_Y",
    "POS_Z",
    "POS_E",
    "TEMP_NOZ",
    "TEMP_BED",
    "TTEM_NOZ",
    "TTEM_BED",
    "Z_OFFSET",
    "FANSPEED",
    "PRNSPEED",
    "FLOWFACT",
    "WAITHEAT",
    "WAITUSER",
    "SD_PDONE",
    "DURATION",
    "MEDIAINS",
    "PRN_STAT",
    "FILENAME",
    "FILEPATH",
    "DTEM_NOZ",
    "TIMTOEND",
    "FANPR_RPM",
    "FANHB_RPM",
    "FAN_CHECK_ENABLED",
    "ENDSTOPS",
    "FS_AUTOLOAD_ENABLED",
    "JOB_ID",
    "CURR_POS_X",
    "CURR_POS_Y",
    "CURR_POS_Z",
    "CURR_POS_E",
    "TRAVEL_ACCEL"
};

static_assert((sizeof(__var_name) / sizeof(char *)) == (MARLIN_VAR_MAX + 1), "Invalid number of elements in __var_name");

const char *marlin_vars_get_name(marlin_var_id_t var_id) {
    if (var_id <= MARLIN_VAR_MAX)
        return __var_name[var_id];
    return "";
}

marlin_var_id_t marlin_vars_get_id_by_name(const char *var_name) {
    for (int i = 0; i <= MARLIN_VAR_MAX; i++)
        if (strcmp(var_name, __var_name[i]) == 0)
            return i;
    return MARLIN_VAR_MAX + 1;
}

variant8_t marlin_vars_get_var(marlin_vars_t *vars, marlin_var_id_t var_id) {
    if (!vars)
        // FIXME: send no variant (needs new variant type) instead of empty one.
        // Empty variant is a usable / sendable variant.
        return variant8_empty();

    switch (var_id) {
    case MARLIN_VAR_MOTION:
        return variant8_ui8(vars->motion);
    case MARLIN_VAR_GQUEUE:
        return variant8_ui8(vars->gqueue);
    case MARLIN_VAR_PQUEUE:
        return variant8_ui8(vars->pqueue);
    case MARLIN_VAR_IPOS_X:
        return variant8_i32(vars->ipos[0]);
    case MARLIN_VAR_IPOS_Y:
        return variant8_i32(vars->ipos[1]);
    case MARLIN_VAR_IPOS_Z:
        return variant8_i32(vars->ipos[2]);
    case MARLIN_VAR_IPOS_E:
        return variant8_i32(vars->ipos[3]);
    case MARLIN_VAR_POS_X:
        return variant8_flt(vars->pos[0]);
    case MARLIN_VAR_POS_Y:
        return variant8_flt(vars->pos[1]);
    case MARLIN_VAR_POS_Z:
        return variant8_flt(vars->pos[2]);
    case MARLIN_VAR_POS_E:
        return variant8_flt(vars->pos[3]);
    case MARLIN_VAR_TEMP_NOZ:
        return variant8_flt(vars->temp_nozzle);
    case MARLIN_VAR_TEMP_BED:
        return variant8_flt(vars->temp_bed);
    case MARLIN_VAR_TTEM_NOZ:
        return variant8_flt(vars->target_nozzle);
    case MARLIN_VAR_TTEM_BED:
        return variant8_flt(vars->target_bed);
    case MARLIN_VAR_Z_OFFSET:
        return variant8_flt(vars->z_offset);
    case MARLIN_VAR_FANSPEED:
        return variant8_ui8(vars->print_fan_speed);
    case MARLIN_VAR_PRNSPEED:
        return variant8_ui16(vars->print_speed);
    case MARLIN_VAR_FLOWFACT:
        return variant8_ui16(vars->flow_factor);
    case MARLIN_VAR_WAITHEAT:
        return variant8_bool(vars->wait_heat);
    case MARLIN_VAR_WAITUSER:
        return variant8_bool(vars->wait_user);
    case MARLIN_VAR_SD_PDONE:
        return variant8_ui8(vars->sd_percent_done);
    case MARLIN_VAR_DURATION:
        return variant8_ui32(vars->print_duration);
    case MARLIN_VAR_MEDIAINS:
        return variant8_bool(vars->media_inserted);
    case MARLIN_VAR_PRNSTATE:
        return variant8_ui8(vars->print_state);
    case MARLIN_VAR_FILENAME:
        return variant8_pchar(vars->media_LFN, 0, 1);
    case MARLIN_VAR_FILEPATH:
        return variant8_pchar(vars->media_SFN_path, 0, 1);
    case MARLIN_VAR_DTEM_NOZ:
        return variant8_flt(vars->display_nozzle);
    case MARLIN_VAR_TIMTOEND:
        return variant8_ui32(vars->time_to_end);
    case MARLIN_VAR_PRINT_FAN_RPM:
        return variant8_ui16(vars->print_fan_rpm);
    case MARLIN_VAR_HEATBREAK_FAN_RPM:
        return variant8_ui16(vars->heatbreak_fan_rpm);
    case MARLIN_VAR_FAN_CHECK_ENABLED:
        return variant8_bool(vars->fan_check_enabled);
    case MARLIN_VAR_ENDSTOPS:
        return variant8_ui32(vars->endstops);
    case MARLIN_VAR_FS_AUTOLOAD_ENABLED:
        return variant8_bool(vars->fs_autoload_enabled);
    case MARLIN_VAR_JOB_ID:
        return variant8_ui16(vars->job_id);
    case MARLIN_VAR_CURR_POS_X:
        return variant8_flt(vars->curr_pos[0]);
    case MARLIN_VAR_CURR_POS_Y:
        return variant8_flt(vars->curr_pos[1]);
    case MARLIN_VAR_CURR_POS_Z:
        return variant8_flt(vars->curr_pos[2]);
    case MARLIN_VAR_CURR_POS_E:
        return variant8_flt(vars->curr_pos[3]);
    case MARLIN_VAR_TRAVEL_ACCEL:
        return variant8_flt(vars->travel_acceleration);
    }

    abort(); // unreachable
}

void marlin_vars_set_var(marlin_vars_t *vars, marlin_var_id_t var_id, variant8_t var) {
    if (!vars)
        return;

    switch (var_id) {
    case MARLIN_VAR_MOTION:
        vars->motion = variant8_get_ui8(var);
        break;
    case MARLIN_VAR_GQUEUE:
        vars->gqueue = variant8_get_ui8(var);
        break;
    case MARLIN_VAR_PQUEUE:
        vars->pqueue = variant8_get_ui8(var);
        break;
    case MARLIN_VAR_IPOS_X:
        vars->ipos[0] = variant8_get_i32(var);
        break;
    case MARLIN_VAR_IPOS_Y:
        vars->ipos[1] = variant8_get_i32(var);
        break;
    case MARLIN_VAR_IPOS_Z:
        vars->ipos[2] = variant8_get_i32(var);
        break;
    case MARLIN_VAR_IPOS_E:
        vars->ipos[3] = variant8_get_i32(var);
        break;
    case MARLIN_VAR_POS_X:
        vars->pos[0] = variant8_get_flt(var);
        break;
    case MARLIN_VAR_POS_Y:
        vars->pos[1] = variant8_get_flt(var);
        break;
    case MARLIN_VAR_POS_Z:
        vars->pos[2] = variant8_get_flt(var);
        break;
    case MARLIN_VAR_POS_E:
        vars->pos[3] = variant8_get_flt(var);
        break;
    case MARLIN_VAR_CURR_POS_X:
        vars->curr_pos[0] = variant8_get_flt(var);
        break;
    case MARLIN_VAR_CURR_POS_Y:
        vars->curr_pos[1] = variant8_get_flt(var);
        break;
    case MARLIN_VAR_CURR_POS_Z:
        vars->curr_pos[2] = variant8_get_flt(var);
        break;
    case MARLIN_VAR_CURR_POS_E:
        vars->curr_pos[3] = variant8_get_flt(var);
        break;
    case MARLIN_VAR_TEMP_NOZ:
        vars->temp_nozzle = variant8_get_flt(var);
        break;
    case MARLIN_VAR_TEMP_BED:
        vars->temp_bed = variant8_get_flt(var);
        break;
    case MARLIN_VAR_TTEM_NOZ:
        vars->target_nozzle = variant8_get_flt(var);
        break;
    case MARLIN_VAR_TTEM_BED:
        vars->target_bed = variant8_get_flt(var);
        break;
    case MARLIN_VAR_Z_OFFSET:
        vars->z_offset = variant8_get_flt(var);
        break;
    case MARLIN_VAR_FANSPEED:
        vars->print_fan_speed = variant8_get_ui8(var);
        break;
    case MARLIN_VAR_PRNSPEED:
        vars->print_speed = variant8_get_ui16(var);
        break;
    case MARLIN_VAR_FLOWFACT:
        vars->flow_factor = variant8_get_ui16(var);
        break;
    case MARLIN_VAR_WAITHEAT:
        vars->wait_heat = variant8_get_bool(var);
        break;
    case MARLIN_VAR_WAITUSER:
        vars->wait_user = variant8_get_bool(var);
        break;
    case MARLIN_VAR_SD_PDONE:
        vars->sd_percent_done = variant8_get_ui8(var);
        break;
    case MARLIN_VAR_DURATION:
        vars->print_duration = variant8_get_ui32(var);
        break;
    case MARLIN_VAR_MEDIAINS:
        vars->media_inserted = variant8_get_bool(var);
        break;
    case MARLIN_VAR_PRNSTATE:
        vars->print_state = variant8_get_ui8(var);
        break;
    case MARLIN_VAR_FILENAME:
        if (vars->media_LFN)
            if (variant8_get_type(var) == VARIANT8_PCHAR) {
                char *filename = variant8_get_pch(var);
                memset(vars->media_LFN, '\0', FILE_NAME_BUFFER_LEN * sizeof(char)); // set to zeros to be on the safe side
                strlcpy(vars->media_LFN, filename, FILE_NAME_BUFFER_LEN);
            }
        break;
    case MARLIN_VAR_FILEPATH:
        if (vars->media_SFN_path)
            if (variant8_get_type(var) == VARIANT8_PCHAR) {
                char *filepath = variant8_get_pch(var);
                memset(vars->media_SFN_path, '\0', FILE_PATH_BUFFER_LEN * sizeof(char)); // set to zeros to be on the safe side
                strlcpy(vars->media_SFN_path, filepath, FILE_PATH_BUFFER_LEN);
            }
        break;
    case MARLIN_VAR_DTEM_NOZ:
        vars->display_nozzle = variant8_get_flt(var);
        break;
    case MARLIN_VAR_TIMTOEND:
        vars->time_to_end = variant8_get_ui32(var);
        break;
    case MARLIN_VAR_PRINT_FAN_RPM:
        vars->print_fan_rpm = variant8_get_ui16(var);
        break;
    case MARLIN_VAR_HEATBREAK_FAN_RPM:
        vars->heatbreak_fan_rpm = variant8_get_ui16(var);
        break;
    case MARLIN_VAR_FAN_CHECK_ENABLED:
        vars->fan_check_enabled = variant8_get_bool(var);
        break;
    case MARLIN_VAR_ENDSTOPS:
        vars->endstops = variant8_get_ui32(var);
        break;
    case MARLIN_VAR_FS_AUTOLOAD_ENABLED:
        vars->fs_autoload_enabled = variant8_get_bool(var);
        break;
    case MARLIN_VAR_JOB_ID:
        vars->job_id = variant8_get_ui16(var);
        break;
    case MARLIN_VAR_TRAVEL_ACCEL:
        vars->travel_acceleration = variant8_get_flt(var);
        break;
    }
}

int marlin_vars_value_to_str(marlin_vars_t *vars, marlin_var_id_t var_id, char *str, unsigned int size) {
    if (!vars)
        return 0;

    switch (var_id) {
    case MARLIN_VAR_MOTION:
        return snprintf(str, size, "%u", (unsigned int)(vars->motion));
    case MARLIN_VAR_GQUEUE:
        return snprintf(str, size, "%u", (unsigned int)(vars->gqueue));
    case MARLIN_VAR_PQUEUE:
        return snprintf(str, size, "%u", (unsigned int)(vars->pqueue));
    case MARLIN_VAR_IPOS_X:
    case MARLIN_VAR_IPOS_Y:
    case MARLIN_VAR_IPOS_Z:
    case MARLIN_VAR_IPOS_E:
        return snprintf(str, size, "%li", (long int)vars->ipos[var_id - MARLIN_VAR_IPOS_X]);
    case MARLIN_VAR_POS_X:
    case MARLIN_VAR_POS_Y:
    case MARLIN_VAR_POS_Z:
    case MARLIN_VAR_POS_E:
        return snprintf(str, size, "%.3f", (double)(vars->pos[var_id - MARLIN_VAR_POS_X]));
    case MARLIN_VAR_TEMP_NOZ:
        return snprintf(str, size, "%.1f", (double)(vars->temp_nozzle));
    case MARLIN_VAR_TEMP_BED:
        return snprintf(str, size, "%.1f", (double)(vars->temp_bed));
    case MARLIN_VAR_TTEM_NOZ:
        return snprintf(str, size, "%.1f", (double)(vars->target_nozzle));
    case MARLIN_VAR_TTEM_BED:
        return snprintf(str, size, "%.1f", (double)(vars->target_bed));
    case MARLIN_VAR_Z_OFFSET:
        return snprintf(str, size, "%.4f", (double)(vars->z_offset));
    case MARLIN_VAR_FANSPEED:
        return snprintf(str, size, "%u", (unsigned int)(vars->print_fan_speed));
    case MARLIN_VAR_PRNSPEED:
        return snprintf(str, size, "%u", (unsigned int)(vars->print_speed));
    case MARLIN_VAR_FLOWFACT:
        return snprintf(str, size, "%u", (unsigned int)(vars->flow_factor));
    case MARLIN_VAR_WAITHEAT:
        return snprintf(str, size, "%u", (unsigned int)(vars->wait_heat));
    case MARLIN_VAR_WAITUSER:
        return snprintf(str, size, "%u", (unsigned int)(vars->wait_user));
    case MARLIN_VAR_SD_PDONE:
        return snprintf(str, size, "%u", (unsigned int)(vars->sd_percent_done));
    case MARLIN_VAR_DURATION:
        return snprintf(str, size, "%lu", (long unsigned int)(vars->print_duration));
    case MARLIN_VAR_MEDIAINS:
        return snprintf(str, size, "%u", (unsigned int)(vars->media_inserted));
    case MARLIN_VAR_PRNSTATE:
        return snprintf(str, size, "%u", (unsigned int)(vars->print_state));
    case MARLIN_VAR_FILENAME:
        return snprintf(str, size, "%s", vars->media_LFN);
    case MARLIN_VAR_FILEPATH:
        return snprintf(str, size, "%s", vars->media_SFN_path);
    case MARLIN_VAR_DTEM_NOZ:
        return snprintf(str, size, "%.1f", (double)(vars->display_nozzle));
    case MARLIN_VAR_TIMTOEND:
        return snprintf(str, size, "%lu", (long unsigned int)(vars->time_to_end));
    case MARLIN_VAR_PRINT_FAN_RPM:
        return snprintf(str, size, "%u", (unsigned int)(vars->print_fan_rpm));
    case MARLIN_VAR_HEATBREAK_FAN_RPM:
        return snprintf(str, size, "%u", (unsigned int)(vars->heatbreak_fan_rpm));
    case MARLIN_VAR_FAN_CHECK_ENABLED:
        return snprintf(str, size, "%u", (unsigned int)(vars->fan_check_enabled));
    case MARLIN_VAR_ENDSTOPS:
        return snprintf(str, size, "%lu", (unsigned long int)(vars->endstops));
    case MARLIN_VAR_FS_AUTOLOAD_ENABLED:
        return snprintf(str, size, "%u", (unsigned int)(vars->fs_autoload_enabled));
    case MARLIN_VAR_JOB_ID:
        return snprintf(str, size, "%hu", vars->job_id);
    case MARLIN_VAR_TRAVEL_ACCEL:
        return snprintf(str, size, "%f", (double)(vars->travel_acceleration));
    default:
        return snprintf(str, size, "???");
    }
    return 0;
}

int marlin_vars_str_to_value(marlin_vars_t *vars, marlin_var_id_t var_id, const char *str) {
    if (!vars)
        return 0;

    switch (var_id) {
    case MARLIN_VAR_MOTION:
        return sscanf(str, "%hhu", &(vars->motion));
    case MARLIN_VAR_GQUEUE:
        return sscanf(str, "%hhu", &(vars->gqueue));
    case MARLIN_VAR_PQUEUE:
        return sscanf(str, "%hhu", &(vars->pqueue));
    case MARLIN_VAR_IPOS_X:
    case MARLIN_VAR_IPOS_Y:
    case MARLIN_VAR_IPOS_Z:
    case MARLIN_VAR_IPOS_E:
        return sscanf(str, "%li", &(vars->ipos[var_id - MARLIN_VAR_IPOS_X]));
    case MARLIN_VAR_POS_X:
    case MARLIN_VAR_POS_Y:
    case MARLIN_VAR_POS_Z:
    case MARLIN_VAR_POS_E:
        return sscanf(str, "%f", &(vars->pos[var_id - MARLIN_VAR_POS_X]));
    case MARLIN_VAR_CURR_POS_X:
    case MARLIN_VAR_CURR_POS_Y:
    case MARLIN_VAR_CURR_POS_Z:
    case MARLIN_VAR_CURR_POS_E:
        return sscanf(str, "%f", &(vars->curr_pos[var_id - MARLIN_VAR_CURR_POS_X]));
    case MARLIN_VAR_TEMP_NOZ:
        return sscanf(str, "%f", &(vars->temp_nozzle));
    case MARLIN_VAR_TEMP_BED:
        return sscanf(str, "%f", &(vars->temp_bed));
    case MARLIN_VAR_TTEM_NOZ:
        return sscanf(str, "%f", &(vars->target_nozzle));
    case MARLIN_VAR_TTEM_BED:
        return sscanf(str, "%f", &(vars->target_bed));
    case MARLIN_VAR_Z_OFFSET:
        return sscanf(str, "%f", &(vars->z_offset));
    case MARLIN_VAR_FANSPEED:
        return sscanf(str, "%hhu", &(vars->print_fan_speed));
    case MARLIN_VAR_PRNSPEED:
        return sscanf(str, "%hu", (unsigned short *)&(vars->print_speed));
    case MARLIN_VAR_FLOWFACT:
        return sscanf(str, "%hu", (unsigned short *)&(vars->flow_factor));
    case MARLIN_VAR_WAITHEAT:
        return sscanf(str, "%hhu", &(vars->wait_heat));
    case MARLIN_VAR_WAITUSER:
        return sscanf(str, "%hhu", &(vars->wait_user));
    case MARLIN_VAR_SD_PDONE:
        return sscanf(str, "%hhu", &(vars->sd_percent_done));
    case MARLIN_VAR_DURATION:
        return sscanf(str, "%lu", &(vars->print_duration));
    case MARLIN_VAR_MEDIAINS:
        return sscanf(str, "%hhu", &(vars->media_inserted));
    case MARLIN_VAR_PRNSTATE:
        return sscanf(str, "%hhu", &(vars->print_state));
    case MARLIN_VAR_FILENAME:
        return sscanf(str, "%s", (vars->media_LFN));
    case MARLIN_VAR_FILEPATH:
        return sscanf(str, "%s", (vars->media_SFN_path));
    case MARLIN_VAR_DTEM_NOZ:
        return sscanf(str, "%f", &(vars->display_nozzle));
    case MARLIN_VAR_TIMTOEND:
        return sscanf(str, "%lu", &(vars->time_to_end));
    case MARLIN_VAR_PRINT_FAN_RPM:
        return sscanf(str, "%hu", &(vars->print_fan_rpm));
    case MARLIN_VAR_HEATBREAK_FAN_RPM:
        return sscanf(str, "%hu", &(vars->heatbreak_fan_rpm));
    case MARLIN_VAR_FAN_CHECK_ENABLED:
        return sscanf(str, "%hhu", &(vars->fan_check_enabled));
    case MARLIN_VAR_ENDSTOPS:
        return sscanf(str, "%lu", &(vars->endstops));
    case MARLIN_VAR_FS_AUTOLOAD_ENABLED:
        return sscanf(str, "%hhu", &(vars->fs_autoload_enabled));
    case MARLIN_VAR_JOB_ID:
        return sscanf(str, "%hu", &(vars->job_id));
    case MARLIN_VAR_TRAVEL_ACCEL:
        return sscanf(str, "%f", &(vars->travel_acceleration));
    }

    return 0;
}

void marlin_msg_to_str(const marlin_msg_t id, char *str) {
    str[0] = '!';
    str[1] = (char)id;
    str[2] = 0;
}
