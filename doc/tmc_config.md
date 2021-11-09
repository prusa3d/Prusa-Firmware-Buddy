# How to config TMC stepper drivers?
---

- src/common/trinamic.cpp
-- slaveconf register

- lib/TMCStepper/src/source/TMC2209Stepper.cpp
-- Initializes listed registers. The configuration of those registers is in trinamic.cpp (section below).
-- gconf, ihold_irun, tpowerdown, chopconf, pwmconf register

- lib/Marlin/Marlin/src/module/stepper/trinamic.cpp
-- Configures the trinamics based on Configuration_PRINTER_adv.h file. Configuration of some registers is hardcoded.
-- gconf.pdn_disable, gconf.mstep_reg_select, gconf.i_scale_analog
-- chopconf.tbl, iholddelay, tpowerdown,
-- pwm_lim, pwm_reg, pwm_autograd, pwm_autoscale, pwm_freq, pwm_grad, pwm_ofs
-- gstat

- lib/Marlin/Marlin/src/feature/tmc_util.h
-- chopper

- include/marlin/Configuration_PRINTER_adv.h
-- motor current, microsteps, spreadcycle/stealthChop
-- interpolate, chopper_timing, stall_sensitivity
-- If you wanna add costume TMC init code put into braces: #define TMC_ADV() \
        {}
    this function is called after Marlin's trinamic initialization

- lib/Marlin/Marlin/src/feature/tmc_util.cpp
-- home tcoolthrs
