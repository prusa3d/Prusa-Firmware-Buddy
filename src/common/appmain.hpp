/**
 * @file
 * @date Sep 17, 2020
 * @author Marek Bel
 */

#pragma once
#include "config_buddy_2209_02.h"
#ifdef NEW_FANCTL
    #include "fanctl.h"
extern CFanCtl fanCtlPrint;
extern CFanCtl fanCtlHeatBreak;
#endif
