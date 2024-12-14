#include "footer_item_ready_status.hpp"
#include "connect/marlin_printer.hpp"
#include "img_resources.hpp"

FooterItemReadyStatus::FooterItemReadyStatus(window_t *parent)
    : FooterIconText_IntVal(parent, &img::ok_16x16, static_makeView, static_readValue) {
}

int FooterItemReadyStatus::static_readValue() {
    return connect_client::MarlinPrinter::is_printer_ready();
}

string_view_utf8 FooterItemReadyStatus::static_makeView(int value) {
    return string_view_utf8::MakeRAM(value == 1 ? (const uint8_t *)N_("YES") : (const uint8_t *)N_("NO"));
}
