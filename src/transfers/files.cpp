#include "files.hpp"

#include <atomic>
#include <cstdio>
#include <sys/statvfs.h>
#include <unistd.h>

using std::atomic;
using std::memory_order_relaxed;

namespace transfers {

namespace {

    atomic<size_t> transfer_idx = 0;

    static const char *const TRANSFER_TEMPLATE = "/usb/%zu.tmp";

} // namespace

size_t next_transfer_idx() {
    return transfer_idx.fetch_add(1, memory_order_relaxed);
}

TransferName transfer_name(size_t idx) {
    TransferName result = {};
    snprintf(result.begin(), result.size(), TRANSFER_TEMPLATE, idx);
    return result;
}

PreallocateResult file_preallocate(const char *fname, size_t size) {
    unique_file_ptr file(fopen(fname, "wb"));
    // We assume the fname is valid transfer name ‒ eg /usb/XXX.tmp
    if (!file) {
        return "Missing USB drive";
    }

    struct statvfs svfs;
    if (statvfs(fname, &svfs) || (svfs.f_bavail * svfs.f_frsize * svfs.f_bsize) < size) {
        file.reset();
        remove(fname);
        return "USB drive full";
    }
    const size_t csize = svfs.f_frsize * svfs.f_bsize;
    /* Pre-allocate the area needed for the file to prevent frequent metadata updates during the transfer.
       We do the pre-allocation incrementally cluster-by-cluster to minimize the chance of stalling
       the media and blocking other tasks potentially waiting for the file system lock.
       The file size doesn't need to be exactly the size of the uploaded content. We will truncate
       the file later on. */
    for (unsigned long sz = csize; sz < size; sz += csize) {
        if (fseek(file.get(), sz, SEEK_SET)) {
            file.reset();
            remove(fname);
            return "USB drive full";
        }
    }
    fsync(fileno(file.get()));
    fseek(file.get(), 0, SEEK_SET);

    return file;
}

bool write_block(FILE *f, const uint8_t *data, size_t size) {
    while (size > 0) {
        errno = 0; // Make sure there are no leftovers in successful-but-eof cases
        const size_t written = fwrite(data, 1, size, f);
        if (written == size) {
            return true;
        } else {
            if (errno == EAGAIN || errno == EBUSY) {
                // We have written possibly something, retry with the rest
                data += written;
                size -= written;
            } else {
                // Some other (unrecoverable?) error
                return false;
            }
        }
    }

    return true;
}

} // namespace transfers
