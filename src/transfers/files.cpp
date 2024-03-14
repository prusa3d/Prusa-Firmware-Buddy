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

} // namespace transfers
