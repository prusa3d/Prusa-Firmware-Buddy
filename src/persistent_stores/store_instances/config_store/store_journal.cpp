/// @file
#include "store_journal.hpp"

ConfigStore &config_store_journal() {
    static constinit ConfigStore instance {};
    return instance;
}
