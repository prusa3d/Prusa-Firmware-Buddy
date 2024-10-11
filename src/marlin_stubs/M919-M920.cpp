#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "../../lib/Marlin/Marlin/src/feature/tmc_util.h"
#include "../../lib/Marlin/Marlin/src/module/stepper/indirection.h"
#include "../../lib/Marlin/Marlin/src/gcode/queue.h"

#include "M919-M920.h"
#include "trinamic.h"

#include <stdint.h>
#include <string>

template <typename TMC>
void tmc_set_reg(TMC &st, uint8_t reg, uint32_t val) {
    st.write(reg, val);
}

template <typename TMC>
void tmc_read_reg(TMC &st, uint8_t reg) {
    st.printLabel();
    char buff[14] = { 0 };
    snprintf(buff, sizeof(buff), " 0x%x", (unsigned int)st.read(reg));
    SERIAL_ECHO_START();
    SERIAL_ECHO(buff);
}

/// command, write acesse, read acesse
tmc_reg_t *text_cmd_to_register(const char *cmd_in, bool write, bool read) {
    tmc_reg_t *tmc_reg = tmc_reg_map;
    while (tmc_reg->cmd_name != NULL) {
        if (!strcmp(tmc_reg->cmd_name, cmd_in) && tmc_reg->write == write) {
            return tmc_reg;
        } else if (!strcmp(tmc_reg->cmd_name, cmd_in) && tmc_reg->read == read) {
            return tmc_reg;
        }
        tmc_reg++;
    }
    return nullptr;
}

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M919: TMC Config Write <a href="https://reprap.org/wiki/G-code#M919:_TMC_Config_Write">M919: TMC Config Write</a>
 *
 * Writes a value to the TMC driver’s register.
 *
 *#### Usage
 *
 *    M919 [ X | Y | Z | E | <TMC reg> | <data> ]
 *
 *#### Parameters
 * - `X` - Write a value to X axis TMC driver's register
 * - `Y` - Write a value to Y axis TMC driver's register
 * - `Z` - Write a value to Z axis TMC driver's register
 * - `E` - Write a value to E axis TMC driver's register
 * - <TMC reg> - Select specific TMC driver's register
 * - <data> - Data to write in selected TMC driver's register
 */

void PrusaGcodeSuite::M919() {
    char cmd[16] = { 0 };
    int val = 0;
    tmc_reg_t *adress_reg;

    LOOP_XYZE(i) {

        if (parser.seen(axis_codes[i]) && parser.string_arg) {

            int read = sscanf(parser.string_arg, "%16s %x", cmd, &val);

            if (read != 2) {
                SERIAL_ERROR_MSG("does not match '<axis> <TMC reg> <data>'");
                return;
            }

            adress_reg = text_cmd_to_register(cmd, true, false);

            if (adress_reg == nullptr) {
                SERIAL_ERROR_MSG("Enter valid TMC reg name");
                return;
            }

            switch (i) {
            case X_AXIS:
                tmc_set_reg(stepperX, adress_reg->reg_adr, val);
                break;
            case Y_AXIS:
                tmc_set_reg(stepperY, adress_reg->reg_adr, val);
                break;
            case Z_AXIS:
                tmc_set_reg(stepperZ, adress_reg->reg_adr, val);
                break;
            case E_AXIS:
                tmc_set_reg(stepperE0, adress_reg->reg_adr, val);
                break;
            }
            SERIAL_ECHO_START();
            SERIAL_ECHOLNPAIR("TMC register updated: ", parser.string_arg);
        }
    }
    queue.ok_to_send();
}

/**
 *### M920: TMC Config Read <a href="https://reprap.org/wiki/G-code#M920:_TMC_Config_Read">M920: TMC Config Read</a>
 *
 * Reads a value from the TMC driver’s register.
 *
 *#### Usage
 *
 *    M919 [ X | Y | Z | E | <TMC reg> ]
 *
 *#### Parameters
 *
 * - `X` - Read a value from X axis TMC driver's register
 * - `Y` - Read a value from Y axis TMC driver's register
 * - `Z` - Read a value from Z axis TMC driver's register
 * - `E` - Read a value from E axis TMC driver's register
 * - `<TMC reg>` - Select specific TMC driver's register
 */
void PrusaGcodeSuite::M920() {
    char cmd[16] = { 0 };
    tmc_reg_t *adress_reg;

    LOOP_XYZE(i) {

        if (parser.seen(axis_codes[i]) && parser.string_arg) {

            int read = sscanf(parser.string_arg, "%16s", cmd);

            if (read != 1) {
                SERIAL_ERROR_MSG("does not match '<axis> <TMC reg>'");
                return;
            }

            adress_reg = text_cmd_to_register(cmd, false, true);

            if (adress_reg == nullptr) {
                SERIAL_ERROR_MSG("Enter valid TMC reg name");
                return;
            }

            switch (i) {
            case X_AXIS:
                tmc_read_reg(stepperX, adress_reg->reg_adr);
                break;
            case Y_AXIS:
                tmc_read_reg(stepperY, adress_reg->reg_adr);
                break;
            case Z_AXIS:
                tmc_read_reg(stepperZ, adress_reg->reg_adr);
                break;
            case E_AXIS:
                tmc_read_reg(stepperE0, adress_reg->reg_adr);
                break;
            }
        }
    }
    queue.ok_to_send();
}

/** @}*/
