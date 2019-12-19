// marlin_vars.c

#include "marlin_vars.h"
#include <stdio.h>
#include <string.h>

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
    "SD_PRINT",
    "SD_PDONE",
    "DURATION",
    "FSENSOR",
};

const char *marlin_vars_get_name(uint8_t var_id) {
    if (var_id <= MARLIN_VAR_MAX)
        return __var_name[var_id];
    return "";
}

int marlin_vars_get_id_by_name(const char *var_name) {
    int i;
    for (i = 0; i <= MARLIN_VAR_MAX; i++)
        if (strcmp(var_name, __var_name[i]) == 0)
            return i;
    return -1;
}

variant8_t marlin_vars_get_var(marlin_vars_t *vars, uint8_t var_id) {
    if (vars)
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
            return variant8_ui8(vars->fan_speed);
        case MARLIN_VAR_PRNSPEED:
            return variant8_ui16(vars->print_speed);
        case MARLIN_VAR_FLOWFACT:
            return variant8_ui16(vars->flow_factor);
        case MARLIN_VAR_WAITHEAT:
            return variant8_ui8(vars->wait_heat);
        case MARLIN_VAR_WAITUSER:
            return variant8_ui8(vars->wait_user);
        case MARLIN_VAR_SD_PRINT:
            return variant8_ui8(vars->sd_printing);
        case MARLIN_VAR_SD_PDONE:
            return variant8_ui8(vars->sd_percent_done);
        case MARLIN_VAR_DURATION:
            return variant8_ui32(vars->print_duration);
        }
    return variant8_empty();
}

void marlin_vars_set_var(marlin_vars_t *vars, uint8_t var_id, variant8_t var) {
    if (vars)
        switch (var_id) {
        case MARLIN_VAR_MOTION:
            vars->motion = var.ui8;
            break;
        case MARLIN_VAR_GQUEUE:
            vars->gqueue = var.ui8;
            break;
        case MARLIN_VAR_PQUEUE:
            vars->pqueue = var.ui8;
            break;
        case MARLIN_VAR_IPOS_X:
            vars->ipos[0] = var.i32;
            break;
        case MARLIN_VAR_IPOS_Y:
            vars->ipos[1] = var.i32;
            break;
        case MARLIN_VAR_IPOS_Z:
            vars->ipos[2] = var.i32;
            break;
        case MARLIN_VAR_IPOS_E:
            vars->ipos[3] = var.i32;
            break;
        case MARLIN_VAR_POS_X:
            vars->pos[0] = var.flt;
            break;
        case MARLIN_VAR_POS_Y:
            vars->pos[1] = var.flt;
            break;
        case MARLIN_VAR_POS_Z:
            vars->pos[2] = var.flt;
            break;
        case MARLIN_VAR_POS_E:
            vars->pos[3] = var.flt;
            break;
        case MARLIN_VAR_TEMP_NOZ:
            vars->temp_nozzle = var.flt;
            break;
        case MARLIN_VAR_TEMP_BED:
            vars->temp_bed = var.flt;
            break;
        case MARLIN_VAR_TTEM_NOZ:
            vars->target_nozzle = var.flt;
            break;
        case MARLIN_VAR_TTEM_BED:
            vars->target_bed = var.flt;
            break;
        case MARLIN_VAR_Z_OFFSET:
            vars->z_offset = var.flt;
            break;
        case MARLIN_VAR_FANSPEED:
            vars->fan_speed = var.ui8;
            break;
        case MARLIN_VAR_PRNSPEED:
            vars->print_speed = var.ui16;
            break;
        case MARLIN_VAR_FLOWFACT:
            vars->flow_factor = var.ui16;
            break;
        case MARLIN_VAR_WAITHEAT:
            vars->wait_heat = var.ui8;
            break;
        case MARLIN_VAR_WAITUSER:
            vars->wait_user = var.ui8;
            break;
        case MARLIN_VAR_SD_PRINT:
            vars->sd_printing = var.ui8;
            break;
        case MARLIN_VAR_SD_PDONE:
            vars->sd_percent_done = var.ui8;
            break;
        case MARLIN_VAR_DURATION:
            vars->print_duration = var.ui32;
            break;
        }
}

void marlin_vars_value_to_str(marlin_vars_t *vars, uint8_t var_id, char *str) {
    if (vars)
        switch (var_id) {
        case MARLIN_VAR_MOTION:
            sprintf(str, "%u", (unsigned int)(vars->motion));
            break;
        case MARLIN_VAR_GQUEUE:
            sprintf(str, "%u", (unsigned int)(vars->gqueue));
            break;
        case MARLIN_VAR_PQUEUE:
            sprintf(str, "%u", (unsigned int)(vars->pqueue));
            break;
        case MARLIN_VAR_IPOS_X:
        case MARLIN_VAR_IPOS_Y:
        case MARLIN_VAR_IPOS_Z:
        case MARLIN_VAR_IPOS_E:
            sprintf(str, "%li", (long int)vars->ipos[var_id - MARLIN_VAR_IPOS_X]);
            break;
        case MARLIN_VAR_POS_X:
        case MARLIN_VAR_POS_Y:
        case MARLIN_VAR_POS_Z:
        case MARLIN_VAR_POS_E:
            sprintf(str, "%.3f", (double)(vars->pos[var_id - MARLIN_VAR_POS_X]));
            break;
        case MARLIN_VAR_TEMP_NOZ:
            sprintf(str, "%.1f", (double)(vars->temp_nozzle));
            break;
        case MARLIN_VAR_TEMP_BED:
            sprintf(str, "%.1f", (double)(vars->temp_bed));
            break;
        case MARLIN_VAR_TTEM_NOZ:
            sprintf(str, "%.1f", (double)(vars->target_nozzle));
            break;
        case MARLIN_VAR_TTEM_BED:
            sprintf(str, "%.1f", (double)(vars->target_bed));
            break;
        case MARLIN_VAR_Z_OFFSET:
            sprintf(str, "%.4f", (double)(vars->z_offset));
            break;
        case MARLIN_VAR_FANSPEED:
            sprintf(str, "%u", (unsigned int)(vars->fan_speed));
            break;
        case MARLIN_VAR_PRNSPEED:
            sprintf(str, "%u", (unsigned int)(vars->print_speed));
            break;
        case MARLIN_VAR_FLOWFACT:
            sprintf(str, "%u", (unsigned int)(vars->flow_factor));
            break;
        case MARLIN_VAR_WAITHEAT:
            sprintf(str, "%u", (unsigned int)(vars->wait_heat));
            break;
        case MARLIN_VAR_WAITUSER:
            sprintf(str, "%u", (unsigned int)(vars->wait_user));
            break;
        case MARLIN_VAR_SD_PRINT:
            sprintf(str, "%u", (unsigned int)(vars->sd_printing));
            break;
        case MARLIN_VAR_SD_PDONE:
            sprintf(str, "%u", (unsigned int)(vars->sd_percent_done));
            break;
        case MARLIN_VAR_DURATION:
            sprintf(str, "%lu", (long unsigned int)(vars->print_duration));
            break;
        default:
            sprintf(str, "???");
        }
}

int marlin_vars_str_to_value(marlin_vars_t *vars, uint8_t var_id, const char *str) {
    int ret = 0;
    if (vars)
        switch (var_id) {
        case MARLIN_VAR_MOTION:
            ret = sscanf(str, "%hhu", &(vars->motion));
            break;
        case MARLIN_VAR_GQUEUE:
            ret = sscanf(str, "%hhu", &(vars->gqueue));
            break;
        case MARLIN_VAR_PQUEUE:
            ret = sscanf(str, "%hhu", &(vars->pqueue));
            break;
        case MARLIN_VAR_IPOS_X:
        case MARLIN_VAR_IPOS_Y:
        case MARLIN_VAR_IPOS_Z:
        case MARLIN_VAR_IPOS_E:
            ret = sscanf(str, "%li", &(vars->ipos[var_id - MARLIN_VAR_IPOS_X]));
            break;
        case MARLIN_VAR_POS_X:
        case MARLIN_VAR_POS_Y:
        case MARLIN_VAR_POS_Z:
        case MARLIN_VAR_POS_E:
            ret = sscanf(str, "%f", &(vars->pos[var_id - MARLIN_VAR_POS_X]));
            break;
        case MARLIN_VAR_TEMP_NOZ:
            ret = sscanf(str, "%f", &(vars->temp_nozzle));
            break;
        case MARLIN_VAR_TEMP_BED:
            ret = sscanf(str, "%f", &(vars->temp_bed));
            break;
        case MARLIN_VAR_TTEM_NOZ:
            ret = sscanf(str, "%f", &(vars->target_nozzle));
            break;
        case MARLIN_VAR_TTEM_BED:
            ret = sscanf(str, "%f", &(vars->target_bed));
            break;
        case MARLIN_VAR_Z_OFFSET:
            ret = sscanf(str, "%f", &(vars->z_offset));
            break;
        case MARLIN_VAR_FANSPEED:
            ret = sscanf(str, "%hhu", &(vars->fan_speed));
            break;
        case MARLIN_VAR_PRNSPEED:
            ret = sscanf(str, "%hu", (unsigned short *)&(vars->print_speed));
            break;
        case MARLIN_VAR_FLOWFACT:
            ret = sscanf(str, "%hu", (unsigned short *)&(vars->flow_factor));
            break;
        case MARLIN_VAR_WAITHEAT:
            ret = sscanf(str, "%hhu", &(vars->wait_heat));
            break;
        case MARLIN_VAR_WAITUSER:
            ret = sscanf(str, "%hhu", &(vars->wait_user));
            break;
        case MARLIN_VAR_SD_PRINT:
            ret = sscanf(str, "%hhu", &(vars->sd_printing));
            break;
        case MARLIN_VAR_SD_PDONE:
            ret = sscanf(str, "%hhu", &(vars->sd_percent_done));
            break;
        case MARLIN_VAR_DURATION:
            ret = sscanf(str, "%lu", &(vars->print_duration));
            break;
        }
    return ret;
}
