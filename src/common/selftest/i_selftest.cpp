// selftest.cpp

#include "i_selftest.hpp"
#include "stdarg.h"
#include "log.h"
#include "app.h"
#include "otp.h"
#include "hwio.h"
#include "marlin_server.hpp"
#include "wizard_config.hpp"
#include "filament_sensors_handler.hpp"

LOG_COMPONENT_DEF(Selftest, LOG_SEVERITY_DEBUG);

ISelftest::ISelftest()
    : m_Time(0)
    , m_USBLog_fp(NULL) {
}

void ISelftest::phaseStart() {
    FSensors_instance().IncEvLock(); // block autoload and M600
    marlin_server::set_exclusive_mode(1);
    FSM_CREATE__LOGGING(Selftest); // TODO data 0/1 selftest/wizard
    log_open();
}

void ISelftest::phaseFinish() {
    log_close();
    FSM_DESTROY__LOGGING(Selftest);
    marlin_server::set_exclusive_mode(0);
    FSensors_instance().DecEvLock(); // stop blocking autoload and M600
}

bool ISelftest::phaseWait() {
    static uint32_t tick = 0;
    if (tick == 0) {
        tick = m_Time;
        return true;
    } else if ((m_Time - tick) < 2000) {
        return true;
    }
    tick = 0;
    return false;
}

void ISelftest::log_open() {
#ifndef _DEBUG
    return; // Enabling USB logs only in debug builds
#endif      // _DEBUG

    const char *suffix = get_log_suffix();
    char fname[64];
    serial_nr_t sn;
    uint8_t sn_length = otp_get_serial_nr(&sn);
    static const char unk[] = "unknown";
    char const *serial = sn_length != 0 ? sn.txt : unk;
    snprintf(fname, sizeof(fname), "/usb/test_%s%s.txt", serial, suffix);

    m_USBLog_fp = fopen(fname, "w+");
    if (m_USBLog_fp) {
        log_printf("SELFTEST START\n");
        log_printf("printer serial: %s\n\n", serial);
    }
}

void ISelftest::log_close() {
    if (!m_USBLog_fp) {
        return;
    }
    log_printf("SELFTEST END\n");
    fclose(m_USBLog_fp);
    m_USBLog_fp = NULL;
}

void ISelftest::log_printf(const char *fmt, ...) {
    if (!m_USBLog_fp) {
        return;
    }
    char line[SELFTEST_MAX_LOG_PRINTF];
    va_list va;
    va_start(va, fmt);
    int str_len = vsnprintf(line, SELFTEST_MAX_LOG_PRINTF, fmt, va);
    va_end(va);

    while (str_len > 0) {
        int chunk_size = fprintf(m_USBLog_fp, line);
        if (chunk_size < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            continue;
        }
        if (chunk_size < 0) {
            return;
        }
        str_len -= chunk_size;
    }
    return;
}

bool ISelftest::abort_part(selftest::IPartHandler **pppart) {
    if (pppart && (*pppart)) {
        (*pppart)->Abort();
        delete *pppart;
        *pppart = nullptr;
        return true;
    }
    return false;
}
