#include "transfers/partial_file.hpp"

using namespace transfers;

std::variant<const char *, PartialFile::Ptr> PartialFile::open(const char *, PartialFile::State) {
    return "not implemented";
}
