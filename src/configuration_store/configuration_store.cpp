#include "configuration_store.hpp"
#include "eeprom.h"

namespace Journal {
void migrate() {
    // load data from old eeprom to buffer load the data to item
    std::array<uint8_t, 512> buffer { 1, 2, 3 };

    bool fs_enabled = eeprom_get_bool(EEVAR_FAN_CHECK_ENABLED);
    memcpy(buffer.data(), std::launder(&fs_enabled), sizeof(fs_enabled));
    config_store().load_item(hash("Fan check"), buffer, sizeof(fs_enabled));
}

}
void init_stores() {
    st25dv64k_init();
    // called to call the constructor of configuration store
    config_store();
    eeprom_init();
    //    if (status == EEPROM_INIT_Normal || status == EEPROM_INIT_Upgraded) {
    //        Journal::migrate();
    //        config_store().init();
    //        config_store().save_all();
    //    } else {
    //        config_store().init();
    //        config_store().load_all();
    //    }
}
