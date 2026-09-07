/// @file
#include "screen_menu_version_info.hpp"

#include "config.h"
#include <version/version.hpp>
#include "img_resources.hpp"
#include "shared_config.h" //BOOTLOADER_VERSION_ADDRESS
#include <feature/filament_sensor/filament_sensors_handler.hpp>
#include <utils/string_builder.hpp>

#include <option/bootloader.h>
#include <option/has_mmu2.h>

#if HAS_MMU2()
    #include "Marlin/src/feature/prusa/MMU2/mmu2_mk4.h"
#endif

MI_INFO_FW::MI_INFO_FW()
    : WI_INFO_t {
        _("Firmware Version"),
    } {
    ChangeInformation(version::project_version_full);
}

#if BOOTLOADER()
MI_INFO_BOOTLOADER::MI_INFO_BOOTLOADER()
    : WI_INFO_t {
        _("Bootloader Version"),
    } {
    ArrayStringBuilder<12> sb;
    const version_t *bootloader_version = (const version_t *)BOOTLOADER_VERSION_ADDRESS;
    sb.append_printf("%d.%d.%d", bootloader_version->major, bootloader_version->minor, bootloader_version->patch);
    ChangeInformation(sb.str());
}
#endif

#if HAS_MMU2()
MI_INFO_MMU::MI_INFO_MMU()
    : WI_INFO_t {
        _("MMU Version"),
    } {
    if (FSensors_instance().HasMMU()) {
        const auto mmu_version = MMU2::mmu2.GetMMUFWVersion();
        if (mmu_version.major != 0) {
            ArrayStringBuilder<12> sb;
            sb.append_printf("%d.%d.%d", mmu_version.major, mmu_version.minor, mmu_version.build);
            ChangeInformation(sb.str());
        } else {
            ChangeInformation("N/A");
        }
        show();
    } else {
        hide();
    }
}
#endif

ScreenMenuVersionInfo::ScreenMenuVersionInfo()
    : ScreenMenuVersionInfo__ {
        _("VERSION INFO"),
        &img::info_16x16,
    } {
    EnableLongHoldScreenAction();
}
