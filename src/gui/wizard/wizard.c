// wizard.c

#include "wizard.h"
#include "gui.h"
#include "dbg.h"
#include "config.h"
#include "screen_wizard.h"
#include "screens.h"

extern uint64_t wizard_mask;

//detailed caption constants for debugging
const char *wizard_caption[] = {
    "WIZARD start",
    "WIZARD info",
    "WIZARD first",
    "SELFTEST fan0",
    "SELFTEST fan1",
    "SELFTEST x",
    "SELFTEST y",
    "SELFTEST z",
    "SELFTEST home",
    "SELFTEST temp",
    "SELFTEST pass",
    "SELFTEST fail",
    "XYZCALIB home",
    "XYZCALIB z",
    "XYZCALIB xy0",
    "XYZCALIB xy1",
    "XYZCALIB xy2",
    "XYZCALIB xy3",
    "XYZCALIB xy4",
    "XYZCALIB xy5",
    "XYZCALIB xy6",
    "XYZCALIB xy7",
    "XYZCALIB pass",
    "XYZCALIB fail",
    "FIRSTLAY 0",
    "FIRSTLAY 1",
    "FIRSTLAY 2",
    "FIRSTLAY 3",
    "FIRSTLAY 4",
    "FIRSTLAY 5",
    "FIRSTLAY 6",
    "FIRSTLAY 7",
    "FINISH",
};

void wizard_run_mask(uint64_t mask) {
    wizard_mask = mask;
    screen_open(get_scr_wizard()->id);
}

void wizard_run_complete(void) {
    wizard_run_mask(_STATE_MASK_WIZARD);
}

void wizard_run_selftest(void) {
    wizard_run_mask(_STATE_MASK_SELFTEST);
}

void wizard_run_xyzcalib(void) {
    //	wizard_run_mask(_STATE_MASK_XYZCALIB);
    uint64_t mask = (_STATE_MASK(_STATE_XYZCALIB_INIT) | _STATE_MASK(_STATE_XYZCALIB_HOME) |
#ifdef WIZARD_Z_CALIBRATION
        _STATE_MASK(_STATE_XYZCALIB_Z) |
#endif
        _STATE_MASK(_STATE_XYZCALIB_XY_MSG_CLEAN_NOZZLE) | _STATE_MASK(_STATE_XYZCALIB_XY_MSG_IS_SHEET) | _STATE_MASK(_STATE_XYZCALIB_XY_MSG_REMOVE_SHEET) | _STATE_MASK(_STATE_XYZCALIB_XY_MSG_PLACE_PAPER) | _STATE_MASK(_STATE_XYZCALIB_XY_SEARCH) | _STATE_MASK(_STATE_XYZCALIB_XY_MSG_PLACE_SHEET) | _STATE_MASK(_STATE_XYZCALIB_XY_MEASURE) | _STATE_MASK(_STATE_XYZCALIB_PASS) | _STATE_MASK(_STATE_XYZCALIB_FAIL) | _STATE_MASK(_STATE_LAST));
    wizard_run_mask(mask);
}

void wizard_run_xyzcalib_xy(void) {
    //	wizard_run_mask(_STATE_MASK_XYZCALIB_XY);
    uint64_t mask = (_STATE_MASK(_STATE_XYZCALIB_INIT) | _STATE_MASK(_STATE_XYZCALIB_HOME) | _STATE_MASK(_STATE_XYZCALIB_PASS) | _STATE_MASK(_STATE_XYZCALIB_FAIL) | _STATE_MASK(_STATE_LAST));
    wizard_run_mask(mask);
}

void wizard_run_xyzcalib_z(void) {
    //	wizard_run_mask(_STATE_MASK_XYZCALIB_Z);
    uint64_t mask = (_STATE_MASK(_STATE_XYZCALIB_INIT) | _STATE_MASK(_STATE_XYZCALIB_HOME) | _STATE_MASK(_STATE_XYZCALIB_Z) | _STATE_MASK(_STATE_XYZCALIB_PASS) | _STATE_MASK(_STATE_XYZCALIB_FAIL) | _STATE_MASK(_STATE_LAST));
    wizard_run_mask(mask);
}

void wizard_run_firstlay(void) {
    //	wizard_run_mask(_STATE_MASK_FIRSTLAY);
    uint64_t mask = (_STATE_MASK(_STATE_FIRSTLAY_INIT) | _STATE_MASK(_STATE_FIRSTLAY_LOAD) | _STATE_MASK(_STATE_FIRSTLAY_MSBX_CALIB) | _STATE_MASK(_STATE_FIRSTLAY_MSBX_START_PRINT) | _STATE_MASK(_STATE_FIRSTLAY_PRINT) | _STATE_MASK(_STATE_FIRSTLAY_MSBX_REPEAT_PRINT) | _STATE_MASK(_STATE_FIRSTLAY_FAIL) | _STATE_MASK(_STATE_LAST));
    wizard_run_mask(mask);
}
