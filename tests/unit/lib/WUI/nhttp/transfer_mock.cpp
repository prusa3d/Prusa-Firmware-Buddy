#include "transfers/transfer.hpp"
#include <sys/stat.h>

using namespace transfers;

std::optional<struct stat> Transfer::get_transfer_partial_file_stat(MutablePath &destination_path) {
    struct stat st = {};
    return st;
}

bool Transfer::is_valid_transfer(MutablePath &destination_path) {
    return true;
}
