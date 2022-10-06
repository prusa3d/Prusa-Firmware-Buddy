#include "configuration_store.hpp"
using namespace configuration_store;

ConfigurationStore<ConfigurationStoreStructure> &store() {
    return ConfigurationStore<ConfigurationStoreStructure>::GetStore();
}
