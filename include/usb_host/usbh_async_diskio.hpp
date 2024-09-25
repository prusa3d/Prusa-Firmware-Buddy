#pragma once

#include <freertos/mutex.hpp>
#include <atomic>
#include <limits>
#include <printers.h>

#ifndef UNITTESTS
    #include "usbh_core.h"
    #include "usbh_msc.h"
#endif

// Task handle of the process for executing the r/w MSC operations
extern osThreadId USBH_MSC_WorkerTaskHandle;

using UsbhMscRequestCallback = void (*)(USBH_StatusTypeDef result, void *param1, void *param2);

// Structure for describing MSC asynchronous r/w operations
struct UsbhMscRequest {
    enum class UsbhMscRequestOperation : uint8_t {
        Read,
        Write,
        Noop
    };

    static const size_t SECTOR_SIZE = 512;

    using LunNbr = uint8_t;
    using SectorNbr = uint32_t;

    UsbhMscRequestOperation operation;
    LunNbr lun;
    // Number of 512B sectors to write
    uint16_t count;
    SectorNbr sector_nbr;
    uint8_t *data;
    USBH_StatusTypeDef result;
    UsbhMscRequestCallback callback;
    void *callback_param1;
    void *callback_param2;
};

USBH_StatusTypeDef usbh_msc_submit_request(UsbhMscRequest *);

#define USBH_MSC_RW_MAX_DELAY_MS (10000)
#define USBH_MSC_RW_MAX_DELAY    (USBH_MSC_RW_MAX_DELAY_MS / portTICK_PERIOD_MS)
