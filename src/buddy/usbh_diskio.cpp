#include "ff_gen_drv.h"
#include "usbh_diskio.h"
#include "usbh_async_diskio.hpp"
#include "ccm_thread.hpp"
#include "usb_host.h"

#include <common/freertos_mutex.hpp>
#include <common/freertos_shared_mutex.hpp>
#include <mutex>
#include <shared_mutex>
#include <utility_extensions.hpp>
#include <bit>

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

    if (xQueueSend(request_queue, &request_ptr, USBH_MSC_RW_MAX_DELAY) != pdPASS) {
        return USBH_FAIL;
    }

    xSemaphoreTake(semaphore, portMAX_DELAY);

    return request.result;
}

USBH_StatusTypeDef usbh_msc_submit_request(UsbhMscRequest *request) {
    // we don't have any io scheduler, but if only tasks with the same priority send
    // their requests, they will be distributed fairly
    assert(((DWORD)request->data & 3) == 0);
#ifdef USBH_MSC_READAHEAD
    switch (request->operation) {
    case UsbhMscRequest::UsbhMscRequestOperation::Read:
        usbh_msc_readahead.notify_read(request->lun, request->sector_nbr + request->count);
        break;
    case UsbhMscRequest::UsbhMscRequestOperation::Write:
        usbh_msc_readahead.invalidate(request->lun, request->sector_nbr, request->count);
        break;
    case UsbhMscRequest::UsbhMscRequestOperation::Noop:
        break;
    }
#endif
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
#ifdef USBH_MSC_READAHEAD
        usbh_msc_readahead.send_stats();
        BaseType_t queue_status;
        TickType_t ticks_to_wait = 0;
        do {
            queue_status = xQueueReceive(request_queue, &request, ticks_to_wait);
            if (queue_status != pdPASS) {
                bool preload_performed = usbh_msc_readahead.preload();
                if (preload_performed) {
    #ifdef USBH_MSC_READAHEAD_STATISTICS
                    if (uxQueueMessagesWaiting(request_queue)) {
                        usbh_msc_readahead.inc_stats_block_another_io();
                    }
    #endif
                } else {
                    ticks_to_wait = portMAX_DELAY;
                }
            }
        } while (queue_status != pdPASS);
#else
        BaseType_t queue_status = xQueueReceive(request_queue, &request, portMAX_DELAY);
#endif
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
    static StaticQueue_t queue;
    static uint8_t storage_area[queue_length * sizeof(UsbhMscRequest *)];
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
DRESULT USBH_read_ii(BYTE lun, BYTE *buff, DWORD sector, UINT count) {
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

DRESULT USBH_read(BYTE lun, BYTE *buff, DWORD sector, UINT count) {
#ifdef USBH_MSC_READAHEAD
    assert(count <= 32);
    DRESULT result = RES_OK;
    // use a mask register to indicate which sectors are taken from the cache
    // from the least significant bit (0 means it needs to be loaded)
    uint32_t mask = ~0u << count;
    for (unsigned i = 0; i < count; ++i) {
        if (usbh_msc_readahead.get(lun, sector + i, buff + i * UsbhMscRequest::SECTOR_SIZE)) {
            mask |= 1 << i;
        }
    }
    if (mask != ~0u << count) { // some sectors in the cache
        while (mask != ~0u) {
            int i = std::countr_one(mask);
            result = USBH_read_ii(lun, buff + i * UsbhMscRequest::SECTOR_SIZE, sector + i, 1);
            if (result != RES_OK) {
                return result;
            }
            mask |= 1 << i;
        }
    } else { // no cache match => read whole buffer at once
        result = USBH_read_ii(lun, buff, sector, count);
    }
    #ifdef USBH_MSC_READAHEAD
    usbh_msc_readahead.notify_read(lun, sector + count);
    #endif

    return result;
#else
    return USBH_read_ii(lun, buff, sector, count);
#endif
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
    usbh_msc_readahead.invalidate(lun, sector, count);
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

#ifdef USBH_MSC_READAHEAD

bool UsbhMscReadahead::get(UsbhMscRequest::LunNbr lun_nbr, UsbhMscRequest::SectorNbr sector_nbr, uint8_t *data) {
    if (this->lun_nbr != lun_nbr) {
        return false;
    }

    bool found = false;
    {
        Lock lock(mutex);
        auto entry = std::find(begin(cache), end(cache), sector_nbr);

        if (entry != end(cache)) {
            Lock lock2(entry->mutex);
            if (entry->sector_nbr == sector_nbr) {
                entry->timestamp = 0;
                if (entry->preloaded) {
                    found = true;
                    memcpy(data, entry->data, UsbhMscRequest::SECTOR_SIZE);
                } else {
                    entry->reset();
                    entry->sector_nbr = INVALID_SECTOR_NBR;
                }
            }
        }
    }
    if (found) {
        inc_stats_hit();
        xTaskNotifyGive(USBH_MSC_WorkerTaskHandle);
        return true;
    } else {
        inc_stats_missed();
        return false;
    }
}

// typical fatfs read requests are 1kb so it's a good idea to preload 2 sectors
void UsbhMscReadahead::notify_read(UsbhMscRequest::LunNbr lun_nbr, UsbhMscRequest::SectorNbr sector_nbr) {
    Lock lock(mutex);
    if (this->lun_nbr != lun_nbr) {
        return;
    }
    static_assert(size >= 2);
    // find the oldest and second oldest slots
    auto it = begin(cache);
    auto first = it++;
    auto second = it++;
    if (*second < *first) {
        std::swap(first, second);
    }

    for (; it != end(cache); ++it) {
        if (*it < *first) {
            second = first;
            first = it;
        } else if (*it < *second) {
            second = it;
        }
    }

    auto plan = [](auto it, auto sector_nbr) {
        Lock locks(it->mutex);
        it->sector_nbr = sector_nbr;
        it->preloaded = false;
        it->timestamp = osKernelSysTick();
    };
    if (std::find(begin(cache), end(cache), sector_nbr) == end(cache)) {
        plan(first, sector_nbr);
    } else {
        second = first;
    }
    sector_nbr++;
    if (std::find(begin(cache), end(cache), sector_nbr) == end(cache)) {
        plan(second, sector_nbr);
    }
    // wake up the USBH_worker task to perform another readahead
    static UsbhMscRequest noop = {
        .operation = UsbhMscRequest::UsbhMscRequestOperation::Noop,
        .lun = 0,
        .count = 0,
        .sector_nbr = 0,
        .data = 0,
        .result = USBH_OK,
        .callback = 0,
        .callback_param1 = 0,
        .callback_param2 = 0
    };
    static UsbhMscRequest *noop_ptr = &noop;
    xQueueSend(request_queue, &noop_ptr, 0);
}

void UsbhMscReadahead::enable(UsbhMscRequest::LunNbr lun_nbr) {
    this->lun_nbr = lun_nbr;
}

void UsbhMscReadahead::disable() {
    Lock lock(mutex);
    reset();
    this->lun_nbr = INVALID_LUN_NBR;
}

void UsbhMscReadahead::invalidate(UsbhMscRequest::LunNbr lun_nbr, UsbhMscRequest::SectorNbr sector_nbr, size_t count) {
    Lock lock(mutex);
    if (this->lun_nbr != lun_nbr) {
        return;
    }
    for (auto &e : cache) {
        if (e.sector_nbr >= sector_nbr && e.sector_nbr < sector_nbr + count) {
            Lock lock2(e.mutex);
            e.reset();
        }
    }
}

bool UsbhMscReadahead::preload() {
    decltype(cache)::iterator entry;
    {
        Lock lock(mutex);

        if (this->lun_nbr == INVALID_LUN_NBR) {
            return false;
        }
        entry = std::find_if(begin(cache), end(cache),
            [](const auto &e) {
                return e.preloaded == false && e.sector_nbr != INVALID_SECTOR_NBR;
            });

        if (entry == end(cache)) {
            return false;
        }
    }
    {
    #ifdef USBH_MSC_READAHEAD_STATISTICS
        stats_speculative_read_count++;
    #endif
        {
            Lock lock(entry->mutex);
            if (entry->preloaded == false && entry->sector_nbr != INVALID_SECTOR_NBR) {
                auto result = USBH_MSC_Read(&hUsbHostHS, lun_nbr, entry->sector_nbr, entry->data, 1);
                if (result == USBH_OK) {
                    entry->preloaded = true;
                } else {
                    entry->reset();
                }
            }
        }
        return true;
    }
}

void UsbhMscReadahead::reset() {
    for (auto &e : cache) {
        e.reset();
    }
}

void UsbhMscReadahead::send_stats() {
    #ifdef USBH_MSC_READAHEAD_STATISTICS
    auto now = osKernelSysTick();
    if (now - sent_statistics_timestamp > 5000) {
        log_info(USBHost, "MSC readahead stat speculative reads=%" PRIu32 ", hit=%" PRIu32 ", missed=%" PRIu32 ", blocking=%" PRIu32,
            stats_speculative_read_count.load(), stats_hit.load(), stats_missed.load(), stats_block_another_io.load());
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
