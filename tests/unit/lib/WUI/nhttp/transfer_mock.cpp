#include "transfers/transfer.hpp"
#include <transfers/transfer_file_check.hpp>
#include <sys/stat.h>

using namespace transfers;

std::optional<struct stat> Transfer::get_transfer_partial_file_stat(MutablePath &destination_path) {
    struct stat st = {};
    return st;
}

namespace transfers {
bool is_valid_transfer(const MutablePath &destination_path) {
    return true;
}
} // namespace transfers
