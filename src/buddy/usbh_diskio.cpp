#include "ff_gen_drv.h"
#include "usbh_diskio.h"
#include "usbh_async_diskio.hpp"
#include "ccm_thread.hpp"
#include "usb_host.h"

#include <freertos/binary_semaphore.hpp>
#include <freertos/mutex.hpp>
#include <common/freertos_shared_mutex.hpp>
#include <mutex>
#include <shared_mutex>
#include <utility_extensions.hpp>
#include <bit>
#include <FreeRTOS.h>
#include <logging/log.hpp>

LOG_COMPONENT_REF(USBHost);
using Mutex = freertos::Mutex;
using Lock = std::unique_lock<Mutex>;

static const uint16_t USB_DEFAULT_BLOCK_SIZE = FF_MIN_SS;
static DWORD scratch[FF_MAX_SS / 4];

extern USBH_HandleTypeDef hUsbHostHS;

DSTATUS USBH_initialize(BYTE);
DSTATUS USBH_status(BYTE);
DRESULT USBH_read(BYTE, BYTE *, DWORD, UINT);

#if FF_FS_READONLY == 0
DRESULT USBH_write(BYTE, const BYTE *, DWORD, UINT);
DRESULT USBH_ioctl(BYTE, BYTE, void *);
#endif /* FF_FS_READONLY == 0 */

const Diskio_drvTypeDef USBH_Driver = {
    USBH_initialize,
    USBH_status,
    USBH_read,
#if FF_FS_READONLY == 0
    USBH_write,
    USBH_ioctl,
#endif /* FF_FS_READONLY == 0 */
};

osThreadId USBH_MSC_WorkerTaskHandle;

static constexpr size_t queue_length = 5;

static QueueHandle_t request_queue;
static StaticQueue_t queue;
static uint8_t storage_area[queue_length * sizeof(UsbhMscRequest *)];

void USBH_worker_notify(USBH_StatusTypeDef, void *semaphore, void *) {
    static_cast<freertos::BinarySemaphore *>(semaphore)->release();
}

// Queues the r/w request for processing (USBH_MSC_WorkerTask does it) and blocks until the end of processing is reported by the callback
// we don't have any io scheduler, but if only tasks with the same priority send
// their requests, they will be distributed fairly.
static USBH_StatusTypeDef USBH_exec(UsbhMscRequest::UsbhMscRequestOperation operation,
    BYTE lun, BYTE *buff, DWORD sector, uint16_t count) {
    freertos::BinarySemaphore semaphore;
    UsbhMscRequest request {
        operation,
        lun,
        count,
        sector,
        buff,
        USBH_FAIL,
        USBH_worker_notify,
        &semaphore,
        nullptr
    };
    UsbhMscRequest *request_ptr = &request;

    if (xQueueSend(request_queue, &request_ptr, USBH_MSC_RW_MAX_DELAY) != pdPASS) {
        return USBH_FAIL;
    }

    semaphore.acquire();

    return request.result;
}

USBH_StatusTypeDef usbh_msc_submit_request(UsbhMscRequest *request) {
    // we don't have any io scheduler, but if only tasks with the same priority send
    // their requests, they will be distributed fairly
    assert(((DWORD)request->data & 3) == 0);
    if (xQueueSend(request_queue, &request, portMAX_DELAY) != pdPASS) {
        return USBH_FAIL;
    }
    return USBH_OK;
}

// A real-time priority task that takes individual requests from the request_queue and executes them,
// most operations are deferred work from USB interrrupts, so they should be executed immediately (real-time priority).
// Total CPU time is relatively small.
static void USBH_MSC_WorkerTask(void const *) {
    for (;;) {
        UsbhMscRequest *request;
        BaseType_t queue_status = xQueueReceive(request_queue, &request, portMAX_DELAY);
        if (queue_status == pdPASS && request->operation != UsbhMscRequest::UsbhMscRequestOperation::Noop) {
            {
                switch (request->operation) {
                case UsbhMscRequest::UsbhMscRequestOperation::Read: {
                    freertos::SharedMutexProxy mutex { &hUsbHostHS.class_mutex };
                    std::shared_lock lock { mutex };
                    request->result = USBH_MSC_Read(&hUsbHostHS, request->lun, request->sector_nbr, request->data, request->count);
                } break;
                case UsbhMscRequest::UsbhMscRequestOperation::Write: {
                    freertos::SharedMutexProxy mutex { &hUsbHostHS.class_mutex };
                    std::shared_lock lock { mutex };
                    request->result = USBH_MSC_Write(&hUsbHostHS, request->lun, request->sector_nbr, request->data, request->count);
                } break;
                default:
                    abort();
                }
            }

            if (request->result != USBH_OK) {
                usbh_power_cycle::io_error();
            }
            if (request->callback) {
                request->callback(request->result, request->callback_param1, request->callback_param2);
            }
        }
    }
}

static void USBH_StartMSCWorkerTask() {
    request_queue = xQueueCreateStatic(queue_length, sizeof(UsbhMscRequest *), storage_area, &queue);
    configASSERT(request_queue);
    osThreadCCMDef(USBH_MSC_WorkerTask, USBH_MSC_WorkerTask, TASK_PRIORITY_USB_MSC_WORKER_HIGH, 0U, 512);
    USBH_MSC_WorkerTaskHandle = osThreadCreate(osThread(USBH_MSC_WorkerTask), NULL);
}

DSTATUS USBH_initialize([[maybe_unused]] BYTE lun) {
    USBH_StartMSCWorkerTask();
    /* CAUTION : USB Host library has to be initialized in the application */
    return RES_OK;
}

/**
 * @brief  Gets Disk Status
 * @param  lun : lun id
 * @retval DSTATUS: Operation status
 */
DSTATUS USBH_status(BYTE lun) {
    DRESULT res = RES_ERROR;
    freertos::SharedMutexProxy mutex { &hUsbHostHS.class_mutex };
    std::shared_lock lock { mutex };
    if (USBH_MSC_UnitIsReady(&hUsbHostHS, lun)) {
        res = RES_OK;
    } else {
        res = RES_ERROR;
    }

    return res;
}

static USBH_StatusTypeDef USBH_MSC_GetLUNInfo_synchronized(
    USBH_HandleTypeDef *phost,
    uint8_t lun,
    MSC_LUNTypeDef *info) {
    freertos::SharedMutexProxy mutex { &phost->class_mutex };
    std::shared_lock lock { mutex };
    return USBH_MSC_GetLUNInfo(phost, lun, info);
}

/**
 * @brief  Reads Sector(s)
 * @param  lun : lun id
 * @param  *buff: Data buffer to store read data
 * @param  sector: Sector address (LBA)
 * @param  count: Number of sectors to read (1..128)
 * @retval DRESULT: Operation result
 */
DRESULT USBH_read(BYTE lun, BYTE *buff, DWORD sector, UINT count) {
    DRESULT res = RES_ERROR;
    MSC_LUNTypeDef info;
    USBH_StatusTypeDef status = USBH_OK;

    if ((DWORD)buff & 3) { // DMA Alignment issue, do single up to aligned buffer
        while ((count--) && (status == USBH_OK)) {
            status = USBH_exec(UsbhMscRequest::UsbhMscRequestOperation::Read, lun, (uint8_t *)scratch, sector + count, 1);
            if (status == USBH_OK) {
                memcpy(&buff[count * FF_MAX_SS], scratch, FF_MAX_SS);
            } else {
                break;
            }
        }
    } else {
        status = USBH_exec(UsbhMscRequest::UsbhMscRequestOperation::Read, lun, buff, sector, count);
    }

    if (status == USBH_OK) {
        res = RES_OK;
    } else {
        USBH_MSC_GetLUNInfo_synchronized(&hUsbHostHS, lun, &info);
        switch (info.sense.asc) {
        case SCSI_ASC_LOGICAL_UNIT_NOT_READY:
        case SCSI_ASC_MEDIUM_NOT_PRESENT:
        case SCSI_ASC_NOT_READY_TO_READY_CHANGE:
            USBH_ErrLog("USB Disk is not ready!"); // not localized, only writes to debug log
            res = RES_NOTRDY;
            break;

        default:
            res = RES_ERROR;
            break;
        }
    }

    return res;
}

/**
 * @brief  Writes Sector(s)
 * @param  lun : lun id
 * @param  *buff: Data to be written
 * @param  sector: Sector address (LBA)
 * @param  count: Number of sectors to write (1..128)
 * @retval DRESULT: Operation result
 */
#if FF_FS_READONLY == 0
DRESULT USBH_write(BYTE lun, const BYTE *buff, DWORD sector, UINT count) {
    DRESULT res = RES_ERROR;
    MSC_LUNTypeDef info;
    USBH_StatusTypeDef status = USBH_OK;
    if ((DWORD)buff & 3) { // DMA Alignment issue, do single up to aligned buffer
        USBH_ErrLog("Suspicious DMA Alignment issue, do single up to aligned buffer");
        while (count--) {
            memcpy(scratch, &buff[count * FF_MAX_SS], FF_MAX_SS);
            status = USBH_exec(UsbhMscRequest::UsbhMscRequestOperation::Write, lun, (BYTE *)scratch, sector + count, 1);
            if (status == USBH_FAIL) {
                break;
            }
        }
    } else {
        status = USBH_exec(UsbhMscRequest::UsbhMscRequestOperation::Write, lun, (BYTE *)buff, sector, count);
    }

    if (status == USBH_OK) {
        res = RES_OK;
    } else {
        USBH_MSC_GetLUNInfo_synchronized(&hUsbHostHS, lun, &info);
        switch (info.sense.asc) {
        case SCSI_ASC_WRITE_PROTECTED:
            USBH_ErrLog("USB Disk is Write protected!"); // not localized, only writes to debug log
            res = RES_WRPRT;
            break;

        case SCSI_ASC_LOGICAL_UNIT_NOT_READY:
        case SCSI_ASC_MEDIUM_NOT_PRESENT:
        case SCSI_ASC_NOT_READY_TO_READY_CHANGE:
            USBH_ErrLog("USB Disk is not ready!"); // not localized, only writes to debug log
            res = RES_NOTRDY;
            break;

        default:
            res = RES_ERROR;
            break;
        }
    }

    return res;
}
#endif /* FF_FS_READONLY == 0 */

/**
 * @brief  I/O control operation
 * @param  lun : lun id
 * @param  cmd: Control code
 * @param  *buff: Buffer to send/receive control data
 * @retval DRESULT: Operation result
 */
#if FF_FS_READONLY == 0
DRESULT USBH_ioctl(BYTE lun, BYTE cmd, void *buff) {
    USBH_StatusTypeDef lun_info_res = USBH_FAIL;
    DRESULT res = RES_ERROR;
    MSC_LUNTypeDef info;

    switch (cmd) {
    /* Make sure that no pending write process */
    case CTRL_SYNC:
        res = RES_OK;
        break;

    /* Get number of sectors on the disk (DWORD) */
    case GET_SECTOR_COUNT:
        lun_info_res = USBH_MSC_GetLUNInfo_synchronized(&hUsbHostHS, lun, &info);
        if (lun_info_res == USBH_OK) {
            *(DWORD *)buff = info.capacity.block_nbr;
            res = RES_OK;
        } else {
            res = RES_ERROR;
        }
        break;

    /* Get R/W sector size (WORD) */
    case GET_SECTOR_SIZE:
        lun_info_res = USBH_MSC_GetLUNInfo_synchronized(&hUsbHostHS, lun, &info);
        if (lun_info_res == USBH_OK) {
            *(DWORD *)buff = info.capacity.block_size;
            res = RES_OK;
        } else {
            res = RES_ERROR;
        }
        break;

        /* Get erase block size in unit of sector (DWORD) */
    case GET_BLOCK_SIZE:
        lun_info_res = USBH_MSC_GetLUNInfo_synchronized(&hUsbHostHS, lun, &info);
        if (lun_info_res == USBH_OK) {
            *(DWORD *)buff = info.capacity.block_size / USB_DEFAULT_BLOCK_SIZE;
            res = RES_OK;
        } else {
            res = RES_ERROR;
        }
        break;

    default:
        res = RES_PARERR;
    }

    return res;
}
#endif /* FF_FS_READONLY == 0 */
