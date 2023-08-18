#include "ff_gen_drv.h"
#include "usbh_diskio.h"
#include "usbh_async_diskio.hpp"
#include "ccm_thread.hpp"

#include <freertos_mutex.hpp>
#include <mutex>

LOG_COMPONENT_REF(USBHost);
using Mutex = FreeRTOS_Mutex;
using Lock = std::unique_lock<Mutex>;

static const uint16_t USB_DEFAULT_BLOCK_SIZE = FF_MIN_SS;
static DWORD scratch[FF_MAX_SS / 4];
static Mutex mutex;

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

#ifdef USBH_MSC_READAHEAD
UsbhMscReadahead usbh_msc_readahead;
#endif

osThreadId USBH_MSC_WorkerTaskHandle;
static constexpr size_t queue_length = 5;
static QueueHandle_t request_queue;

void USBH_worker_notify(USBH_StatusTypeDef, void *semaphore, void *) {
    xSemaphoreGive(semaphore);
}

// Queues the r/w request for processing (USBH_MSC_WorkerTask does it) and blocks until the end of processing is reported by the callback
// we don't have any io scheduler, but if only tasks with the same priority send
// their requests, they will be distributed fairly.
static USBH_StatusTypeDef USBH_exec(UsbhMscRequest::UsbhMscRequestOperation operation,
    BYTE lun, BYTE *buff, DWORD sector, uint16_t count) {
    StaticSemaphore_t semaphore_data;
    SemaphoreHandle_t semaphore = xSemaphoreCreateBinaryStatic(&semaphore_data);
    UsbhMscRequest request {
        operation,
        lun,
        count,
        sector,
        buff,
        USBH_FAIL,
        USBH_worker_notify,
        semaphore,
        nullptr
    };
    UsbhMscRequest *request_ptr = &request;

    if (xQueueSend(request_queue, &request_ptr, USBH_MSC_RW_MAX_DELAY) != pdPASS)
        return USBH_FAIL;

    xSemaphoreTake(semaphore, portMAX_DELAY);

    return request.result;
}

USBH_StatusTypeDef usbh_msc_submit_request(UsbhMscRequest *request) {
    // we don't have any io scheduler, but if only tasks with the same priority send
    // their requests, they will be distributed fairly
    assert((DWORD)request->data & 3);
    if (xQueueSend(request_queue, &request, portMAX_DELAY) != pdPASS)
        return USBH_FAIL;
    return USBH_OK;
}

// A real-time priority task that takes individual requests from the request_queue and executes them,
// most operations are deferred work from USB interrrupts, so they should be executed immediately (real-time priority).
// Total CPU time is relatively small.
static void USBH_MSC_WorkerTask(void const *) {
    for (;;) {
        UsbhMscRequest *request;
#ifdef USBH_MSC_READAHEAD
        usbh_msc_readahead.send_stats();
        BaseType_t queue_status = xQueueReceive(request_queue, &request, 0);
        if (queue_status != pdPASS) {
            usbh_msc_readahead.read_next_sector();
    #ifdef USBH_MSC_READAHEAD_STATISTICS
            if (uxQueueMessagesWaiting(request_queue)) {
                usbh_msc_readahead.inc_stats_block_another_io();
            }
    #endif
            queue_status = xQueueReceive(request_queue, &request, portMAX_DELAY);
        }
#else
        BaseType_t queue_status = xQueueReceive(request_queue, &request, portMAX_DELAY);
#endif
        if (queue_status == pdPASS) {
            {
                Lock lock(mutex);

                switch (request->operation) {
                case UsbhMscRequest::UsbhMscRequestOperation::Read:
                    request->result = USBH_MSC_Read(&hUsbHostHS, request->lun, request->sector_nbr, request->data, request->count);
#ifdef USBH_MSC_READAHEAD
                    usbh_msc_readahead.set_next_sector(request->lun, request->sector_nbr + request->count);
#endif
                    break;
                case UsbhMscRequest::UsbhMscRequestOperation::Write:
                    request->result = USBH_MSC_Write(&hUsbHostHS, request->lun, request->sector_nbr, request->data, request->count);
                    break;
                }
            }
            if (request->callback) {
                request->callback(request->result, request->callback_param1, request->callback_param2);
            }
        }
    }
}

static void USBH_StartMSCWorkerTask() {
    static StaticQueue_t queue;
    static uint8_t storage_area[queue_length * sizeof(UsbhMscRequest *)];
    request_queue = xQueueCreateStatic(queue_length, sizeof(UsbhMscRequest *), storage_area, &queue);
    configASSERT(request_queue);
    osThreadDef(USBH_MSC_WorkerTask, USBH_MSC_WorkerTask, TASK_PRIORITY_USB_MSC_WORKER, 0U, 512);
    USBH_MSC_WorkerTaskHandle = osThreadCreate(osThread(USBH_MSC_WorkerTask), NULL);
}

DSTATUS USBH_initialize([[maybe_unused]] BYTE lun) {
    Lock lock(mutex);
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
    Lock lock(mutex);
    DRESULT res = RES_ERROR;

    if (USBH_MSC_UnitIsReady(&hUsbHostHS, lun)) {
        res = RES_OK;
    } else {
        res = RES_ERROR;
    }

    return res;
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

#ifdef USBH_MSC_READAHEAD
    if (count == 1 && usbh_msc_readahead.get(lun, sector, buff)) {
        return RES_OK;
    }
#endif

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
        Lock lock(mutex);
        USBH_MSC_GetLUNInfo(&hUsbHostHS, lun, &info);

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
    #ifdef USBH_MSC_READAHEAD
    for (UINT s = sector; s < sector + count; ++s) {
        usbh_msc_readahead.invalidate(lun, s);
    }
    #endif
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
        Lock lock(mutex);
        USBH_MSC_GetLUNInfo(&hUsbHostHS, lun, &info);

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
    Lock lock(mutex);
    DRESULT res = RES_ERROR;
    MSC_LUNTypeDef info;

    switch (cmd) {
    /* Make sure that no pending write process */
    case CTRL_SYNC:
        res = RES_OK;
        break;

    /* Get number of sectors on the disk (DWORD) */
    case GET_SECTOR_COUNT:
        if (USBH_MSC_GetLUNInfo(&hUsbHostHS, lun, &info) == USBH_OK) {
            *(DWORD *)buff = info.capacity.block_nbr;
            res = RES_OK;
        } else {
            res = RES_ERROR;
        }
        break;

    /* Get R/W sector size (WORD) */
    case GET_SECTOR_SIZE:
        if (USBH_MSC_GetLUNInfo(&hUsbHostHS, lun, &info) == USBH_OK) {
            *(DWORD *)buff = info.capacity.block_size;
            res = RES_OK;
        } else {
            res = RES_ERROR;
        }
        break;

        /* Get erase block size in unit of sector (DWORD) */
    case GET_BLOCK_SIZE:

        if (USBH_MSC_GetLUNInfo(&hUsbHostHS, lun, &info) == USBH_OK) {
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

#ifdef USBH_MSC_READAHEAD

bool UsbhMscReadahead::get(UsbhMscRequest::LunNbr lun_nbr, UsbhMscRequest::SectorNbr sector_nbr, uint8_t *data) {
    if (this->lun_nbr != lun_nbr)
        return false;
    if (mutex.try_lock()) {
        if (sector_nbr == cached_sector_nbr) {
            inc_stats_hit();
            goto hit;
        } else {
            inc_stats_missed();
            mutex.unlock();
            return false;
        }
    } else if (sector_nbr == cached_sector_nbr) {
        // sector read in progress => wait for completion
        mutex.lock();
        if (sector_nbr == cached_sector_nbr) {
            inc_stats_hit_in_progress();
            goto hit;
        }
        inc_stats_missed();
        mutex.unlock();
        return false;
    }
    inc_stats_missed();
    return false;
hit:
    memcpy(data, buffer, UsbhMscRequest::SECTOR_SIZE);
    next_sector_nbr = cached_sector_nbr.load() + 1;
    mutex.unlock();
    // wake up the USBH_worker task to perform another readahead
    xTaskNotifyGive(USBH_MSC_WorkerTaskHandle);
    return true;
}

void UsbhMscReadahead::set_next_sector(UsbhMscRequest::LunNbr lun_nbr, UsbhMscRequest::SectorNbr sector_nbr) {
    if (this->lun_nbr != lun_nbr)
        return;
    this->next_sector_nbr = sector_nbr;
}

void UsbhMscReadahead::enable(UsbhMscRequest::LunNbr lun_nbr) {
    this->lun_nbr = lun_nbr;
    next_sector_nbr = INVALID_SECTOR_NBR;
    cached_sector_nbr = INVALID_SECTOR_NBR;
}

void UsbhMscReadahead::disable() {
    this->lun_nbr = INVALID_LUN_NBR;
    next_sector_nbr = INVALID_SECTOR_NBR;
    cached_sector_nbr = INVALID_SECTOR_NBR;
}

void UsbhMscReadahead::invalidate(UsbhMscRequest::LunNbr lun_nbr, UsbhMscRequest::SectorNbr sector_nbr) {
    if (this->lun_nbr != lun_nbr)
        return;
    if (this->cached_sector_nbr == sector_nbr) {
        this->cached_sector_nbr = INVALID_SECTOR_NBR;
    }
}

void UsbhMscReadahead::read_next_sector() {
    if (lun_nbr == INVALID_LUN_NBR
        || next_sector_nbr == INVALID_SECTOR_NBR
        || next_sector_nbr == cached_sector_nbr)
        return;
    #ifdef USBH_MSC_READAHEAD_STATISTICS
    stats_speculative_read_count++;
    #endif

    Lock lock(mutex);
    // it is necessary to set it now so that a possible reading attempt will know about it
    cached_sector_nbr = next_sector_nbr.load();
    auto result = USBH_MSC_Read(&hUsbHostHS, lun_nbr, next_sector_nbr, buffer, 1);
    if (result != USBH_OK) {
        cached_sector_nbr = INVALID_SECTOR_NBR;
        next_sector_nbr = INVALID_SECTOR_NBR;
    }
}

void UsbhMscReadahead::send_stats() {
    #ifdef USBH_MSC_READAHEAD_STATISTICS
    auto now = osKernelSysTick();
    if (now - sent_statistics_timestamp > 5000) {
        log_info(USBHost, "MSC readahead stat speculative reads=%d, hit=%d, hit_in_progress=%d, missed=%d, blocking=%d",
            stats_speculative_read_count.load(), stats_hit.load(), stats_hit_in_progress.load(),
            stats_missed.load(), stats_block_another_io.load());
        sent_statistics_timestamp = now;
    }
    #endif
}

void UsbhMscReadahead::inc_stats_block_another_io() {
    #ifdef USBH_MSC_READAHEAD_STATISTICS
    stats_block_another_io++;
    #endif
}

#endif // USBH_MSC_READAHEAD
