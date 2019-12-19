// fake_uiapi.cpp

#include "../Marlin/src/lcd/extensible_ui/ui_api.h"
#include "../Marlin/src/lcd/ultralcd.h"
#include "../Marlin/src/module/planner.h"

extern MarlinUI ui;

namespace ExtUI {

bool isMoving() {
    return planner.has_blocks_queued();
}

uint8_t getProgress_percent() {
    return ui.get_progress();
}

}
