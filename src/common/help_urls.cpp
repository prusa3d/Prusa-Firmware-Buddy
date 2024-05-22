#include <common/help_urls.hpp>

#include <config_store/store_instance.hpp>

const char *get_printer_help_url() {
#if HAS_EXTENDED_PRINTER_TYPE()
    return printer_help_url[config_store().extended_printer_type.get()];
#else
    return printer_help_url;
#endif
}
