#include "nfc.hpp"

#include "st25dv64k.h"
#include "st25dv64k_internal.h"

// ntohs
#ifdef UNITTESTS
    #include <arpa/inet.h>
#else
    #include <lwip/def.h>
#endif

#include <str_utils.hpp>
#include <string.h>
#include <tuple>

namespace nfc {

struct __attribute__((packed)) CapabilityContainer {
    uint8_t magic; // 0xE1
    uint8_t version_and_access;
    uint8_t mlen; // (accessible_bytes - sizeof(CapabilityContainer)) / 8
    uint8_t features;
};
static_assert(sizeof(CapabilityContainer) == 4);

enum {
    field_falling = (0x1 << 3),
    field_rising = (0x1 << 4),
};

namespace ndef {

    enum Type {
        tlv = 0x3,
        terminator = 0xfe,
    };

    struct __attribute__((packed)) TLVHeaderShort {
        uint8_t type;
        uint8_t length; // << 0xFF !!
    };

    struct __attribute__((packed)) TLVHeaderLong {
        uint8_t type;
        uint8_t length1; // == 0xFF !!
        uint16_t length2; // 0x00FF - 0xFFFE
    };

    enum {
        tlv_long_header = 0xff,
    };

    struct __attribute__((packed)) MessageRecordNoId {
        uint8_t header;
        uint8_t type_length;
        uint8_t payload_length;
    };

    struct __attribute__((packed)) MessageRecordID {
        uint8_t header;
        uint8_t type_length;
        uint8_t payload_length;
        uint8_t id_length;
    };

    enum Message_Record_Header_Bits {
        type_field = 0b111,
        media_type = 0x2,
        id_length_flag = 0x1 << 3,
        short_record = 0x1 << 4,
        chunk_flag = 0x1 << 5,
        message_end = 0x1 << 6,
        message_begin = 0x1 << 7,
    };

    struct WifiValueHeader {
        uint16_t type;
        uint16_t length;
    };

    enum {
        wifi_value_type_credentials = 0x100E,
        wifi_value_type_ssid = 0x1045,
        wifi_value_type_password = 0x1027,
        // Wifi_Value_Type_Auth_Type = 0x1003,
        // Wifi_Value_Type_Encryption_Type = 0x100F,
    };

} // namespace ndef

uint8_t old_state { 0 };

void init_printer_id() {
    // TXT info "PRUSA3D/MK4S"
    // generated using: ndeftool txt -l en "PRUSA3D/MK4S" | xxd -i
    constexpr const char data[] = {
        // ndef::Message_Record_No_Id
        0xd1, 0x01, 0x0f,
        // TXT type
        0x54,
        // TXT payload
        0x02, 0x65, 0x6e, 0x50, 0x52, 0x55, 0x53, 0x41, 0x33, 0x44, 0x2f, 0x4d, 0x4b, 0x34, 0x53
    };

    uint16_t dst_addr = sizeof(nfc::CapabilityContainer);

    ndef::TLVHeaderShort tlv {
        .type = ndef::Type::tlv, // NDEF Message TLV
        .length = sizeof(data)
    };

    std::ignore = user_write_bytes(EepromCommand::memory, dst_addr, &tlv, sizeof(tlv));
    dst_addr += sizeof(tlv);

    std::ignore = user_write_bytes(EepromCommand::memory, dst_addr, data, sizeof(data));
    dst_addr += sizeof(data);

    tlv = {
        .type = ndef::Type::terminator, // terminator
        .length = sizeof(data)
    };

    std::ignore = user_write_bytes(EepromCommand::memory, dst_addr, &tlv, sizeof(tlv));
    dst_addr += sizeof(tlv);

    // overwrite possible previous credentials,
    // the 128 is not a strict limit, there is 0x3ff bytes for NFC
    for (uint32_t zero { 0 }; dst_addr < 128; dst_addr += sizeof(zero)) {
        // intentionally writing small data; larger blocks could fail
        std::ignore = user_write_bytes(EepromCommand::memory, dst_addr, &zero, sizeof(zero));
    }
}

std::optional<WifiCredentials> try_parse_wifi_message(uint16_t from, const uint8_t type_length, const uint8_t payload_length, const uint8_t id_length) {
    WifiCredentials credentials;
    memset(&credentials, 0, sizeof(credentials));

    {
        // temporaryly use the password field as a buffer
        auto &buffer = credentials.password;

        constexpr const char WIFI_MIME_TYPE[] = "application/vnd.wfa.wsc";
        static_assert(sizeof(buffer) > strlen_constexpr(WIFI_MIME_TYPE));

        const auto result = user_read_bytes(
            EepromCommand::memory,
            from,
            buffer.data(),
            std::min(static_cast<unsigned int>(type_length), sizeof(buffer) - 1));

        if (result != i2c::Result::ok) {
            return {};
        }

        if (strcmp(buffer.data(), WIFI_MIME_TYPE) != 0) {
            return {};
        }
    }

    // move to payload
    from += type_length + id_length;

    auto read_tlv_value = [](uint16_t address, unsigned int data_length, auto &dst) -> bool {
        if (data_length > sizeof(dst) - 1) {
            // Value is too long, don't accept
            return false;
        }

        // cleanup
        memset(dst.data(), 0, sizeof(dst));

        const auto result = user_read_bytes(EepromCommand::memory,
            address,
            &dst,
            std::min(data_length, sizeof(dst) - 1));

        return result == i2c::Result::ok;
    };

    bool has_ssid { false }, has_password { false };

    for (uint16_t stop = from + payload_length; from < stop;) {
        ndef::WifiValueHeader value_header;

        // read the wifi conig
        const auto result = user_read_bytes(EepromCommand::memory, from, &value_header, sizeof(value_header));
        if (result != i2c::Result::ok) {
            return {};
        }

        // fix endian for multi-byte values
        value_header.type = ntohs(value_header.type);
        value_header.length = ntohs(value_header.length);

        // skip to payload
        from += sizeof(ndef::WifiValueHeader);

        if (value_header.type == ndef::wifi_value_type_credentials) {
            // The payload is a TLV value, don't skip it, continue immediately
            continue;
        }

        if (value_header.type == ndef::wifi_value_type_ssid) {
            const auto data_length { static_cast<unsigned int>(value_header.length) };
            has_ssid = read_tlv_value(from, data_length, credentials.ssid);
            if (!has_ssid) {
                return {};
            }
        } else if (value_header.type == ndef::wifi_value_type_password) {
            const auto data_length { static_cast<unsigned int>(value_header.length) };
            has_password = read_tlv_value(from, data_length, credentials.password);
            if (!has_password) {
                return {};
            }
        }
        from += value_header.length;
    }

    return (has_ssid && has_password) ? std::make_optional(credentials) : std::nullopt;
}

std::optional<WifiCredentials> iterate_ndef(uint16_t from, uint16_t to) {
    for (bool start = true; from < to; start = false) {
        ndef::MessageRecordID rec;
        auto result = user_read_bytes(EepromCommand::memory, from, &rec, sizeof(rec));
        if (result != i2c::Result::ok) {
            return {};
        }

        if (start && (rec.header & ndef::message_begin) != ndef::message_begin) {
            // the 1st message has to have begin bit set
            return {};
        }

        from += sizeof(ndef::MessageRecordID);

        if ((rec.header & ndef::id_length_flag) != ndef::id_length_flag) {
            // it was the short header
            --from; // there was no id_length actually
            rec.id_length = 0;
        }

        if ((rec.header & (ndef::chunk_flag | ndef::type_field)) == ndef::media_type) {
            // only non-chunked and media_type are supported
            if (auto wifi = try_parse_wifi_message(from, rec.type_length, rec.payload_length, rec.id_length)) {
                // have credentials
                return wifi;
            }
        }

        from += rec.type_length + rec.payload_length + rec.id_length;

        if ((rec.header & ndef::message_end) == ndef::message_end) {
            break;
        }
    }

    return {};
}

void init() {
    st25dv64k_wr_cfg(REG_LOCK_CCFILE, 0); // unprotect CC region

    {
        nfc::CapabilityContainer cc {
            .magic = 0xE1,
            .version_and_access = (0x1 << 6), // V1, rest is 0
            .mlen = 0x40, // 512 / 8, // must be less than 0x4ff
            .features = 1, // enable MBREAD
        };

        st25dv64k_present_pwd(0);
        st25dv64k_user_write_bytes(0x0, &cc, sizeof(cc));

        st25dv64k_wr_cfg(REG_LOCK_CCFILE, 0x1); // lock the CC block

        st25dv64k_wr_cfg(REG_ENDA3, 0xFF);
        st25dv64k_wr_cfg(REG_ENDA2, 0xFF); // AREA2 0x500 to end
        st25dv64k_wr_cfg(REG_ENDA1, 0x27); // AREA1 0 to 0x4ff

        st25dv64k_wr_cfg(REG_RFA1SS, 0b0); // AREA 1 RF R/W
        st25dv64k_wr_cfg(REG_RFA2SS, 0b1101); // AREA 2 RF N/A
        st25dv64k_wr_cfg(REG_RFA3SS, 0b1101); // AREA 3 RF N/A
    }

    nfc::init_printer_id();
}

void turn_on() {
    st25dv64k_present_pwd(0);
    uint8_t val { 0 };
    std::ignore = user_write_bytes(EepromCommand::memory, MEM_RF_MNGT_Dyn, &val, sizeof(val));
}

void turn_off() {
    st25dv64k_present_pwd(0);
    uint8_t val { 0b11 };
    std::ignore = user_write_bytes(EepromCommand::memory, MEM_RF_MNGT_Dyn, &val, sizeof(val));
}

bool has_nfc() {
    // Currently, we don't have a way to detect the antenna presence. So just guess based on the printer model.
    // Revisit this if we add more products

#if PRINTER_IS_PRUSA_MK4
    static_assert(extended_printer_type_count == 3);
    return config_store().extended_printer_type.get() == ExtendedPrinterType::mk4s;

#elif PRINTER_IS_PRUSA_MK3_5
    static_assert(!HAS_EXTENDED_PRINTER_TYPE());
    return false;

#else
    #error Revisit this

#endif
}

bool has_activity() {
    uint8_t state { 0 };

    const auto result = user_read_bytes(EepromCommand::memory, MEM_IT_STS_Dyn, &state, sizeof(state));
    if (result != i2c::Result::ok || state == old_state) {
        return false;
    }

    old_state = state;

    return (state & nfc::field_falling);
}

std::optional<WifiCredentials> consume_data() {
    std::optional<WifiCredentials> credentials;

    ndef::TLVHeaderLong tlv;

    const auto result = user_read_bytes(EepromCommand::memory, sizeof(nfc::CapabilityContainer), &tlv, sizeof(tlv));

    if (result != i2c::Result::ok) {
        // not accepting new state to try again next time
        return {};
    }

    if (tlv.type == ndef::Type::tlv) { // NDEF Message TLV
        const uint16_t from = sizeof(nfc::CapabilityContainer)
            + ((tlv.length1 == ndef::tlv_long_header) ? sizeof(ndef::TLVHeaderLong) : sizeof(ndef::TLVHeaderShort));
        const uint16_t to = from
            + ((tlv.length1 == ndef::tlv_long_header) ? ntohs(tlv.length2) : tlv.length1);
        credentials = iterate_ndef(from, to);
    }

    // Always reset the eeprom data, even if we didn't understand them.
    nfc::init_printer_id();

    return credentials;
}

std::atomic<int8_t> SharedEnabler::level { 0 };

SharedEnabler::SharedEnabler() {
    if (level.fetch_add(1) == 0) {
        turn_on();
    }
}

SharedEnabler::~SharedEnabler() {
    if (level.fetch_sub(1) == 1) {
        turn_off();
    }
}

} // namespace nfc
