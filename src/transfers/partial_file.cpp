#include "partial_file.hpp"

#include <buddy/fatfs.h>
#include <buddy/filesystem_fatfs.h>
#include <common/unique_file_ptr.hpp>
#include <common/bsod.h>
#include <logging/log.h>

#include <algorithm>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

LOG_COMPONENT_REF(transfers);
using namespace transfers;
using std::holds_alternative;
using std::make_tuple;
using std::unique_lock;
using std::variant;

static_assert(PartialFile::SECTOR_SIZE == FF_MAX_SS);
static_assert(PartialFile::SECTOR_SIZE == FF_MIN_SS);

PartialFile::PartialFile(UsbhMscRequest::LunNbr lun, UsbhMscRequest::SectorNbr first_sector, State state, int file_lock)
    : sector_pool(lun, usb_msc_write_finished_callback, this)
    , write_error(false)
    , first_sector_nbr(first_sector)
    , current_sector(nullptr)
    , current_offset(0)
    , state(state)
    , last_progress_percent(-1)
    , file_lock(file_lock) {
}

PartialFile::~PartialFile() {
    // the current sector may contain incomplete content, so we must avoid overwriting potentially valid data
    discard_current_sector();
    // synchronization is required due to the validity of callback pointers
    sector_pool.sync(0, true);
    if (file_lock != -1) {
        close(file_lock);
    }
}

variant<const char *, PartialFile::Ptr> PartialFile::create(const char *path, size_t size) {
    unique_file_ptr file(fopen(path, "wb"));

    if (!file) {
        log_error(transfers, "Failed to open file %d", errno);
        return "Failed to write to location";
    }

    // we want to allocate contiguous space on the drive
    // so lets get a bit dirty and go one level lower
    FIL *fatfs_file = filesystem_fastfs_get_underlying_struct(file.get());
    if (!fatfs_file) {
        file.reset();
        remove(path);
        return "Failed to prepare file for writing";
    }

    // expand the file size
    auto alloc_result = f_expand(fatfs_file, size, /*allocate_now=*/1, /*yield=*/1);
    if (alloc_result != FR_OK) {
        file.reset();
        remove(path);
        return "USB drive full";
    }

    return PartialFile::convert(path, std::move(file), State());
}

variant<const char *, PartialFile::Ptr> PartialFile::open(const char *path, State state, bool ignore_opened) {
    unique_file_ptr file(fopen(path, ignore_opened ? "rb" : "rb+"));

    if (!file) {
        return "Failed to open file";
    }

    return PartialFile::convert(path, std::move(file), state);
}

variant<const char *, PartialFile::Ptr> PartialFile::convert(const char *path, unique_file_ptr file, State state) {
    FIL *fatfs_file = filesystem_fastfs_get_underlying_struct(file.get());
    if (!fatfs_file) {
        return "Failed to open file";
    }

    // check file contiguity
    int is_contiguous = 0;
    auto result = fatfs_test_contiguous_file(fatfs_file, &is_contiguous);
    if (result != FR_OK) {
        return "Failed to check file contiguity";
    }
    if (!is_contiguous) {
        return "File is not contiguous";
    }

    state.total_size = f_size(fatfs_file);

    // get first sector
    auto drive = fatfs_file->obj.fs->pdrv;
    auto lba = fatfs_file->obj.fs->database + fatfs_file->obj.fs->csize * (fatfs_file->obj.sclust - 2);

    // We want to keep a *read only* file open for our lifetime to prevent
    // someone from deleting it (and us then writing into sectors no longer
    // allocated for the file and other funny things).
    //
    // For that we first have to *close* the read-write/write file to get it
    // (and we want only a file descriptor, not FILE *). Yes, there's short a
    // race condition there when someone could delete the file and create a new
    // one with the same name but different sectors between we close & open,
    // but it's still better than not having the file lock at all.
    file.reset();
    int fd = ::open(path, O_RDONLY);
    if (fd == -1) {
        return "Can't lock file in place";
    }

    return std::make_shared<PartialFile>(drive, lba, state, fd);
}

UsbhMscRequest::SectorNbr PartialFile::get_sector_nbr(size_t offset) {
    auto sector = first_sector_nbr + offset / FF_MAX_SS;
    // total_size won't change, don't have to lock
    if (offset >= state.total_size) {
        sector += 1;
    }
    return sector;
}

size_t PartialFile::get_offset(UsbhMscRequest::SectorNbr sector_nbr) {
    return (sector_nbr - first_sector_nbr) * SECTOR_SIZE;
}

bool PartialFile::write_current_sector() {
    assert(current_sector != nullptr);
    log_debug(transfers, "Sending sector over USB %d (%.20s)", current_sector->sector_nbr, current_sector->data);
    if (lseek(file_lock, 0, SEEK_SET) == -1) {
        // Safety measure. It is possible that between creation of this
        // PartialFile and current call to write_sector, the USB got unplugged
        // and some other got plugged in. This would have severe effects on the
        // filesystem, as we bypass the filesystem here and just send the data
        // to specific offset.
        //
        // The "usual" file descriptors are already hooked up to this mechanism
        // to protect them (hopefully), so we simply abuse that mechanism by
        // "poking" the file descriptor for this given file. We use lseek as
        // hopefully cheap way to "poke" it. We use the 'rewind' mode, the
        // 'ftell' mode has a shortcut in it and actually does _not_ check the
        // validity of the file.
        //
        // FIXME: This actually can block! It probably doesn't "do" anything
        // with the USB itself, but acqires some lock and it takes a long time
        // to do so.
        return false;
    }
    const uint32_t slot_idx = reinterpret_cast<uint32_t>(current_sector->callback_param2);
    auto start = get_offset(current_sector->sector_nbr);
    // total_size wont change, don't have to lock
    auto end = std::min(start + SECTOR_SIZE, state.total_size);
    // Synchronized through release-acquire pair through the queue into USB thread
    future_extend[slot_idx] = ValidPart { start, end };
    // This _in theory_ can block, which we don't like. But, as the queue for
    // the requests is currently 5 long, we can put at most 2 there, there can
    // be other 3 threads doing something with the USB at the same time and
    // still not block... unlikely.
    //
    // So we don't complicate the things and keep it this way (at worst, we
    // will stall tcpip thread for a short bit).
    auto result = usbh_msc_submit_request(current_sector);
    if (result != USBH_OK) {
        return false;
    }
    return true;
}

bool PartialFile::seek(size_t offset) {
    auto new_sector = get_sector_nbr(offset);

    if (current_sector && current_sector->sector_nbr == new_sector) {
        current_offset = offset;
        return true;
    }

    if (current_sector && current_sector->sector_nbr != new_sector) {
        log_warning(transfers, "Discarding buffered data for sector %d", current_sector->sector_nbr);
    }

    current_offset = offset;
    discard_current_sector();
    return true;
}

void PartialFile::discard_current_sector() {
    if (current_sector) {
        sector_pool.release(reinterpret_cast<uint32_t>(current_sector->callback_param2));
        current_sector = nullptr;
    }
}

PartialFile::BufferPeek PartialFile::get_current_buffer(bool block_waiting) {
    if (!current_sector) {
        if (current_offset >= state.total_size) {
            return OutOfRange {};
        }
        const auto sector_nbr = get_sector_nbr(current_offset);

        current_sector = sector_pool.acquire(block_waiting);
        if (current_sector == nullptr) {
            if (block_waiting) {
                return WriteError {};
            } else {
                return WouldBlock {};
            }
        }
        current_sector->sector_nbr = sector_nbr;
    }

    const size_t sector_offset = current_offset % SECTOR_SIZE;
    return make_tuple(current_sector->data, sector_offset);
}

bool PartialFile::advance_written(size_t by) {
    assert(current_sector);
    const auto next_offset = current_offset + by;
    const auto next_sector_nbr = get_sector_nbr(next_offset);
    if (next_offset > state.total_size) {
        fatal_error("Request to write past the end of file.", "transfers");
    }
    if (next_sector_nbr != current_sector->sector_nbr) {
        // TODO: We may need some non-blocking way?
        if (write_current_sector()) {
            current_sector = nullptr;
        } else {
            return false;
        }
    }

    current_offset = next_offset;
    return true;
}

bool PartialFile::write(const uint8_t *data, size_t size) {
    if (write_error) {
        return false;
    }
    while (size) {
        auto buffer = get_current_buffer(true);

        if (holds_alternative<WouldBlock>(buffer)) {
            // We ask for blocking mode
            assert(0);
            return false;
        } else if (holds_alternative<WriteError>(buffer)) {
            return false;
        } else if (holds_alternative<OutOfRange>(buffer)) {
            log_error(transfers, "Write past end of file attempted");
            return false;
        }
        // else -> we got a buffer
        assert(holds_alternative<BufferAndOffset>(buffer));
        auto [buff_ptr, offset] = get<BufferAndOffset>(buffer);

        const size_t sector_remaining = SECTOR_SIZE - offset;
        const size_t write_size = std::min(size, sector_remaining);
        assert(sector_remaining > 0);
        assert(write_size > 0);
        memcpy(buff_ptr + offset, data, write_size);

        if (!advance_written(write_size)) {
            return false;
        }

        data += write_size;
        size -= write_size;
    }

    return true;
}

bool PartialFile::sync() {
    uint32_t sync_avoid = 0;
    if (current_sector) {
        sync_avoid = 1;
        auto copied_sector = sector_pool.acquire(true);
        if (!copied_sector) {
            return false;
        }
        memcpy(copied_sector->data, current_sector->data, SECTOR_SIZE);
        copied_sector->sector_nbr = current_sector->sector_nbr;
        auto status = write_current_sector();
        if (status) {
            current_sector = copied_sector;
        } else {
            sector_pool.release(reinterpret_cast<uint32_t>(copied_sector->callback_param2));
            log_error(transfers, "Failed to write sector");
            return false;
        }
    }
    if (!sector_pool.sync(sync_avoid, false)) {
        return false;
    }
    return !write_error;
}

void PartialFile::extend_valid_part(ValidPart new_part) {
    unique_lock lock(state_mutex);
    // extend head
    if (state.valid_head) {
        state.valid_head->merge(new_part);
    } else if (!state.valid_head && new_part.start == 0) {
        state.valid_head = new_part;
    }
    auto head_end = state.valid_head ? state.valid_head->end : 0;

    // extend tail
    if (state.valid_tail) {
        state.valid_tail->merge(new_part);
    } else if (!state.valid_tail && new_part.start > head_end) {
        state.valid_tail = new_part;
    }

    // does head spreads to the end?
    if (state.valid_head && state.valid_head->end == state.total_size) {
        state.valid_tail = state.valid_head;
    }

    // head met with tail?
    if (state.valid_head && state.valid_tail) {
        state.valid_head->merge(*state.valid_tail);
        state.valid_tail->merge(*state.valid_head);
    }

    // report print progress
    int percent_valid = state.get_percent_valid();
    if (percent_valid != last_progress_percent) {
        print_progress();
        last_progress_percent = percent_valid;
    }
}

bool PartialFile::has_valid_head(size_t bytes) const {
    auto st = get_state();
    return st.valid_head && st.valid_head->start == 0 && st.valid_head->end >= bytes;
}

bool PartialFile::has_valid_tail(size_t bytes) const {
    auto st = get_state();
    return st.valid_tail && st.valid_tail->start <= (st.total_size - bytes) && st.valid_tail->end == st.total_size;
}

void PartialFile::print_progress() {
    /*
     * FIXME: This overflows the AsyncIO stack as it sends the large message over UDP from that particular thread.
     *
     * While the stack size was already increased because of other logs, we
     * don't want to go even further for this message.
     */
#if 0
    // Note: we are accessing state here directly. We are being called from places that are already locked.
    std::array<char, 40> progress;
    for (auto &c : progress) {
        c = '-';
    }
    float progress_size = progress.size();
    float head_end = state.valid_head ? state.valid_head->end : 0;
    size_t file_size = state.total_size;
    float tail_start = state.valid_tail ? state.valid_tail->start : file_size;
    float head_progress = head_end * progress_size / static_cast<float>(file_size);
    for (size_t i = 0; i < head_progress; i++) {
        progress[i] = '#';
    }
    float tail_progress = (file_size - tail_start) * progress_size / static_cast<float>(file_size);
    for (size_t i = 0; i < tail_progress; i++) {
        progress[progress.size() - 1 - i] = '#';
    }

    int percent = state.get_percent_valid();

    log_info(transfers, "Progress: %.40s  %i%%", progress.data(), percent);
#endif
}

PartialFile::SectorPool::SectorPool(UsbhMscRequest::LunNbr lun, UsbhMscRequestCallback callback, void *callback_param)
    : semaphore(xSemaphoreCreateCounting(size, size))
    , slot_mask(~0u << size) {
    for (unsigned i = 0; i < size; ++i) {
        pool[i] = {
            UsbhMscRequest::UsbhMscRequestOperation::Write,
            lun,
            1,
            0,
            new uint8_t[SECTOR_SIZE],
            USBH_FAIL,
            callback,
            callback_param,
            reinterpret_cast<void *>(i)
        };
    }
}

PartialFile::SectorPool::~SectorPool() {
    sync(0, true);
    for (unsigned i = 0; i < size; ++i) {
        delete[] pool[i].data;
    }
    vSemaphoreDelete(semaphore);
}

UsbhMscRequest *PartialFile::SectorPool::acquire(bool block_waiting) {
    auto result = xSemaphoreTake(semaphore, block_waiting ? USBH_MSC_RW_MAX_DELAY : 0);
    if (result != pdPASS) {
        return nullptr;
    }

    mutex.lock();
    auto slot = get_available_slot();
    slot_mask |= 1 << slot;
    mutex.unlock();

    memset(pool[slot].data, 0, SECTOR_SIZE);
    return pool + slot;
}

void PartialFile::SectorPool::release(uint32_t slot) {
    mutex.lock();
    slot_mask &= ~(1 << slot);
    xSemaphoreGive(semaphore);
    mutex.unlock();
}

bool PartialFile::SectorPool::sync(uint32_t avoid, bool force) {
    assert(avoid <= size);

    // We flush the whole queue by blocking all the slots for ourselves before
    // returning them back. When we have them all, none can be in flight.
    uint32_t block = size - avoid;
    uint32_t acquired = 0; // How many were actually acquired (in case of timeouts, it can be less)
    for (uint32_t i = 0; i < block; i++) {
        if (xSemaphoreTake(semaphore, force ? portMAX_DELAY : USBH_MSC_RW_MAX_DELAY) == pdPASS) {
            acquired++;
        } else {
            break;
        }
    }
    for (uint32_t i = 0; i < acquired; i++) {
        xSemaphoreGive(semaphore);
    }

    return block == acquired;
}

void PartialFile::release_file() {
    // Flush everything so we won't try writing through this later.
    discard_current_sector();
    sector_pool.sync(0, true);
    // Prevent any future attempts to write here.
    write_error = true;
    // And release the lock.
    close(file_lock);
    file_lock = -1;
}

uint32_t PartialFile::SectorPool::get_available_slot() const {
    assert(is_available_slot());
    return std::countr_one(slot_mask);
}

void PartialFile::usbh_msc_finished(USBH_StatusTypeDef result, uint32_t slot) {
    if (result == USBH_OK && !write_error) {
        // Still safe, slot can't be reused before release below, so one can't take this until then.
        extend_valid_part(future_extend[slot]);
    } else {
        log_error(transfers, "Failed to write sector");
        write_error = true;
    }
    sector_pool.release(slot);

    // Call the callback. Make sure this is _after_ we released the slot, so it
    // can be reused from there.
    if (written_callback != nullptr) {
        written_callback(written_callback_arg);
    }
}

void PartialFile::usb_msc_write_finished_callback(USBH_StatusTypeDef result, void *param1, void *param2) {
    reinterpret_cast<PartialFile *>(param1)->usbh_msc_finished(result, reinterpret_cast<uint32_t>(param2));
}

PartialFile::State PartialFile::get_state() const {
    unique_lock lock(state_mutex);

    return state;
}
