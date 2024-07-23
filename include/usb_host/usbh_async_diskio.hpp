#pragma once

#include <freertos/mutex.hpp>
#include <atomic>
#include <limits>
#include <printers.h>

#ifndef UNITTESTS
    #include "usbh_core.h"
    #include "usbh_msc.h"
#endif

#if !PRINTER_IS_PRUSA_MINI() /* MINI doesn't have enough RAM, sorry MINI */
    #define USBH_MSC_READAHEAD
    // #ifdef _DEBUG
    #define USBH_MSC_READAHEAD_STATISTICS
// #endif
#endif /* !PRINTER_IS_PRUSA_MINI() */

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

#ifdef USBH_MSC_READAHEAD
struct UsbhMscReadahead {
    static constexpr uint8_t size = 4;

    UsbhMscReadahead() { disable(); }
    /// If the desired sector is already loaded or if it is being read (wait for the operation to complete), return true and copy the sector to data, otherwise return false immediately
    bool get(UsbhMscRequest::LunNbr, UsbhMscRequest::SectorNbr, uint8_t *data);
    /// Notify number of last readed sector
    void notify_read(UsbhMscRequest::LunNbr, UsbhMscRequest::SectorNbr);
    /// Enable readahead on given LUN
    void enable(UsbhMscRequest::LunNbr);
    /// Disable readahead - initial state
    void disable();
    /// Invalidate preloaded data (in case the sectors are being written)
    void invalidate(UsbhMscRequest::LunNbr, UsbhMscRequest::SectorNbr, size_t count);
    /// Perform a read of the next sector, returns true if preload was performed
    bool preload();

private:
    void reset();

    static constexpr UsbhMscRequest::SectorNbr INVALID_SECTOR_NBR = std::numeric_limits<UsbhMscRequest::SectorNbr>::max();
    static constexpr UsbhMscRequest::LunNbr INVALID_LUN_NBR = std::numeric_limits<UsbhMscRequest::LunNbr>::max();
    std::atomic<UsbhMscRequest::LunNbr> lun_nbr;

    struct Entry {
        std::atomic<UsbhMscRequest::SectorNbr> sector_nbr;
        bool preloaded;
        uint32_t timestamp;
        freertos::Mutex mutex;
        uint8_t data[UsbhMscRequest::SECTOR_SIZE];

        void reset() {
            sector_nbr = INVALID_SECTOR_NBR;
            preloaded = false;
            timestamp = 0;
        }
        bool operator==(UsbhMscRequest::SectorNbr sector_nbr) { return this->sector_nbr == sector_nbr; };
        bool operator<(const Entry &e) { return timestamp < e.timestamp; }
    };

    freertos::Mutex mutex;
    std::array<Entry, size> cache;

    // Statictics
public:
    void send_stats();
    void inc_stats_block_another_io();

private:
    // send statistics to log - not more often than once every 5 seconds
    #ifdef USBH_MSC_READAHEAD_STATISTICS
    void inc_stats_hit() { stats_hit++; }
    void inc_stats_speculative_read_count() { stats_speculative_read_count++; }
    void inc_stats_missed() { stats_missed++; }
    std::atomic<uint32_t> stats_speculative_read_count = 0;
    std::atomic<uint32_t> stats_hit = 0;
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
