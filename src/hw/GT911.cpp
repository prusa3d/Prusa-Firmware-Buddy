/**
 * @file GT911.cpp
 * @brief this should be universal GT911 touch driver class
 * but currently has lot of dependency
 * also hw::pin does not support to be output switchable to interrupt input
 * TODO refactor
 */

#include "GT911.hpp"
#include "touch_get.hpp"
#include "touch_dependency.hpp"
#include "hwio_pindef.h"
#include "timing.h"
#include "main.h" // LCD_I2C
#include "log.h"
#include <stdio.h>
#include <unistd.h>
#include <device/peripherals.h>

LOG_COMPONENT_DEF(Touch, LOG_SEVERITY_INFO);

/**
 * @brief reset of touch controller
 * 1. RESET low, INT low
 * 2. wait >100ms
 * slave address to 0xBA/0xBB:
 * 3. INT high or low (0xBA/0xBB) or (0x28/0x29)
 * 4. wait >100us
 * 5. RESET high while int high
 * 6. wait >5ms
 * 7. INT low
 * end of slave address to 0xBA/0xBB:
 * 8. wait >50ms
 * 9. config interrupt
 */
void GT911::ResetChip(touch::reset_clr_fnc_t reset_clr_fnc) {
    using namespace buddy::hw;
    {
        OutputEnabler touch_sig_output(touch_sig, Pin::State::low, OMode::pushPull, OSpeed::low);

        /* T1: > 100us */
        delay_us(110);

        /* begin select I2C slave addr */

        /* HIGH: 0x28/0x29 (0x14 7bit), LOW: 0xBA/0xBB (0x5D 7bit) */
        touch_sig_output.write(devAddress == 0x14 ? Pin::State::high : Pin::State::low);

        /* T2: > 100us */
        delay_us(110);

        reset_clr_fnc();

        /* T3: > 5ms */
        delay_ms(6);

        touch_sig_output.write(Pin::State::low);
        /* end select I2C slave addr */

        /* T4: 50ms */
        delay_ms(51);
    }
}

// it returns uint8_t to remove dependency of header on HAL
uint8_t GT911::Read(uint16_t address, uint8_t *buff, uint8_t len) {
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&I2C_HANDLE_FOR(touch), 0xBA, address, I2C_MEMADD_SIZE_16BIT, buff, len, TIMEOUT);
    return status;
}

// it returns uint8_t to remove dependency of header on HAL
uint8_t GT911::Write(uint16_t reg, uint8_t *buff, uint8_t len) {
    return HAL_I2C_Mem_Write(&I2C_HANDLE_FOR(touch), 0xBA, reg, I2C_MEMADD_SIZE_16BIT, buff, len, TIMEOUT);
}

bool GT911::IsConnected() {
    HAL_StatusTypeDef result = HAL_I2C_IsDeviceReady(&I2C_HANDLE_FOR(touch), 0xBA, 2, 2);
    return result == HAL_OK;
}

void GT911::ReadID() {
    uint8_t buff[4];

    Read(0x8144, buff, 2);
}

/**
 * @brief to test if touch is communicating
 *
 */
bool GT911::CanReadConfig() {
    uint8_t val;

    uint8_t status = Read(GT_REG_CFG, &val, 1);

    return status == HAL_StatusTypeDef::HAL_OK && val == default_config[0];
}

/**
 * @brief Read touch points into array
 *  HAL_ERROR       = -1
 *  HAL_BUSY        = -2
 *  HAL_TIMEOUT     = -3
 *  TRY_AGAIN_ERROR = -100
 * @param data array to store points
 * @return int8_t returns number of points or error as negative value
 */
int8_t GT911::ReadInput(uint8_t *data) {
    int touch_num;
    int error;

    uint8_t regState[1];

    error = Read(GOODIX_READ_COORD_ADDR, regState, 1);

    if (error) {
        return -error;
    }

    if (!(regState[0] & 0x80))
        return -100; // Try again error

    touch_num = regState[0] & 0x0f;

    if (touch_num > 0) {

        error = Read(GOODIX_READ_COORD_ADDR + 1, data, sizeof(GTPoint) * (touch_num));

        if (error)
            return -error;
    }

    return touch_num;
}

uint8_t GT911::CalcChecksum(uint8_t *buf, uint8_t len) {
    uint8_t ccsum = 0;
    for (uint8_t i = 0; i < len; i++) {
        ccsum += buf[i];
    }
    //ccsum %= 256;
    ccsum = (~ccsum) + 1;
    return ccsum;
}

static GT911 &Instance() {
    static GT911 ret;
    return ret;
}

void GT911::Input() {

    static int8_t last_touch_count = 0;

    int8_t touch_count = ReadInput(points);

    // negative value == error
    if (touch_count >= 0) {

        // uncomment to log all actions
        // log_info(Touch, "touch_count %d",touch_count );

        // released after single touch
        // do this before write
        if (touch_count == 0 && last_touch_count == 1) {
            const size_t offset = 0;                                       // we need oly 1st point
            GT911::GTPoint *p_point = (GT911::GTPoint *)(points + offset); // get point
            log_info(Touch, "point ID %d, x %d, y %d, area %d", p_point->trackId, p_point->x_position, p_point->y_position, p_point->area);

            GT911::encode_union pt;
            pt.point.x = (p_point->x_position);
            pt.point.y = (p_point->y_position);
            pt.point.read = false;
            encoded_point = pt.u32;
        }

        Write(GOODIX_READ_COORD_ADDR, 0, 1);

        last_touch_count = touch_count;
    } else {
        log_warning(Touch, "Touch read error %d ", touch_count);
    }
}

std::optional<point_ui16_t> GT911::Get() {
    encode_union pt;
    pt.u32 = encoded_point;
    if (pt.point.read) {
        return std::nullopt;
    }
    point_ui16_t ret { pt.point.x, pt.point.y };

    pt.point.read = true;
    encoded_point = pt.u32;
    return ret;
}

bool GT911::Download(const char *fn) {
    FILE *fd;
    fd = fopen(fn, "w");
    if (fd) {
        //read touch settings
        uint8_t cfg[CONFIG_LEN];
        Instance().Read(GT911::GT_REG_CFG, cfg, CONFIG_LEN);

        int ret = fwrite((void *)cfg, 1, CONFIG_LEN, fd);

        fclose(fd);

        return ret == CONFIG_LEN;
    }
    return 0;
}

bool GT911::Upload(const char *fn) {
    FILE *fd;
    fd = fopen(fn, "r");
    if (fd) {
        //read touch settings
        uint8_t cfg[CONFIG_LEN];
        int ret = fread((void *)cfg, 1, CONFIG_LEN, fd);

        cfg[CONFIG_LEN - 2] = GT911::CalcChecksum(cfg, CONFIG_LEN - 2); // recalculate checksum
        cfg[CONFIG_LEN - 1] = 1;                                        // enforce update flag
        Instance().Write(GT911::GT_REG_CFG, cfg, CONFIG_LEN);

        fclose(fd);

        return ret == CONFIG_LEN;
    }
    return 0;
}

bool GT911::ResetRegisters() {
    uint8_t cfg[CONFIG_LEN];
    uint8_t status;
    const unsigned max_try = 5;
    unsigned try_no = 0;
    do {
        status = Read(GT_REG_CFG, cfg, CONFIG_LEN);
        ++try_no;
        if (status != HAL_StatusTypeDef::HAL_OK)
            log_warning(Touch, "config read error, attempt %u of %u ", try_no, max_try);
    } while (status != HAL_StatusTypeDef::HAL_OK && try_no < max_try);

    // compare all but checksum and update flag
    if (status == HAL_StatusTypeDef::HAL_OK && memcmp(cfg, default_config, CONFIG_LEN - 2)) {
        log_info(Touch, "updating chip registers");

        memcpy(cfg, default_config, CONFIG_LEN - 2);
        cfg[CONFIG_LEN - 2] = CalcChecksum(cfg, CONFIG_LEN - 2); // recalculate checksum (not needed when setting from default data, just to be safe)
        cfg[CONFIG_LEN - 1] = 1;                                 // enforce update flag

        try_no = 0;
        do {
            status = Write(GT_REG_CFG, cfg, CONFIG_LEN);
            ++try_no;
            if (status != HAL_StatusTypeDef::HAL_OK)
                log_warning(Touch, "config write error, attempt %u of %u ", try_no, max_try);
        } while (status != HAL_StatusTypeDef::HAL_OK && try_no < max_try);
    }

    if (status == HAL_StatusTypeDef::HAL_OK) {
        log_info(Touch, "config ok");
        hw_not_operational = false;
    } else {
        log_error(Touch, "not operational");
        hw_not_operational = true;
    }

    return !hw_not_operational;
}

void touch::reset_chip(touch::reset_clr_fnc_t reset_clr_fnc) {
    Instance().ResetChip(reset_clr_fnc);
}

bool touch::set_registers() {
    return Instance().ResetRegisters();
}

bool touch::is_hw_broken() {
    return Instance().IsHwBroken();
}

bool touch::does_read_work() {
    return Instance().CanReadConfig();
}

bool __attribute__((weak)) touch::is_enabled() {
    log_error(Touch, "%s needs to be defined for touch to work", __PRETTY_FUNCTION__);
    return false;
}

void __attribute__((weak)) touch::enable() {
    log_error(Touch, "%s needs to be defined for touch to work", __PRETTY_FUNCTION__);
}

void __attribute__((weak)) touch::disable() {
    log_error(Touch, "%s needs to be defined for touch to work", __PRETTY_FUNCTION__);
}

// driver can give 1 sample per 5 - 20 ms
// but hal is in busy state if we read too fast, 100ms seems to be ok
void touch::poll() {
    static uint32_t now = ticks_ms();
    if (buddy::hw::touch_sig.read() == buddy::hw::Pin::State::high) {
        static uint32_t high = 0;
        ++high;
        if ((ticks_ms() - now) > 100) {
            now = ticks_ms();
            Instance().Input();
        }
    } else {
        static uint32_t low = 0;
        ++low;
    }
}

std::optional<point_ui16_t> touch::Get() {
    return Instance().Get();
};

bool touch::download(const char *fn) {
    return Instance().Download(fn);
}

bool touch::download_as_new() {
    char file_name[sizeof("/usb/touch_backup_999.bin")];
    uint32_t inc = 0;
    snprintf(file_name, sizeof(file_name), "/usb/touch_backup.bin");
    while ((access(file_name, F_OK)) == 0) {
        inc++;
        snprintf(file_name, sizeof(file_name), "/usb/touch_backup_%lu.bin", inc);
    }

    return download(file_name);
}

bool touch::upload(const char *fn) {
    return Instance().Upload(fn);
}

static int workaround_step = 0;
static void clear_workaround_step() {
    workaround_step = 0;
}

static void wait_for_pin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState pin_state, uint32_t max_wait_us = 128) {
    volatile uint32_t start_us = ticks_us();
    while (HAL_GPIO_ReadPin(GPIOx, GPIO_Pin) != pin_state) {
        volatile uint32_t now = ticks_us();
        if (((now - start_us) > max_wait_us) && HAL_GPIO_ReadPin(GPIOx, GPIO_Pin) != pin_state) {
            log_error(Touch, "not operational step %d", workaround_step++);
            return;
        }
    }
}

/**
 * @brief I2C is busy and does not communicate
 * I have found similar issue in STM32F1 error datasheet (but not in STM32F4)
 * Description
 * The I2C analog filters embedded in the I2C I/Os may be tied to low level, whereas SCL and SDA lines are kept at
 * high level. This can occur after an MCU power-on reset, or during ESD stress. Consequently, the I2C BUSY flag
 * is set, and the I2C cannot enter master mode (START condition cannot be sent). The I2C BUSY flag cannot be
 * cleared by the SWRST control bit, nor by a peripheral or a system reset. BUSY bit is cleared under reset, but it
 * is set high again as soon as the reset is released, because the analog filter output is still at low level. This issue
 * occurs randomly.
 *
 * Note: Under the same conditions, the I2C analog filters may also provide a high level, whereas SCL and SDA lines are
 * kept to low level. This should not create issues as the filters output is correct after next SCL and SDA transition.
 *
 *
 * Workaround
 * The SCL and SDA analog filter output is updated after a transition occurs on the SCL and SDA line respectively.
 * The SCL and SDA transition can be forced by software configuring the I2C I/Os in output mode. Then, once the
 * analog filters are unlocked and output the SCL and SDA lines level, the BUSY flag can be reset with a software
 * reset, and the I2C can enter master mode. Therefore, the following sequence must be applied:
 *
 * In case we need this for multiple I2C, create new logging component and move this function to separate file
 */
static void i2c_workaround(I2C_HandleTypeDef *hi2c, GPIO_TypeDef *SDA_PORT, uint32_t SDA_PIN, GPIO_TypeDef *SCL_PORT, uint32_t SCL_PIN) {
    clear_workaround_step();

    GPIO_InitTypeDef GPIO_InitStruct;

    //1. Disable the I2C peripheral by clearing the PE bit in I2Cx_CR1 register.
    __HAL_I2C_DISABLE(hi2c);

    //2. Configure the SCL and SDA I/Os as General Purpose Output Open-Drain, High level (Write 1 to GPIOx_ODR).
    GPIO_InitStruct.Pin = SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
    HAL_GPIO_Init(SDA_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = SCL_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
    HAL_GPIO_Init(SCL_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(SDA_PORT, SDA_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(SCL_PORT, SCL_PIN, GPIO_PIN_SET);

    //3. Check SCL and SDA High level in GPIOx_IDR.
    wait_for_pin(SDA_PORT, SDA_PIN, GPIO_PIN_SET);
    wait_for_pin(SCL_PORT, SCL_PIN, GPIO_PIN_SET);

    //4. Configure the SDA I/O as General Purpose Output Open-Drain, Low level (Write 0 to GPIOx_ODR).
    GPIO_InitStruct.Pin = SDA_PIN;
    HAL_GPIO_Init(SDA_PORT, &GPIO_InitStruct);

    HAL_GPIO_TogglePin(SDA_PORT, SDA_PIN);

    //5. Check SDA Low level in GPIOx_IDR.
    wait_for_pin(SDA_PORT, SDA_PIN, GPIO_PIN_RESET);

    //6. Configure the SCL I/O as General Purpose Output Open-Drain, Low level (Write 0 to GPIOx_ODR).
    GPIO_InitStruct.Pin = SCL_PIN;
    HAL_GPIO_Init(SCL_PORT, &GPIO_InitStruct);

    HAL_GPIO_TogglePin(SCL_PORT, SCL_PIN);

    //7. Check SCL Low level in GPIOx_IDR.
    wait_for_pin(SCL_PORT, SCL_PIN, GPIO_PIN_RESET);

    //8. Configure the SCL I/O as General Purpose Output Open-Drain, High level (Write 1 to GPIOx_ODR).
    GPIO_InitStruct.Pin = SDA_PIN;
    HAL_GPIO_Init(SDA_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(SDA_PORT, SDA_PIN, GPIO_PIN_SET);

    //9. Check SCL High level in GPIOx_IDR.
    wait_for_pin(SDA_PORT, SDA_PIN, GPIO_PIN_SET);

    //10. Configure the SDA I/O as General Purpose Output Open-Drain , High level (Write 1 to GPIOx_ODR).
    GPIO_InitStruct.Pin = SCL_PIN;
    HAL_GPIO_Init(SCL_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(SCL_PORT, SCL_PIN, GPIO_PIN_SET);

    //11. Check SDA High level in GPIOx_IDR.
    wait_for_pin(SCL_PORT, SCL_PIN, GPIO_PIN_SET);

    //12. Configure the SCL and SDA I/Os as Alternate function Open-Drain.
    GPIO_InitStruct.Pin = SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Alternate = 0x04; // 4 == I2C
    HAL_GPIO_Init(SDA_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = SCL_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Alternate = 0x04; // 4 == I2C
    HAL_GPIO_Init(SCL_PORT, &GPIO_InitStruct);

    //13. Set SWRST bit in I2Cx_CR1 register.
    hi2c->Instance->CR1 |= I2C_CR1_SWRST;

    //14. Clear SWRST bit in I2Cx_CR1 register.
    hi2c->Instance->CR1 ^= I2C_CR1_SWRST;

    // extra step call init fnc
    I2C_INIT(touch);

    //15. Enable the I2C peripheral by setting the PE bit in I2Cx_CR1 register.
    __HAL_I2C_ENABLE(hi2c);
}

void touch::touch_workaround() {
    i2c_workaround(&I2C_HANDLE_FOR(touch), TOUCH_SDA_PORT, TOUCH_SDA_PIN, TOUCH_SCL_PORT, TOUCH_SCL_PIN);
}
