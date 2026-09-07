#include "printer_common.hpp"
#include "hostname.hpp"

#include <config_store/store_instance.hpp>

#include <support_utils.h>
#include <version/version.hpp>
#include <bsod/bsod.h>

namespace connect_client {

Printer::Config load_eeprom_config() {
    Printer::Config configuration = {};
    configuration.enabled = config_store().connect_enabled.get();
    // (We need it even if disabled for registration phase)
    strlcpy(configuration.host, config_store().connect_host.get().data(), sizeof(configuration.host));
    decompress_host(configuration.host, sizeof(configuration.host));
    strlcpy(configuration.token, config_store().connect_token.get().data(), sizeof(configuration.token));
    strlcpy(configuration.proxy_host, config_store().connect_proxy_host.get().data(), sizeof(configuration.proxy_host));
    configuration.tls = config_store().connect_tls.get();
    configuration.port = config_store().connect_port.get();
    configuration.proxy_port = config_store().connect_proxy_port.get();
    configuration.custom_cert = config_store().connect_custom_tls_cert.get();

    return configuration;
}

void init_info(Printer::PrinterInfo &info) {
    info.firmware_version = version::project_version_full;
    info.appendix = appendix_exist();
    if (otp_get_serial_nr(info.serial_number) == 0) {
        bsod("otp_get_serial_nr");
    }
    printerHash(info.fingerprint, sizeof(info.fingerprint) - 1, false);
    info.fingerprint[sizeof(info.fingerprint) - 1] = '\0';
}

} // namespace connect_client
