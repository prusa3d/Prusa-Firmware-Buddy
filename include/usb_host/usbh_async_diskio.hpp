#pragma once

#include <freertos_mutex.hpp>
#include <atomic>

#ifndef UNITTESTS
    #include "usbh_core.h"
    #include "usbh_msc.h"
#endif

#define USBH_MSC_READAHEAD
// #ifdef _DEBUG
#define USBH_MSC_READAHEAD_STATISTICS
// #endif

// Task handle of the process for executing the r/w MSC operations
extern osThreadId USBH_MSC_WorkerTaskHandle;

using UsbhMscRequestCallback = void (*)(USBH_StatusTypeDef result, void *param1, void *param2);

// Structure for describing MSC asynchronous r/w operations
struct UsbhMscRequest {
    enum class UsbhMscRequestOperation : uint8_t {
        Read,
        Write
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

#define USBH_MSC_RW_MAX_DELAY (10000 / portTICK_PERIOD_MS)

#ifdef USBH_MSC_READAHEAD
struct UsbhMscReadahead {
    UsbhMscReadahead() { disable(); }
    /// If the desired sector is already loaded or if it is being read (wait for the operation to complete), return true and copy the sector to data, otherwise return false immediately
    bool get(UsbhMscRequest::LunNbr, UsbhMscRequest::SectorNbr, uint8_t *data);
    /// Set number of last readed sector + 1
    void set_next_sector(UsbhMscRequest::LunNbr, UsbhMscRequest::SectorNbr);
    /// Enable readahead on given LUN
    void enable(UsbhMscRequest::LunNbr);
    /// Disable readahead - initial state
    void disable();
    /// Invalidate preloaded data (in case the sector is being written)
    void invalidate(UsbhMscRequest::LunNbr, UsbhMscRequest::SectorNbr);
    /// Perform a read of the next sector
    void read_next_sector();

private:
    static constexpr const UsbhMscRequest::SectorNbr INVALID_SECTOR_NBR = std::numeric_limits<UsbhMscRequest::SectorNbr>::max();
    static constexpr const UsbhMscRequest::LunNbr INVALID_LUN_NBR = std::numeric_limits<UsbhMscRequest::LunNbr>::max();
    std::atomic<UsbhMscRequest::LunNbr> lun_nbr;
    std::atomic<UsbhMscRequest::SectorNbr> next_sector_nbr;
    std::atomic<UsbhMscRequest::SectorNbr> cached_sector_nbr;
    uint8_t buffer[UsbhMscRequest::SECTOR_SIZE];
    FreeRTOS_Mutex mutex;

    // Statictics
public:
    void send_stats();
    void inc_stats_block_another_io();

private:
    // send statistics to log - not more often than once every 5 seconds
    #ifdef USBH_MSC_READAHEAD_STATISTICS
    void inc_stats_hit() { stats_hit++; }
    void inc_stats_speculative_read_count() { stats_speculative_read_count++; }
    void inc_stats_hit_in_progress() { stats_hit_in_progress++; }
    void inc_stats_missed() { stats_missed++; }
    std::atomic<uint32_t> stats_speculative_read_count = 0;
    std::atomic<uint32_t> stats_hit = 0;
    std::atomic<uint32_t> stats_hit_in_progress = 0;
    std::atomic<uint32_t> stats_missed = 0;
    std::atomic<uint32_t> stats_block_another_io = 0;
    uint32_t sent_statistics_timestamp;
    #else
    void inc_stats_speculative_read_count() {}
    void inc_stats_hit() {}
    void inc_stats_hit_in_progress() {}
    void inc_stats_missed() {}
    #endif // USBH_MSC_READAHEAD_STATISTICS
};

extern UsbhMscReadahead usbh_msc_readahead;

#endif // USBH_MSC_READAHEAD
