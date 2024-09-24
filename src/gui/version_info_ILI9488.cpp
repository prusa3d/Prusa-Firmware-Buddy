/**
 * @file version_info_ILI9488.cpp
 */

#include "screen_menu_version_info.hpp"
#include "config.h"
#include <version/version.hpp>
#include "img_resources.hpp"
#include "shared_config.h" //BOOTLOADER_VERSION_ADDRESS
#include "../common/otp.hpp"
#include "common/filament_sensors_handler.hpp"
#include "Marlin/src/feature/prusa/MMU2/mmu2_mk4.h"

#include <str_utils.hpp>

ScreenMenuVersionInfo::ScreenMenuVersionInfo()
    : ScreenMenuVersionInfo__(_(label)) {
    header.SetIcon(&img::info_16x16);

    {
        serial_nr_t serial_nr;
        otp_get_serial_nr(serial_nr);
        uint8_t bom_id = otp_get_bom_id().value_or(0);

        // len of serial number plus '/' and max 3-digit number and null
        ArrayStringBuilder<serial_nr.size() + 1 + 3 + 1> sb;
        sb.append_printf("%s/%u", serial_nr.data(), bom_id);
        Item<MI_INFO_SERIAL_NUM>().ChangeInformation(sb.str());
    }

    Item<MI_INFO_FW>().ChangeInformation(version::project_version_full);

    {
        const version_t *bootloader_version = (const version_t *)BOOTLOADER_VERSION_ADDRESS;

        ArrayStringBuilder<12> sb;
        sb.append_printf("%d.%d.%d", bootloader_version->major, bootloader_version->minor, bootloader_version->patch);
        Item<MI_INFO_BOOTLOADER>().ChangeInformation(sb.str());
    }

#if HAS_MMU
    if (FSensors_instance().HasMMU()) {
        const auto mmu_version = MMU2::mmu2.GetMMUFWVersion();
        if (mmu_version.major != 0) {
            ArrayStringBuilder<12> sb;
            sb.append_printf("%d.%d.%d", mmu_version.major, mmu_version.minor, mmu_version.build);
            Item<MI_INFO_MMU>().ChangeInformation(sb.str());
        } else {
            Item<MI_INFO_MMU>().ChangeInformation("N/A");
        }
        Item<MI_INFO_MMU>().show();
    } else {
        Item<MI_INFO_MMU>().hide();
    }
#endif

    {
        ArrayStringBuilder<4> sb;
        sb.append_printf("%d", otp_get_board_revision().value_or(0));
        Item<MI_INFO_BOARD>().ChangeInformation(sb.str());
    }

    EnableLongHoldScreenAction();
}
