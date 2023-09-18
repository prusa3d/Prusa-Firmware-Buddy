#include <gcode/gcode.h>

#include "M340.h"
#include <log_dest_syslog.h>
#include <stdint.h>
#include <config_store/store_instance.hpp>

void PrusaGcodeSuite::M340() {
    // Syslog has to be allowed in settings
    const MetricsAllow metrics_allow = config_store().metrics_allow.get();
    if (metrics_allow != MetricsAllow::One && metrics_allow != MetricsAllow::All) {
        SERIAL_ERROR_MSG("Syslog is not allowed!");
        return;
    }

    char ipaddr[config_store_ns::metrics_host_size + 1];
    char format[10]; ///< Format string for sscanf that cannot do string size from parameter
    snprintf(format, std::size(format), "%%%us %%i", std::size(ipaddr) - 1);
    int port;
    int read = sscanf(parser.string_arg, format, ipaddr, &port);
    if (read == 2) {
        // Only one host allowed
        if (metrics_allow == MetricsAllow::One) {
            if (strcmp(ipaddr, config_store().metrics_host.get_c_str()) != 0
                || port != config_store().syslog_port.get()) {
                SERIAL_ERROR_MSG("This is not the one host and port allowed!");
                return;
            }
        }
        syslog_configure(ipaddr, port);
        SERIAL_ECHO_START();
        SERIAL_ECHOLN("Syslog configured successfully");
    } else {
        syslog_configure("", 0);
        SERIAL_ECHO_START();
        SERIAL_ECHOLN("does not match '<address> <port>' pattern; disabling syslog");
    }
}
