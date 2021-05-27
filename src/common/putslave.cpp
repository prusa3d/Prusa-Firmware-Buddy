// putslave.c
#include "putslave.h"
#include <string.h>
#include "dbg.h"
#include "version.h"
#include "gpio.h"
#include "hwio.h"
#include "sys.h"
#include "diag.h"
#include "app.h"
#include "marlin_server.h"
#include "otp.h"
#ifdef BUDDY_ENABLE_ETHERNET
    #include "lwip.h"
#endif //BUDDY_ENABLE_ETHERNET
#include "eeprom.h"
#include "cmsis_os.h"
#include "uartslave.h"
#include "hwio_pindef.h"
#include "trinamic.h"
#include "main.h"
#include "fanctl.h"
#include "dump.h"
#include "adc.h"
#include "../Marlin/src/module/temperature.h"
#include "../Marlin/src/Marlin.h"

#ifndef HAS_GUI
    #error "HAS_GUI not defined."
#endif

using namespace buddy::hw;

static int putslave_do_cmd_a_stop(uartslave_t *pslave);

int putslave_parse_cmd_id(uartslave_t *pslave, char *pstr, uint16_t *pcmd_id) {
    int ret;
    uint16_t cmd_id = UARTSLAVE_CMD_ID_UNK;
    if (pstr[0] == 0) {
        *pcmd_id = UARTSLAVE_CMD_ID_0;
        return 0;
    } else if ((pstr[3] == 0) || (pstr[3] == ' ')) {
        ret = 3;
        if (strncmp(pstr, "rst", 3) == 0)
            cmd_id = PUTSLAVE_CMD_ID_RST;
        else if (strncmp(pstr, "ver", 3) == 0)
            cmd_id = PUTSLAVE_CMD_ID_VER;
        else if (strncmp(pstr, "ser", 3) == 0)
            cmd_id = PUTSLAVE_CMD_ID_SER;
        else if (strncmp(pstr, "mac", 3) == 0)
            cmd_id = PUTSLAVE_CMD_ID_MAC;
        else if (strncmp(pstr, "uid", 3) == 0)
            cmd_id = PUTSLAVE_CMD_ID_UID;
        else if (strncmp(pstr, "ip4", 3) == 0)
            cmd_id = PUTSLAVE_CMD_ID_IP4;
        else if (strncmp(pstr, "tst", 3) == 0)
            cmd_id = PUTSLAVE_CMD_ID_TST;
        else if (strncmp(pstr, "adc", 3) == 0)
            cmd_id = PUTSLAVE_CMD_ID_ADC;
        else if (strncmp(pstr, "pwm", 3) == 0)
            cmd_id = PUTSLAVE_CMD_ID_PWM;
        else if (strncmp(pstr, "i2c", 3) == 0)
            cmd_id = PUTSLAVE_CMD_ID_I2C;
        else if (strncmp(pstr, "ten", 3) == 0)
            cmd_id = PUTSLAVE_CMD_ID_TEN;
        else if (strncmp(pstr, "tdg", 3) == 0)
            cmd_id = PUTSLAVE_CMD_ID_TDG;
    } else if ((pstr[4] == 0) || (pstr[4] == ' ')) {
        ret = 4;
        if (strncmp(pstr, "brev", 4) == 0)
            cmd_id = PUTSLAVE_CMD_ID_BREV;
        else if (strncmp(pstr, "btim", 4) == 0)
            cmd_id = PUTSLAVE_CMD_ID_BTIM;
        else if (strncmp(pstr, "lock", 4) == 0)
            cmd_id = PUTSLAVE_CMD_ID_LOCK;
        else if (strncmp(pstr, "tone", 4) == 0)
            cmd_id = PUTSLAVE_CMD_ID_TONE;
        else if (strncmp(pstr, "gpio", 4) == 0)
            cmd_id = PUTSLAVE_CMD_ID_GPIO;
        else if (strncmp(pstr, "stop", 4) == 0)
            cmd_id = PUTSLAVE_CMD_ID_STOP;
        else if (strncmp(pstr, "tste", 4) == 0)
            cmd_id = PUTSLAVE_CMD_ID_TSTE;
        else if (strncmp(pstr, "eecl", 4) == 0)
            cmd_id = PUTSLAVE_CMD_ID_EECL;
        else if (strncmp(pstr, "fpwm", 4) == 0)
            cmd_id = PUTSLAVE_CMD_ID_FPWM;
        else if (strncmp(pstr, "frpm", 4) == 0)
            cmd_id = PUTSLAVE_CMD_ID_FRPM;
        else if (strncmp(pstr, "fpsm", 4) == 0)
            cmd_id = PUTSLAVE_CMD_ID_FPSM;
        else if (strncmp(pstr, "gpcf", 4) == 0)
            cmd_id = PUTSLAVE_CMD_ID_GPCF;
        else if (strncmp(pstr, "doer", 4) == 0)
            cmd_id = PUTSLAVE_CMD_ID_DOER;
        else if (strncmp(pstr, "diag", 4) == 0)
            cmd_id = PUTSLAVE_CMD_ID_DIAG;
        else if (strncmp(pstr, "uart", 4) == 0)
            cmd_id = PUTSLAVE_CMD_ID_UART;
        else if (strncmp(pstr, "move", 4) == 0)
            cmd_id = PUTSLAVE_CMD_ID_MOVE;
        else if (strncmp(pstr, "gpup", 4) == 0)
            cmd_id = PUTSLAVE_CMD_ID_GPUP;
    } else if ((pstr[5] == 0) || (pstr[5] == ' ')) {
        ret = 5;
        if (strncmp(pstr, "gcode", 5) == 0)
            cmd_id = PUTSLAVE_CMD_ID_GCODE;
        else if (strncmp(pstr, "start", 5) == 0)
            cmd_id = PUTSLAVE_CMD_ID_START;
        else if (strncmp(pstr, "inval", 5) == 0)
            cmd_id = PUTSLAVE_CMD_ID_INVAL;
        else if (strncmp(pstr, "eedef", 5) == 0)
            cmd_id = PUTSLAVE_CMD_ID_EEDEF;
    }
    if (cmd_id != UARTSLAVE_CMD_ID_UNK) {
        *pcmd_id = cmd_id;
        return ret;
    }
    return UARTSLAVE_ERR_SYN;
}

int putslave_do_cmd_q_ver(uartslave_t *pslave) {
    uartslave_printf(pslave, "%s-%s ", project_firmware_name, project_version_full);
    return UARTSLAVE_OK;
}

int putslave_do_cmd_q_ser(uartslave_t *pslave) {
    char *ptr = (char *)OTP_SERIAL_NUMBER_ADDR;
    uartslave_printf(pslave, "CZPX%.15s ", ptr);
    return UARTSLAVE_OK;
}

int putslave_do_cmd_q_brev(uartslave_t *pslave) {
    uint8_t *ptr = (uint8_t *)OTP_BOARD_REVISION_ADDR;
    uartslave_printf(pslave, "%d.%d.%d ", ptr[0], ptr[1], ptr[2]);
    return UARTSLAVE_OK;
}

int putslave_do_cmd_q_btim(uartslave_t *pslave) {
    uint32_t *pui32 = (uint32_t *)OTP_BOARD_TIME_STAMP_ADDR;
    uartslave_printf(pslave, "%lu ", pui32[0]);
    return UARTSLAVE_OK;
}

int putslave_do_cmd_q_mac(uartslave_t *pslave) {
    uint8_t *ptr = (uint8_t *)OTP_MAC_ADDRESS_ADDR;
    uartslave_printf(pslave, "%02X:%02X:%02X:%02X:%02X:%02X ",
        ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
    return UARTSLAVE_OK;
}

int putslave_do_cmd_q_uid(uartslave_t *pslave) {
    uint32_t *ptr = (uint32_t *)OTP_STM32_UUID_ADDR;
    uartslave_printf(pslave, "%08lx-%08lx-%08lx ", ptr[0], ptr[1], ptr[2]);
    return UARTSLAVE_OK;
}

int putslave_do_cmd_q_ip4(uartslave_t *pslave) {
#ifdef BUDDY_ENABLE_ETHERNET
    uint8_t *ptr = (uint8_t *)(&netif_default->ip_addr);
    uartslave_printf(pslave, "%u.%u.%u.%u ", ptr[0], ptr[1], ptr[2], ptr[3]);
    return UARTSLAVE_OK;
#else
    return UARTSLAVE_ERR_ONP;
#endif //BUDDY_ENABLE_ETHERNET
}

int putslave_do_cmd_q_lock(uartslave_t *pslave) {
    uint8_t *ptr = (uint8_t *)OTP_LOCK_BLOCK_ADDR;
    uartslave_printf(pslave, "%d ", (ptr[0] == 0) ? 1 : 0);
    return UARTSLAVE_OK;
}

int putslave_do_cmd_q_fpwm(uartslave_t *pslave, char *pstr) {
    int fan = 0;
    if (sscanf(pstr, "%d", &fan) != 1)
        return UARTSLAVE_ERR_SYN;
    if ((fan < 0) || (fan > 1))
        return UARTSLAVE_ERR_OOR;
    uartslave_printf(pslave, "%d ", fanctl_get_pwm(fan));
    return UARTSLAVE_OK;
}

int putslave_do_cmd_q_frpm(uartslave_t *pslave, char *pstr) {
    int fan = 0;
    if (sscanf(pstr, "%d", &fan) != 1)
        return UARTSLAVE_ERR_SYN;
    if ((fan < 0) || (fan > 1))
        return UARTSLAVE_ERR_OOR;
    uartslave_printf(pslave, "%d ", fanctl_get_rpm(fan));
    return UARTSLAVE_OK;
}

int putslave_do_cmd_q_fpsm(uartslave_t *pslave, char *pstr) {
    int fan = 0;
    if (sscanf(pstr, "%d", &fan) != 1)
        return UARTSLAVE_ERR_SYN;
    if ((fan < 0) || (fan > 1))
        return UARTSLAVE_ERR_OOR;
    uartslave_printf(pslave, "%d ", fanctl_get_psm(fan));
    return UARTSLAVE_OK;
}

int putslave_do_cmd_q_adc(uartslave_t *pslave, char *pstr) {
    int adc = 0;
    if (sscanf(pstr, "%d", &adc) != 1)
        return UARTSLAVE_ERR_SYN;
    if ((adc < 0) || (adc > 4))
        return UARTSLAVE_ERR_OOR;
    uartslave_printf(pslave, "%d ", hwio_adc_get_val(static_cast<ADC_t>(adc)));
    return UARTSLAVE_OK;
}

int putslave_do_cmd_q_gpio(uartslave_t *pslave, char *pstr) {
    int gpio = 0;
    if (sscanf(pstr, "%d", &gpio) != 1)
        return UARTSLAVE_ERR_SYN;
    if ((gpio < MARLIN_PORT_PIN(MARLIN_PORT_A, MARLIN_PIN_NR_0)) || (gpio > MARLIN_PORT_PIN(MARLIN_PORT_E, MARLIN_PIN_NR_15)))
        return UARTSLAVE_ERR_OOR;
    uartslave_printf(pslave, "%d ", gpio_get(gpio));
    return UARTSLAVE_OK;
}

int putslave_do_cmd_q_pwm(uartslave_t *pslave, char *pstr) {
    int pwm = 0;
    if (sscanf(pstr, "%d", &pwm) != 1)
        return UARTSLAVE_ERR_SYN;
    if ((pwm < 0) || (pwm > 3))
        return UARTSLAVE_ERR_OOR;
    uartslave_printf(pslave, "%d ", hwio_pwm_get_max(pwm));
    return UARTSLAVE_OK;
}
int putslave_do_cmd_q_tste(uartslave_t *pslave, char *pstr) {

    unsigned int bytes;
    int8_t res_flag;
    if (sscanf(pstr, "%u", &bytes) != 1 || bytes < 16) {
        res_flag = eeprom_test_PUT(2048);
    } else {
        res_flag = eeprom_test_PUT(bytes);
    }
    if (res_flag)
        return UARTSLAVE_OK;
    else
        return UARTSLAVE_ERR_UNK;
}

int putslave_do_cmd_q_diag(uartslave_t *pslave, char *pstr) {

    diag_test();
    uartslave_printf(pslave, "%d ", diag_error);
    return UARTSLAVE_OK;
}

int putslave_do_cmd_q_gpup(uartslave_t *pslave, char *pstr) {
    int gpio = 0;
    if (sscanf(pstr, "%d", &gpio) != 1)
        return UARTSLAVE_ERR_SYN;
    if ((gpio < MARLIN_PORT_PIN(MARLIN_PORT_A, MARLIN_PIN_NR_0)) || (gpio > MARLIN_PORT_PIN(MARLIN_PORT_E, MARLIN_PIN_NR_15)))
        return UARTSLAVE_ERR_OOR;
    gpio_init(gpio, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_LOW);
    uartslave_printf(pslave, "%d ", gpio_get(gpio));
    return UARTSLAVE_OK;
}

int putslave_do_cmd_q_uart(uartslave_t *pslave) {
    uint8_t uart1rx_data[32] = { 0 };
    uint8_t data_out[2] = "i";
    HAL_UART_Transmit(pslave->huart, (uint8_t *)data_out, sizeof(data_out), HAL_MAX_DELAY);
    if (uart1rx_data[0] == data_out[0])
        return UARTSLAVE_OK;
    uart1rx_data[0] = 0;
    return UARTSLAVE_ERR_ONP;
}

#ifndef INIT_TRINAMIC_FROM_MARLIN_ONLY
    #error "INIT_TRINAMIC_FROM_MARLIN_ONLY not defined"
#endif
#if !INIT_TRINAMIC_FROM_MARLIN_ONLY
int putslave_do_cmd_q_tdg(uartslave_t *pslave) {
    tmc_set_sgthrs(255);
    int tmc_stg = 0;
    tmc_stg = tmc_get_diag();
    if (tmc_stg == 15)
        return UARTSLAVE_OK;
    uartslave_printf(pslave, "%d ", tmc_stg);
    return UARTSLAVE_ERR_ONP;
}
#endif //!INIT_TRINAMIC_FROM_MARLIN_ONLY

int putslave_do_cmd_a_rst(uartslave_t *pslave) {
    sys_reset();
    return UARTSLAVE_OK;
}

int _validate_serial(const char *str) {
    unsigned int w2 = 0; // week of year (1-52)
    unsigned int y2 = 0; // year since 2000 (19-25)
    unsigned int p3 = 0; // product - 017 for MINI
    char t1 = 0;         // type - 'K' or 'C' (kit or complete)
    unsigned int n5 = 0; // number - 0..99999
    if (sscanf(str, "%2u%2uX%3uX%c%5u", &w2, &y2, &p3, &t1, &n5) != 5)
        return 0;
    if (w2 > 52)
        return 0;
    if ((y2 < 19) || (y2 > 25))
        return 0;
    if (p3 != 17)
        return 0;
    if ((t1 != 'K') && (t1 != 'C'))
        return 0;
    if (n5 > 99999)
        return 0;
    return 1;
}

int putslave_do_cmd_a_ser(uartslave_t *pslave, char *pstr) {
    char *ptr = (char *)OTP_SERIAL_NUMBER_ADDR;
    char ser[OTP_SERIAL_NUMBER_SIZE];
    if (sys_flash_is_empty(ptr, OTP_SERIAL_NUMBER_SIZE)) {
        memset(ser, 0, OTP_SERIAL_NUMBER_SIZE);
        if (sscanf(pstr, " CZPX%15s", ser) != 1)
            return UARTSLAVE_ERR_SYN;
        if (strlen(ser) != 15)
            return UARTSLAVE_ERR_OOR;
        if (!_validate_serial(ser))
            return UARTSLAVE_ERR_OOR;
        if (sys_flash_write(ptr, ser, OTP_SERIAL_NUMBER_SIZE) != OTP_SERIAL_NUMBER_SIZE)
            return UARTSLAVE_ERR_UNK;
        return UARTSLAVE_OK;
    }
    return UARTSLAVE_ERR_ONP;
}

int putslave_do_cmd_a_brev(uartslave_t *pslave, char *pstr) {
    uint8_t *ptr = (uint8_t *)OTP_BOARD_REVISION_ADDR; // OTP memory - board revision
    uint8_t rev[OTP_BOARD_REVISION_SIZE];
    if (sys_flash_is_empty(ptr, OTP_BOARD_REVISION_SIZE)) {
        if (sscanf(pstr, "%hhu.%hhu.%hhu", rev + 0, rev + 1, rev + 2) != 3)
            return UARTSLAVE_ERR_SYN;
        //TODO: parameter range check
        if (sys_flash_write(ptr, rev, OTP_BOARD_REVISION_SIZE) != OTP_BOARD_REVISION_SIZE)
            return UARTSLAVE_ERR_UNK;
        return UARTSLAVE_OK;
    }
    return UARTSLAVE_ERR_ONP;
}

int putslave_do_cmd_a_btim(uartslave_t *pslave, char *pstr) {
    uint32_t *ptr = (uint32_t *)OTP_BOARD_TIME_STAMP_ADDR; // OTP memory - time stamp
    uint32_t time_stamp;
    if (sys_flash_is_empty(ptr, OTP_BOARD_TIME_STAMP_SIZE)) {
        if (sscanf(pstr, "%lu", &time_stamp) != 1)
            return UARTSLAVE_ERR_SYN;
        //TODO: parameter range check
        if (sys_flash_write(ptr, &time_stamp, OTP_BOARD_TIME_STAMP_SIZE) != OTP_BOARD_TIME_STAMP_SIZE)
            return UARTSLAVE_ERR_UNK;
        return UARTSLAVE_OK;
    }
    return UARTSLAVE_ERR_ONP;
}

int putslave_do_cmd_a_mac(uartslave_t *pslave, char *pstr) {
    uint32_t *ptr = (uint32_t *)OTP_MAC_ADDRESS_ADDR; // OTP memory - mac address
    uint8_t mac[OTP_MAC_ADDRESS_SIZE];
    if (sys_flash_is_empty(ptr, OTP_MAC_ADDRESS_SIZE)) {
        if (sscanf(pstr, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
                mac + 0, mac + 1, mac + 2, mac + 3, mac + 4, mac + 5)
                != 6
            || sscanf(pstr, "%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX",
                   mac + 0, mac + 1, mac + 2, mac + 3, mac + 4, mac + 5)
                != 6)
            return UARTSLAVE_ERR_SYN;
        //TODO: parameter range check
        if (sys_flash_write(ptr, mac, OTP_MAC_ADDRESS_SIZE) != OTP_MAC_ADDRESS_SIZE)
            return UARTSLAVE_ERR_UNK;
        return UARTSLAVE_OK;
    }
    return UARTSLAVE_ERR_ONP;
}

int putslave_do_cmd_a_tone(uartslave_t *pslave, char *pstr) {
    float frq = 0.0F;
    unsigned int del = 0;
    float vol = 1.0F;
    int s = sscanf(pstr, "%f %u %f", &frq, &del, &vol);
    if (s < 2)
        return UARTSLAVE_ERR_SYN;
    if (s == 3)
        hwio_beeper_set_vol(vol);
    hwio_beeper_tone(frq, del);
    return UARTSLAVE_OK;
}

extern osThreadId defaultTaskHandle;
extern osThreadId displayTaskHandle;
extern osThreadId idleTaskHandle;

extern osThreadId webServerTaskHandle;

int put_setup_done = 0;

#if HAS_GUI
int putslave_do_cmd_a_start(uartslave_t *pslave) {
    if (!marlin_server_processing()) {
        NVIC_EnableIRQ(TIM7_IRQn);
        HAL_SPI_MspInit(&hspi2);
        marlin_server_start_processing();
        osThreadResume(displayTaskHandle);
    #ifdef BUDDY_ENABLE_ETHERNET
        osThreadResume(webServerTaskHandle);
    #endif //BUDDY_ENABLE_ETHERNET
        if (diag_fastboot && !put_setup_done) {
            app_setup();
            put_setup_done = 1;
        }
    }
    return UARTSLAVE_OK;
}
#endif

int putslave_do_cmd_a_eecl(uartslave_t *pslave) {
    eeprom_clear();
    return UARTSLAVE_OK;
}

int putslave_do_cmd_a_eedef(uartslave_t *pslave) {
    eeprom_defaults();
    return UARTSLAVE_OK;
}

int putslave_do_cmd_a_gpio(uartslave_t *pslave, char *pstr) {
    int gpio = 0;
    int state = 0;
    int n = 0;
    if (sscanf(pstr, "%d%n", &gpio, &n) != 1)
        return UARTSLAVE_ERR_SYN;
    if ((gpio < MARLIN_PORT_PIN(MARLIN_PORT_A, MARLIN_PIN_NR_0)) || (gpio > MARLIN_PORT_PIN(MARLIN_PORT_E, MARLIN_PIN_NR_15)))
        return UARTSLAVE_ERR_OOR;
    pstr += n;
    if (sscanf(pstr, "%d%n", &state, &n) != 1)
        return UARTSLAVE_ERR_SYN;
    if ((state < 0) || (state > 1))
        return UARTSLAVE_ERR_OOR;
    if (gpio == MARLIN_PORT_PIN(MARLIN_PORT_A, MARLIN_PIN_NR_0)) {
        if (state)
            hwio_beeper_set_pwm(255, 255); // 1
        else
            hwio_beeper_set_pwm(255, 0); // 0
    } else
        gpio_set(gpio, state);
    return UARTSLAVE_OK;
}

int putslave_do_cmd_a_gpcf(uartslave_t *pslave, char *pstr) {
    int gpio = 0;
    int mode = 0;
    int pull = 0;
    int n = 0;
    if (sscanf(pstr, "%d%n", &gpio, &n) != 1)
        return UARTSLAVE_ERR_SYN;
    if ((gpio < MARLIN_PORT_PIN(MARLIN_PORT_A, MARLIN_PIN_NR_0)) || (gpio > MARLIN_PORT_PIN(MARLIN_PORT_E, MARLIN_PIN_NR_15)))
        return UARTSLAVE_ERR_OOR;
    pstr += n;
    if (sscanf(pstr, "%d%n", &mode, &n) != 1)
        return UARTSLAVE_ERR_SYN;
    if ((mode < 0) || (mode > 1))
        return UARTSLAVE_ERR_OOR;
    pstr += n;
    if (sscanf(pstr, "%d%n", &pull, &n) != 1)
        return UARTSLAVE_ERR_SYN;
    if ((pull < 0) || (pull > 1))
        return UARTSLAVE_ERR_OOR;
    gpio_init(gpio, mode ? GPIO_MODE_OUTPUT_PP : GPIO_MODE_INPUT, pull ? GPIO_PULLUP : GPIO_NOPULL, GPIO_SPEED_FREQ_LOW);
    return UARTSLAVE_OK;
}

// externs from adc module used in putslave_do_thermal_error
extern int _adc_val[];
extern uint8_t adc_sta;

void putslave_do_thermal_error(uint32_t adcval, uint8_t adcchn, uint8_t heater) {
    adc_sta = 0xff;            // disable adc sampling
    _adc_val[adcchn] = adcval; // set fake "sampled" adc value
    uint32_t tick = HAL_GetTick();
    while ((HAL_GetTick() - tick) < 500) // wait 500ms (update values inside thermal manager)
        idle();
    // if not currently heating, start heating
    if ((heater == 0) && thermalManager.degTargetHotend(0) == 0)
        thermalManager.setTargetHotend(300, 0);
    if ((heater == 0xff) && thermalManager.degTargetBed() == 0)
        thermalManager.setTargetBed(100);
}

// stackoverflow test (recursion)
void putslave_do_stackoverflow(uint32_t wait, uint32_t cycles, uint32_t cnt) {
    if (cnt > 0)
        putslave_do_stackoverflow(wait, cycles, cnt - 1);
    if (wait)
        osDelay(wait);
    putslave_do_stackoverflow(wait, cycles, cycles);
}

int putslave_do_cmd_a_doer(uartslave_t *pslave, char *pstr) {
    int err = 0;
    int err2 = 0;
    int n = 0;
    if (sscanf(pstr, "%d%n", &err, &n) != 1)
        return UARTSLAVE_ERR_SYN;
    pstr += n;
    sscanf(pstr, "%d%n", &err2, &n);
    switch (err) {
    case 1: // watchdog reset
        while (1)
            ; // endless loop
        break;
    case 2:                      // hardfault
        dump_hardfault_test_0(); //imprecise hardfault
        // TODO: add some next hardfault tests
        break;
    case 3: // thermal error
        switch (err2) {
        case 0: // max-temp nozzle
            putslave_do_thermal_error(0, 4, 0);
            break;
        case 1: // min-temp nozzle
            putslave_do_thermal_error(1023, 4, 0);
            break;
        case 2: // thermal-runaway nozzle
            putslave_do_thermal_error(977, 4, 0);
            break;
        case 10: // max-temp bed
            putslave_do_thermal_error(0, 1, 0xff);
            break;
        case 11: // min-temp bed
            putslave_do_thermal_error(1023, 1, 0xff);
            break;
        case 12: // thermal-runaway bed
            putslave_do_thermal_error(966, 1, 0xff);
            break;
        }
        break;
    case 4: // stack overflow
        switch (err2) {
        case 0: // "soft" variant - slow recursion
            putslave_do_stackoverflow(10, 100, 100);
            break;
        case 1: // "hard" variant - fast recursion (overwrite almost entire RAM)
            putslave_do_stackoverflow(0, 0xffffffff, 0xffffffff);
            break;
        }
    }
    return UARTSLAVE_OK;
}

int putslave_do_cmd_a_gcode(uartslave_t *pslave, char *pstr) {
    char gcode[32];
    int n = 0;
    if (sscanf(pstr + 1, "%31[^\n]%n", gcode, &n) != 1)
        return UARTSLAVE_ERR_SYN;
    marlin_server_enqueue_gcode(gcode);
    return UARTSLAVE_OK;
}

int putslave_do_cmd_a_pwm(uartslave_t *pslave, char *pstr) {
    int pwm = 0;
    int value = 0;
    int n = 0;
    if (sscanf(pstr, "%d%n", &pwm, &n) != 1)
        return UARTSLAVE_ERR_SYN;
    if ((pwm < 0) || (pwm > 3))
        return UARTSLAVE_ERR_OOR;
    pstr += n;
    if (sscanf(pstr, "%d%n", &value, &n) != 1)
        return UARTSLAVE_ERR_SYN;
    if ((value < 0) || (value > hwio_pwm_get_max(pwm)))
        return UARTSLAVE_ERR_OOR;
    hwio_pwm_set_val(pwm, value);
    if (pwm == 2) {
        fanctl1.setPWM(value);
    } else if (pwm == 3) {
        fanctl0.setPWM(value);
    }
    return UARTSLAVE_OK;
}

int putslave_do_cmd_a_lock(uartslave_t *pslave) {
    uint8_t zero = 0;
    if (sys_flash_is_empty((void *)OTP_LOCK_BLOCK_ADDR, 1)) {
        if (sys_flash_write((void *)OTP_LOCK_BLOCK_ADDR, &zero, 1) != 1)
            return UARTSLAVE_ERR_UNK;
        return UARTSLAVE_OK;
    }
    return UARTSLAVE_ERR_ONP;
}

int putslave_do_cmd_a_fpwm(uartslave_t *pslave, char *pstr) {
    unsigned int fan = 0;
    unsigned int pwm = 0;
    if (strlen(pstr)) {
        if (sscanf(pstr, "%u %u", &fan, &pwm) != 2)
            return UARTSLAVE_ERR_SYN;
        if (fan > 1)
            return UARTSLAVE_ERR_OOR;
        if (pwm > 50)
            return UARTSLAVE_ERR_OOR;
        fanctl_set_pwm(fan, pwm);
        return UARTSLAVE_OK;
    }
    return UARTSLAVE_ERR_ONP;
}

int putslave_do_cmd_a_fpsm(uartslave_t *pslave, char *pstr) {
    unsigned int fan = 0;
    unsigned int psm = 0;
    if (strlen(pstr)) {
        if (sscanf(pstr, "%u %u", &fan, &psm) != 2)
            return UARTSLAVE_ERR_SYN;
        if (fan > 1)
            return UARTSLAVE_ERR_OOR;
        if (psm > 2)
            return UARTSLAVE_ERR_OOR;
        fanctl_set_psm(fan, psm);
        return UARTSLAVE_OK;
    }
    return UARTSLAVE_ERR_ONP;
}

int putslave_do_cmd_a_tst(uartslave_t *pslave, char *pstr) {
#if 0 // used to test eeprom wizard flags
    int run_selftest = 0;
    int run_xyzcalib = 0;
    int run_firstlay = 0;
    if (strlen(pstr))
        if (sscanf(pstr, "%d %d %d", &run_selftest, &run_xyzcalib, &run_firstlay) <= 0)
            return UARTSLAVE_ERR_SYN;
    eeprom_set_var(EEVAR_RUN_SELFTEST, variant8_ui8(run_selftest)); //
    eeprom_set_var(EEVAR_RUN_XYZCALIB, variant8_ui8(run_xyzcalib)); //
    eeprom_set_var(EEVAR_RUN_FIRSTLAY, variant8_ui8(run_firstlay)); //
#endif
    return UARTSLAVE_OK;
}

int putslave_do_cmd_a_inval(uartslave_t *pslave) {
    if (sys_fw_is_valid()) {
        if (sys_fw_invalidate())
            return UARTSLAVE_OK;
        return UARTSLAVE_ERR_UNK;
    }
// for testing - format last flash sector in case of invalid firmware
// not working in Debug_Boot because firmware is too big
#if 0
	else
	{
		if (sys_fw_validate())
			return UARTSLAVE_OK;
		return UARTSLAVE_ERR_UNK;
	}
#endif
    return UARTSLAVE_ERR_ONP;
}

extern I2C_HandleTypeDef hi2c1;

int putslave_do_cmd_a_i2c(uartslave_t *pslave) {
    uint8_t ret;
    uint8_t _out[3] = { 0 >> 8, 0 & 0xff, 0x56 };
    ret = HAL_I2C_Master_Transmit(&hi2c1, 0x02, _out, 3, HAL_MAX_DELAY);
    if (ret == 0)
        return UARTSLAVE_OK;
    return UARTSLAVE_ERR_ONP;
}

#if !INIT_TRINAMIC_FROM_MARLIN_ONLY
int putslave_do_cmd_a_ten(uartslave_t *pslave, char *pstr) {
    int state;
    if (sscanf(pstr, "%d", &state) != 1)
        return UARTSLAVE_ERR_SYN;
    const Pin::State pinState = static_cast<Pin::State>(state);
    if ((pinState != Pin::State::low) && (pinState != Pin::State::high))
        return UARTSLAVE_ERR_OOR;
    tmc_set_mres();
    xEnable.write(pinState);
    yEnable.write(pinState);
    zEnable.write(pinState);
    e0Enable.write(pinState);
    return UARTSLAVE_OK;
}

int putslave_do_cmd_a_move(uartslave_t *pslave, char *pstr) {
    int stepper = 0;
    int dir = 0;
    int speed = 0;
    int steps = 0;
    int n = 0;

    if (sscanf(pstr, "%d%n", &stepper, &n) != 1)
        return UARTSLAVE_ERR_SYN;
    if ((stepper < 0) || (stepper > 15))
        return UARTSLAVE_ERR_OOR;
    pstr += n;
    if (sscanf(pstr, "%d%n", &steps, &n) != 1)
        return UARTSLAVE_ERR_SYN;
    if ((steps < 0) || (steps > 10000))
        return UARTSLAVE_ERR_OOR;
    pstr += n;
    if (sscanf(pstr, "%d%n", &dir, &n) != 1)
        return UARTSLAVE_ERR_SYN;
    if ((dir < 0) || (dir > 10))
        return UARTSLAVE_ERR_OOR;
    pstr += n;
    if (sscanf(pstr, "%d%n", &speed, &n) != 1)
        return UARTSLAVE_ERR_SYN;
    if ((speed < 0) || (speed > 10))
        return UARTSLAVE_ERR_OOR;

    tmc_set_move(stepper, steps, dir, speed);
    return UARTSLAVE_OK;
}
#endif //!INIT_TRINAMIC_FROM_MARLIN_ONLY
int putslave_do_cmd(uartslave_t *pslave, uint16_t mod_msk, char cmd, uint16_t cmd_id, char *pstr) {
    if (cmd == '?') {
        if (mod_msk == 0)
            switch (cmd_id) {
            case PUTSLAVE_CMD_ID_VER:
                return putslave_do_cmd_q_ver(pslave);
            case PUTSLAVE_CMD_ID_SER:
                return putslave_do_cmd_q_ser(pslave);
            case PUTSLAVE_CMD_ID_BREV:
                return putslave_do_cmd_q_brev(pslave);
            case PUTSLAVE_CMD_ID_BTIM:
                return putslave_do_cmd_q_btim(pslave);
            case PUTSLAVE_CMD_ID_MAC:
                return putslave_do_cmd_q_mac(pslave);
            case PUTSLAVE_CMD_ID_UID:
                return putslave_do_cmd_q_uid(pslave);
            case PUTSLAVE_CMD_ID_IP4:
                return putslave_do_cmd_q_ip4(pslave);
            case PUTSLAVE_CMD_ID_LOCK:
                return putslave_do_cmd_q_lock(pslave);
            case PUTSLAVE_CMD_ID_FPWM:
                return putslave_do_cmd_q_fpwm(pslave, pstr);
            case PUTSLAVE_CMD_ID_FRPM:
                return putslave_do_cmd_q_frpm(pslave, pstr);
            case PUTSLAVE_CMD_ID_FPSM:
                return putslave_do_cmd_q_fpsm(pslave, pstr);
            case PUTSLAVE_CMD_ID_ADC:
                return putslave_do_cmd_q_adc(pslave, pstr);
            case PUTSLAVE_CMD_ID_GPIO:
                return putslave_do_cmd_q_gpio(pslave, pstr);
            case PUTSLAVE_CMD_ID_PWM:
                return putslave_do_cmd_q_pwm(pslave, pstr);
            case PUTSLAVE_CMD_ID_TSTE:
                return putslave_do_cmd_q_tste(pslave, pstr);
            case PUTSLAVE_CMD_ID_DIAG:
                return putslave_do_cmd_q_diag(pslave, pstr);
            case PUTSLAVE_CMD_ID_UART:
                return putslave_do_cmd_q_uart(pslave);
#if !INIT_TRINAMIC_FROM_MARLIN_ONLY
            case PUTSLAVE_CMD_ID_TDG:
                return putslave_do_cmd_q_tdg(pslave);
#endif
            case PUTSLAVE_CMD_ID_GPUP:
                return putslave_do_cmd_q_gpup(pslave, pstr);
            }
    } else if (cmd == '!') {
        if (mod_msk == 0)
            switch (cmd_id) {
            case PUTSLAVE_CMD_ID_RST:
                return putslave_do_cmd_a_rst(pslave);
            case PUTSLAVE_CMD_ID_SER:
                return putslave_do_cmd_a_ser(pslave, pstr);
            case PUTSLAVE_CMD_ID_BREV:
                return putslave_do_cmd_a_brev(pslave, pstr);
            case PUTSLAVE_CMD_ID_BTIM:
                return putslave_do_cmd_a_btim(pslave, pstr);
            case PUTSLAVE_CMD_ID_MAC:
                return putslave_do_cmd_a_mac(pslave, pstr);
            case PUTSLAVE_CMD_ID_LOCK:
                return putslave_do_cmd_a_lock(pslave);
            case PUTSLAVE_CMD_ID_TST:
                return putslave_do_cmd_a_tst(pslave, pstr);
            case PUTSLAVE_CMD_ID_TONE:
                return putslave_do_cmd_a_tone(pslave, pstr);
#if HAS_GUI
            case PUTSLAVE_CMD_ID_START:
                return putslave_do_cmd_a_start(pslave);
#endif
            case PUTSLAVE_CMD_ID_STOP:
                return putslave_do_cmd_a_stop(pslave);
            case PUTSLAVE_CMD_ID_EECL:
                return putslave_do_cmd_a_eecl(pslave);
            case PUTSLAVE_CMD_ID_FPWM:
                return putslave_do_cmd_a_fpwm(pslave, pstr);
            case PUTSLAVE_CMD_ID_FPSM:
                return putslave_do_cmd_a_fpsm(pslave, pstr);
            case PUTSLAVE_CMD_ID_EEDEF:
                return putslave_do_cmd_a_eedef(pslave);
            case PUTSLAVE_CMD_ID_GPIO:
                return putslave_do_cmd_a_gpio(pslave, pstr);
            case PUTSLAVE_CMD_ID_GPCF:
                return putslave_do_cmd_a_gpcf(pslave, pstr);
            case PUTSLAVE_CMD_ID_DOER:
                return putslave_do_cmd_a_doer(pslave, pstr);
            case PUTSLAVE_CMD_ID_GCODE:
                return putslave_do_cmd_a_gcode(pslave, pstr);
            case PUTSLAVE_CMD_ID_PWM:
                return putslave_do_cmd_a_pwm(pslave, pstr);
            case PUTSLAVE_CMD_ID_INVAL:
                return putslave_do_cmd_a_inval(pslave);
            case PUTSLAVE_CMD_ID_I2C:
                return putslave_do_cmd_a_i2c(pslave);
#if !INIT_TRINAMIC_FROM_MARLIN_ONLY
            case PUTSLAVE_CMD_ID_TEN:
                return putslave_do_cmd_a_ten(pslave, pstr);
            case PUTSLAVE_CMD_ID_MOVE:
                return putslave_do_cmd_a_move(pslave, pstr);
#endif
            }
    }
    return UARTSLAVE_ERR_CNF;
}

static void enter_put() {
#if HAS_GUI
    HAL_SPI_MspDeInit(&hspi2);
#endif
    NVIC_DisableIRQ(TIM7_IRQn);
    hwio_pwm_set_val(HWIO_PWM_HEATER_BED, 0);
    hwio_pwm_set_val(HWIO_PWM_HEATER_0, 0);
    //SCK - PB10
    gpio_init(MARLIN_PORT_PIN(MARLIN_PORT_B, MARLIN_PIN_NR_10), GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH);
    //MISO - PC2
    gpio_init(MARLIN_PORT_PIN(MARLIN_PORT_C, MARLIN_PIN_NR_2), GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_VERY_HIGH);
    //MOSI - PC3
    gpio_init(MARLIN_PORT_PIN(MARLIN_PORT_C, MARLIN_PIN_NR_3), GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH);
}

static int putslave_do_cmd_a_stop(uartslave_t *pslave) {
    if (marlin_server_processing()) {
        marlin_server_stop_processing();
        if (HAS_GUI)
            osThreadSuspend(displayTaskHandle);
#ifdef BUDDY_ENABLE_ETHERNET
        osThreadSuspend(webServerTaskHandle);
#endif //BUDDY_ENABLE_ETHERNET
        enter_put();
    }
    return UARTSLAVE_OK;
}

void putslave_init(uartslave_t *pslave) {
    pslave->parse_cmd_id = putslave_parse_cmd_id;
    pslave->do_cmd = putslave_do_cmd;
    pslave->flags = UARTSLAVE_FLG_ECHO;
    if (diag_fastboot) {
        uartslave_printf(pslave, "fastboot\n");
        marlin_server_stop_processing();
        enter_put();
    }
}
