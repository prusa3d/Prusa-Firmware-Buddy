/**
 * @file MItem_eeprom.cpp
 * @author Radek Vana
 * @date 2021-09-22
 */
#include "MItem_eeprom.hpp"
#include "i2c.hpp"
#include <guiconfig/GuiDefaults.hpp>
#include "i18n.h"

static constexpr bool use_long_text = GuiDefaults::infoDefaultLen >= 16;
// dev only, don't translate
constexpr static const char *label_CRC_ERR = (use_long_text) ? "INIT_CRC_ERROR" : "I_CRC";
constexpr static const char *label_UPGRADED = (use_long_text) ? "INIT_UPGRADED" : "I_UPG";
constexpr static const char *label_UPGRADE_FAILED = (use_long_text) ? "INIT_UPG_FAILED" : "I_UPF";

static constexpr uint8_t i2c_channels = 3;

constexpr static const char *labels_ok[i2c_channels] = {
    (use_long_text) ? "I2C1_OK" : "I1_OK",
    (use_long_text) ? "I2C2_OK" : "I2_OK",
    (use_long_text) ? "I2C3_OK" : "I3_OK"
};
constexpr static const char *labels_error[i2c_channels] = {
    (use_long_text) ? "I2C1_ERROR" : "I1_ERROR",
    (use_long_text) ? "I2C2_ERROR" : "I2_ERROR",
    (use_long_text) ? "I2C3_ERROR" : "I3_ERROR"
};
constexpr static const char *labels_busy[i2c_channels] = {
    (use_long_text) ? "I2C1_BUSY" : "I1_BUSY",
    (use_long_text) ? "I2C2_BUSY" : "I2_BUSY",
    (use_long_text) ? "I2C3_BUSY" : "I3_BUSY"
};
constexpr static const char *labels_timeout[i2c_channels] = {
    (use_long_text) ? "I2C1_TIMEOUT" : "I1_TIMEOUT",
    (use_long_text) ? "I2C2_TIMEOUT" : "I2_TIMEOUT",
    (use_long_text) ? "I2C3_TIMEOUT" : "I3_TIMEOUT"
};

const uint8_t *get_label_ok(uint8_t channel) {
    --channel;
    if (channel < i2c_channels) {
        return (const uint8_t *)labels_ok[channel];
    }
    return nullptr;
}

const uint8_t *get_label_error(uint8_t channel) {
    --channel;
    if (channel < i2c_channels) {
        return (const uint8_t *)labels_error[channel];
    }
    return nullptr;
}

const uint8_t *get_label_busy(uint8_t channel) {
    --channel;
    if (channel < i2c_channels) {
        return (const uint8_t *)labels_busy[channel];
    }
    return nullptr;
}

const uint8_t *get_label_timeout(uint8_t channel) {
    --channel;
    if (channel < i2c_channels) {
        return (const uint8_t *)labels_timeout[channel];
    }
    return nullptr;
}

MI_EEPROM_INIT_CRC_ERROR::MI_EEPROM_INIT_CRC_ERROR()
    : WI_INFO_DEV_t(0, string_view_utf8::MakeCPUFLASH((const uint8_t *)label_CRC_ERR)) { // TODO(ConfigStore) : Not migrated
}

MI_EEPROM_INIT_UPGRADED::MI_EEPROM_INIT_UPGRADED()
    : WI_INFO_DEV_t(0, string_view_utf8::MakeCPUFLASH((const uint8_t *)label_UPGRADED)) { // TODO(ConfigStore) : Not migrated
}

MI_EEPROM_INIT_UPGRADE_FAILED::MI_EEPROM_INIT_UPGRADE_FAILED()
    : WI_INFO_DEV_t(0, string_view_utf8::MakeCPUFLASH((const uint8_t *)label_UPGRADE_FAILED)) { // TODO(ConfigStore) : Not migrated
}
