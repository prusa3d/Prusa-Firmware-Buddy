#pragma once

#include "printer.hpp"

namespace connect_client {

Printer::Config load_eeprom_config();
void init_info(Printer::PrinterInfo &info);

} // namespace connect_client
