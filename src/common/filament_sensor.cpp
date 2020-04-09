// filament_sensor.c

//there is 10kOhm PU to 5V in filament sensor
//mcu PU/PD is in range 30 - 50 kOhm

//when PD is selected and sensor is connected Vmcu = min 3.75V .. (5V * 30kOhm) / (30 + 10 kOhm)
//pin is 5V tolerant

//mcu has 5pF, transistor D-S max 15pF
//max R is 50kOhm
//Max Tau ~= 20*10^-12 * 50*10^3 = 1*10^-6 s ... about 1us

#include "filament_sensor.h"
#include "hwio_pindef.h" //PIN_FSENSOR
#include "stm32f4xx_hal.h"
#include "gpio.h"
#include "eeprom.h"
#include "FreeRTOS.h"      //must apper before include task.h
#include "task.h"          //critical sections
#include "cmsis_os.h"      //osDelay
#include "marlin_client.h" //enable/disable fs in marlin

static volatile fsensor_t state = FS_NOT_INICIALIZED;
static volatile fsensor_t last_state = FS_NOT_INICIALIZED;

typedef enum {
    M600_on_edge = 0,
    M600_on_level = 1,
    M600_never = 2
} send_M600_on_t;

typedef struct {
    uint8_t M600_sent;
    uint8_t send_M600_on;
    uint8_t meas_cycle;
} status_t;
static status_t status = { 0, M600_on_edge, 0 };

/*---------------------------------------------------------------------------*/
//debug functions

extern "C" {

int fs_was_M600_send() {
    return status.M600_sent != 0;
}
char fs_get_send_M600_on() {
    switch (status.send_M600_on) {
    case M600_on_edge:
        return 'e';
    case M600_on_level:
        return 'l';
    case M600_never:
        return 'n';
    default:
        return 'x';
    }
}

/*---------------------------------------------------------------------------*/
//local functions
static void _init();

static int block_M600_injection = 0;
//called when Serial print screen is openned
//printer is not in sd printing mode, so filament sensor does not trigger M600
static void fsm_create_cb(ClinetFSM fsm, uint8_t data) {
    if (fsm == ClinetFSM::Serial_printing)
        block_M600_injection = 1;
}
static void fsm_destroy_cb(ClinetFSM fsm) {
    if (fsm == ClinetFSM::Serial_printing || fsm == ClinetFSM::Load_unload)
        block_M600_injection = 0;
}

//simple filter
//without filter fs_meas_cycle1 could set FS_NO_SENSOR (in case filament just runout)
static void _set_state(fsensor_t st) {
    taskENTER_CRITICAL();
    if (last_state == st)
        state = st;
    last_state = st;
    taskEXIT_CRITICAL();
}

static void _enable() {
    gpio_init(PIN_FSENSOR, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_VERY_HIGH); // pullup
    state = FS_NOT_INICIALIZED;
    last_state = FS_NOT_INICIALIZED;
    status.meas_cycle = 0;
}

static void _disable() {
    state = FS_DISABLED;
    last_state = FS_DISABLED;
    status.meas_cycle = 0;
}

/*---------------------------------------------------------------------------*/
//global thread safe functions
fsensor_t fs_get_state() {
    return state;
}

//value can change during read, but it is not a problem
int fs_did_filament_runout() {
    return state == FS_NO_FILAMENT;
}

void fs_send_M600_on_edge() {
    taskENTER_CRITICAL();
    status.send_M600_on = M600_on_edge;
    taskEXIT_CRITICAL();
}

void fs_send_M600_on_level() {
    taskENTER_CRITICAL();
    status.send_M600_on = M600_on_level;
    taskEXIT_CRITICAL();
}

void fs_send_M600_never() {
    taskENTER_CRITICAL();
    status.send_M600_on = M600_never;
    taskEXIT_CRITICAL();
}
/*---------------------------------------------------------------------------*/
//global thread safe functions
//but cannot be called from interrupt
void fs_enable() {
    taskENTER_CRITICAL();
    _enable();
    eeprom_set_var(EEVAR_FSENSOR_ENABLED, variant8_ui8(1));
    taskEXIT_CRITICAL();
}

void fs_disable() {
    taskENTER_CRITICAL();
    _disable();
    eeprom_set_var(EEVAR_FSENSOR_ENABLED, variant8_ui8(0));
    taskEXIT_CRITICAL();
}

uint8_t fs_get__send_M600_on__and_disable() {
    taskENTER_CRITICAL();
    uint8_t ret = status.send_M600_on;
    status.send_M600_on = M600_never;
    taskEXIT_CRITICAL();
    return ret;
}
void fs_restore__send_M600_on(uint8_t send_M600_on) {
    taskENTER_CRITICAL();
    //cannot call _init(); - it could cause stacking in unincialized state
    status.send_M600_on = send_M600_on;
    taskEXIT_CRITICAL();
}

fsensor_t fs_wait_inicialized() {
    fsensor_t ret = fs_get_state();
    while (ret == FS_NOT_INICIALIZED) {
        osDelay(0); // switch to other threads
        ret = fs_get_state();
    }
    return ret;
}

void fs_clr_sent() {
    taskENTER_CRITICAL();
    status.M600_sent = 0;
    taskEXIT_CRITICAL();
}

/*---------------------------------------------------------------------------*/
//global not thread safe functions
static void _init() {
    int enabled = eeprom_get_var(EEVAR_FSENSOR_ENABLED).ui8 ? 1 : 0;
    marlin_client_set_fsm_create_cb(fsm_create_cb);
    marlin_client_set_fsm_destroy_cb(fsm_destroy_cb);
    if (enabled)
        _enable();
    else
        _disable();
}

void fs_init_on_edge() {
    _init();
    fs_send_M600_on_edge();
}
void fs_init_on_level() {
    _init();
    fs_send_M600_on_level();
}
void fs_init_never() {
    _init();
    fs_send_M600_never();
}

/*---------------------------------------------------------------------------*/
//methods called only in fs_cycle
static void _injectM600() {
    marlin_vars_t *vars = marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_SD_PRINT));
    if (status.M600_sent == 0 && (vars->sd_printing || block_M600_injection)) {
        marlin_gcode_push_front("M600"); //change filament
        status.M600_sent = 1;
    }
}

static void _cycle0() {
    if (gpio_get(PIN_FSENSOR) == 1) {
        gpio_init(PIN_FSENSOR, GPIO_MODE_INPUT, GPIO_PULLDOWN, GPIO_SPEED_FREQ_VERY_HIGH); // pulldown
        status.meas_cycle = 1;                                                             //next cycle shall be 1
    } else {
        int had_filament = state == FS_HAS_FILAMENT ? 1 : 0;
        _set_state(FS_NO_FILAMENT); //it is filtered, 2 requests are needed to change state
        //M600_on_edge == inject after state was changed from FS_HAS_FILAMENT to FS_NO_FILAMENT
        //M600_on_level == inject on FS_NO_FILAMENT
        //M600_never == do not inject
        if (state == FS_NO_FILAMENT) {
            switch (status.send_M600_on) {
            case M600_on_edge:
                if (!had_filament)
                    break;
                // no break if had_filament == 1
            case M600_on_level:
                _injectM600();
                break;
            case M600_never:
            default:
                break;
            }
        }

        status.meas_cycle = 0; //remain in cycle 0
    }
}

//called only in fs_cycle
static void _cycle1() {
    //pulldown was set in cycle 0
    _set_state(gpio_get(PIN_FSENSOR) == 1 ? FS_HAS_FILAMENT : FS_NOT_CONNECTED);
    gpio_init(PIN_FSENSOR, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_VERY_HIGH); // pullup
    status.meas_cycle = 0;                                                           //next cycle shall be 0
}

//dealay between calls must be 1us or longer
void fs_cycle() {
    //sensor is disabled (only init can enable it)
    if (state == FS_DISABLED)
        return;

    //sensor is enabled
    if (status.meas_cycle == 0) {
        _cycle0();
    } else {
        _cycle1();
    }
}

} //extern "C"
