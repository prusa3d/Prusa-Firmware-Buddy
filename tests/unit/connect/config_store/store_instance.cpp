#include "store_instance.hpp"

namespace {

ConfigStore store;
}

ConfigStore &config_store() {
    return store;
}
