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
#include "hw_configuration.hpp"
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

    if (!(regState[0] & 0x80)) {
        return -100; // Try again error
    }

    touch_num = regState[0] & 0x0f;

    if (touch_num > 0) {

        error = Read(GOODIX_READ_COORD_ADDR + 1, data, sizeof(GTPoint) * (touch_num));

        if (error) {
            return -error;
        }
    }

    return touch_num;
}

uint8_t GT911::CalcChecksum(uint8_t *buf, uint8_t len) {
    uint8_t ccsum = 0;
    for (uint8_t i = 0; i < len; i++) {
        ccsum += buf[i];
    }
    // ccsum %= 256;
    ccsum = (~ccsum) + 1;
    return ccsum;
}

GT911::GT911()
    : inverted_interrupt(buddy::hw::Configuration::Instance().has_inverted_touch_interrupt()) {
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
            const size_t offset = 0; // we need oly 1st point
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
        // read touch settings
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
        // read touch settings
        uint8_t cfg[CONFIG_LEN];
        int ret = fread((void *)cfg, 1, CONFIG_LEN, fd);

        cfg[CONFIG_LEN - 2] = GT911::CalcChecksum(cfg, CONFIG_LEN - 2); // recalculate checksum
        cfg[CONFIG_LEN - 1] = 1; // enforce update flag
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
        if (status != HAL_StatusTypeDef::HAL_OK) {
            log_warning(Touch, "config read error, attempt %u of %u ", try_no, max_try);
        }
    } while (status != HAL_StatusTypeDef::HAL_OK && try_no < max_try);

    // compare all but checksum and update flag
    if (status == HAL_StatusTypeDef::HAL_OK && memcmp(cfg, default_config, CONFIG_LEN - 2)) {
        log_info(Touch, "updating chip registers");

        memcpy(cfg, default_config, CONFIG_LEN - 2);
        cfg[CONFIG_LEN - 2] = CalcChecksum(cfg, CONFIG_LEN - 2); // recalculate checksum (not needed when setting from default data, just to be safe)
        cfg[CONFIG_LEN - 1] = 1; // enforce update flag

        try_no = 0;
        do {
            status = Write(GT_REG_CFG, cfg, CONFIG_LEN);
            ++try_no;
            if (status != HAL_StatusTypeDef::HAL_OK) {
                log_warning(Touch, "config write error, attempt %u of %u ", try_no, max_try);
            }
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
    if (buddy::hw::touch_sig.read() == (Instance().has_inverted_interrupt() ? buddy::hw::Pin::State::low : buddy::hw::Pin::State::high)) {
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
