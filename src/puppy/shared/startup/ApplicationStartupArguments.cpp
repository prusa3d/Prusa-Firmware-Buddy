#include "ApplicationStartupArguments.hpp"
#include <option/bootloader.h>
#include "device/board.h"

/// @brief Structure filled by bootloader, has to be placed on specific place
struct __attribute__((packed)) ApplicationStartupArguments {
    uint8_t modbus_address; // address assigned to this puppy
};

struct ApplicationStartupArguments application_startup_arguments __attribute__((__section__(".app_args"), __used__)) = {
    .modbus_address = 0xFF, // this address is not used, section is not inited and bootloader fils data here
};

uint8_t get_assigned_modbus_address() {

#if BOOTLOADER()
    if ((application_startup_arguments.modbus_address < 0x0A) || (application_startup_arguments.modbus_address >= 0x1A)) {
        return 0x00; // Not a valid bootloader address
    }
    // Bootloader gets addresses 0x0A+, modbus uses addresses 0x1A+, add 0x10 to convert from boot address to modbus address
    return (application_startup_arguments.modbus_address + 0x10);
#elif BOARD_IS_MODULARBED()
    // non-bootloader version of puppy will use fixed address
    return 0x1a; // use Modular bed default address
#elif BOARD_IS_DWARF()
    // non-bootloader version of puppy will use fixed address
    return 0x1b; // use DWARF_1 address
#else
    #error Unknown puppy board
#endif
}
