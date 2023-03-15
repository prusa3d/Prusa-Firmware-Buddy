#include "main.h"
#include <device/hal.h>
#include "cmsis_os.h"
#include "SEGGER_SYSVIEW.h"
#include "crc32.h"
#include "eeprom.h"
#include "tick_timer_api.h"
#include <assert.h>

static void enable_trap_on_division_by_zero() {
    SCB->CCR |= SCB_CCR_DIV_0_TRP_Msk;
}

static void enable_backup_domain() {
    // this allows us to use the RTC->BKPXX registers
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();
}

static void enable_segger_sysview() {
    // enable the cycle counter for correct time reporting
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    SEGGER_SYSVIEW_Conf();
}

static void enable_dfu_entry() {
#ifdef BUDDY_ENABLE_DFU_ENTRY
    // check whether user requested to enter the DFU mode
    // this has to be checked after having
    //  1) initialized access to the backup domain
    //  2) having initialized related clocks (SystemClock_Config)
    if (sys_dfu_requested())
        sys_dfu_boot_enter();
#endif
}

static void eeprom_init_i2c() {
    I2C_INIT(eeprom);
}

/// The entrypoint of the startup task
///
/// WARNING
/// The C++ runtime isn't initialized at the beginning of this function
/// and initializing it is the main priority here.
/// So first, we have to get the EEPROM ready, then we call libc_init_array
/// and that is the time everything is ready for us to switch to C++ context.
static void startup_task(void const *argument) {
    // init crc32 module. We need crc in eeprom_init
    crc32_init();

    // init communication with eeprom
    eeprom_init_i2c();

    // init eeprom module itself
    taskENTER_CRITICAL();
    eeprom_init();
    taskEXIT_CRITICAL();

    // init global variables and call constructors
    extern void __libc_init_array(void);
    __libc_init_array();

    // call the main main() function
    main_cpp();

    // terminate this thread (release its resources), we are done
    osThreadTerminate(osThreadGetId());
}

/// The entrypoint of our firmware
///
/// Do not do anything here that isn't essential to starting the RTOS
/// That is our one and only priority.
///
/// WARNING
/// The C++ runtime hasn't been initialized yet (together with C's constructors).
/// So make sure you don't do anything that is dependent on it.
int main() {
    // initialize FPU, vector table & external memory
    SystemInit();

    // initialize HAL
    HAL_Init();

    // configure system clock and timing
    system_core_init();
    tick_timer_init();

    // other MCU setup
    enable_trap_on_division_by_zero();
    enable_backup_domain();
    enable_segger_sysview();
    enable_dfu_entry();

    // define the startup task
    osThreadDef(startup, startup_task, osPriorityHigh, 0, 1024);
    osThreadCreate(osThread(startup), NULL);

    // start the RTOS with the single startup task
    osKernelStart();
}
