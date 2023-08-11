/**
 * @file version_info_ILI9488.cpp
 */

#include "screen_menu_version_info.hpp"
#include "config.h"
#include "version.h"
#include "img_resources.hpp"
#include "shared_config.h" //BOOTLOADER_VERSION_ADDRESS
#include "../common/otp.hpp"

static constexpr size_t SN_STR_SIZE = 25;

void ScreenMenuVersionInfo::set_serial_number(WiInfo<28> &item, const char *sn, uint8_t bom_id) {
    char tmp[SN_STR_SIZE + 4 + 1]; // len of serial number plus '/' and max 3-digit number and null
    snprintf(tmp, sizeof(tmp), "%s/%u", sn, bom_id);
    item.ChangeInformation(tmp);
}

ScreenMenuVersionInfo::ScreenMenuVersionInfo()
    : ScreenMenuVersionInfo__(_(label)) {
    header.SetIcon(&img::info_16x16);

    char help_str[SN_STR_SIZE] = "";

    serial_nr_t serial_nr;
    otp_get_serial_nr(serial_nr);
    uint8_t bom_id = otp_get_bom_id().value_or(0);

    set_serial_number(Item<MI_INFO_SERIAL_NUM>(), serial_nr.begin(), bom_id);

    strncpy(help_str, project_version_full, GuiDefaults::infoDefaultLen);
    help_str[GuiDefaults::infoDefaultLen - 1] = 0;
    Item<MI_INFO_FW>().ChangeInformation(help_str);

    const version_t *bootloader = (const version_t *)BOOTLOADER_VERSION_ADDRESS;
    snprintf(help_str, GuiDefaults::infoDefaultLen, "%d.%d.%d", bootloader->major, bootloader->minor, bootloader->patch);
    Item<MI_INFO_BOOTLOADER>().ChangeInformation(help_str);

    snprintf(help_str, GuiDefaults::infoDefaultLen, "%d", otp_get_board_revision().value_or(0));

    Item<MI_INFO_BOARD>().ChangeInformation(help_str);

    EnableLongHoldScreenAction();
}
