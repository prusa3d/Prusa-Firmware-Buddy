#include "st25dv64k.h"

#include "i2c.hpp"
#include <string.h>
#include "cmsis_os.h"
#include "bsod.h"
#include "utility_extensions.hpp"
#include <algorithm>

#if HAS_NFC()

    // ntohs
    #ifdef UNITTESTS
        #include <arpa/inet.h>
    #else
        #include <lwip/def.h>
    #endif

    #include <str_utils.hpp>
    #include <string.h>

#endif

namespace {

#define ST25DV64K_RTOS

// system config address registr
//  constexpr const uint16_t REG_GPO = 0x0000;
//  constexpr const uint16_t REG_IT_TIME = 0x0001;
//  constexpr const uint16_t REG_EH_MODE = 0x0002;
//  constexpr const uint16_t REG_RF_MNGT = 0x0003;
constexpr const uint16_t REG_RFA1SS = 0x0004;
constexpr const uint16_t REG_ENDA1 = 0x0005;
constexpr const uint16_t REG_RFA2SS = 0x0006;
constexpr const uint16_t REG_ENDA2 = 0x0007;
constexpr const uint16_t REG_RFA3SS = 0x0008;
constexpr const uint16_t REG_ENDA3 = 0x0009;
// constexpr const uint16_t REG_RFA4SS = 0x000A;
// constexpr const uint16_t REG_I2CSS = 0x000B;
constexpr const uint16_t REG_LOCK_CCFILE = 0x000C;
// constexpr const uint16_t REG_MB_MODE = 0x000D;
// constexpr const uint16_t REG_MB_WDG = 0x000E;
// constexpr const uint16_t REG_LOCK_CFG = 0x000F;
constexpr const uint16_t MEM_IT_STS_Dyn = 0x2005;

// EEPROM I2C addresses
enum class EepromCommand : bool {
    memory,
    registers
};

enum class EepromCommandWrite : uint16_t {
    addr_memory = 0xA6,
    addr_registers = 0xAE
};

enum class EepromCommandRead : uint16_t {
    addr_memory = 0xA7,
    addr_registers = 0xAF
};

constexpr EepromCommandWrite eeprom_get_write_address(EepromCommand cmd) {
    if (cmd == EepromCommand::memory) {
        return EepromCommandWrite::addr_memory;
    }
    return EepromCommandWrite::addr_registers;
}

constexpr EepromCommandRead eeprom_get_read_address(EepromCommand cmd) {
    if (cmd == EepromCommand::memory) {
        return EepromCommandRead::addr_memory;
    }
    return EepromCommandRead::addr_registers;
}

constexpr const uint8_t BLOCK_DELAY = 5; // block delay [ms]
constexpr const uint8_t BLOCK_BYTES = 4; // bytes per block

constexpr const uint32_t RETRIES = 3;

#define DELAY HAL_Delay

uint8_t st25dv64k_initialised = 0;

#ifdef ST25DV64K_RTOS

    #include "cmsis_os.h"

osSemaphoreId st25dv64k_sema = 0; // semaphore handle

inline void st25dv64k_lock() {
    if (st25dv64k_sema == 0) {
        osSemaphoreDef(st25dv64kSema);
        st25dv64k_sema = osSemaphoreCreate(osSemaphore(st25dv64kSema), 1);
    }
    osSemaphoreWait(st25dv64k_sema, osWaitForever);
}

inline void st25dv64k_unlock() {
    osSemaphoreRelease(st25dv64k_sema);
}

    #define st25dv64k_delay osDelay

#else
    #define st25dv64k_lock()
    #define st25dv64k_unlock()
    #define st25dv64k_delay HAL_Delay
#endif // ST25DV64K_RTOS

void rise_error_if_needed(i2c::Result result) {
    switch (result) {
    case i2c::Result::ok:
        break;
    case i2c::Result::busy_after_retries:
        fatal_error(ErrCode::ERR_ELECTRO_I2C_TX_BUSY);
        break;
    case i2c::Result::error:
        fatal_error(ErrCode::ERR_ELECTRO_I2C_TX_ERROR);
        break;
    case i2c::Result::timeout:
        fatal_error(ErrCode::ERR_ELECTRO_I2C_TX_TIMEOUT);
        break;
    }
}

void try_fix_if_needed(const i2c::Result &result) {
    switch (result) {
    case i2c::Result::busy_after_retries:
    case i2c::Result::error:
        I2C_INIT(eeprom);
        [[fallthrough]];
    case i2c::Result::timeout:
    case i2c::Result::ok:
        break;
    }
}

[[nodiscard]] i2c::Result eeprom_transmit(EepromCommandWrite cmd, uint8_t *pData, uint16_t size) {
    return i2c::Transmit(I2C_HANDLE_FOR(eeprom), ftrstd::to_underlying(cmd), pData, size, HAL_MAX_DELAY);
}

[[nodiscard]] i2c::Result user_write_address_without_lock(EepromCommandWrite cmd, uint16_t address) {
    uint8_t _out[sizeof(address)];
    _out[0] = address >> 8;
    _out[1] = address & 0xff;

    i2c::Result result = eeprom_transmit(cmd, _out, sizeof(address));
    DELAY(BLOCK_DELAY);

    return result;
}

[[nodiscard]] i2c::Result user_write_bytes_without_lock(EepromCommandWrite cmd, uint16_t address, const void *pdata, uint16_t size) {
    if (size == 0 || pdata == nullptr) {
        return user_write_address_without_lock(cmd, address);
    }

    uint8_t const *p = (uint8_t const *)pdata;
    uint8_t _out[sizeof(address) + BLOCK_BYTES];
    while (size) {
        uint8_t block_size = BLOCK_BYTES - (address % BLOCK_BYTES);
        if (block_size > size) {
            block_size = size;
        }
        _out[0] = address >> 8;
        _out[1] = address & 0xff;
        memcpy(_out + sizeof(address), p, block_size);

        i2c::Result result = eeprom_transmit(cmd, _out, sizeof(address) + block_size);
        if (result != i2c::Result::ok) {
            return result;
        }

        DELAY(BLOCK_DELAY);

        size -= block_size;
        address += block_size;
        p += block_size;
    }
    return i2c::Result::ok;
}

[[nodiscard]] i2c::Result user_read_bytes_without_lock(EepromCommand cmd, uint16_t address, void *pdata, uint16_t size) {
    if (size == 0) {
        return i2c::Result::ok;
    }

    i2c::Result result = user_write_address_without_lock(eeprom_get_write_address(cmd), address);
    if (result == i2c::Result::ok) {
        result = i2c::Receive(I2C_HANDLE_FOR(eeprom), ftrstd::to_underlying(eeprom_get_read_address(cmd)), static_cast<uint8_t *>(pdata), size, HAL_MAX_DELAY);
    }

    return result;
}

[[nodiscard]] i2c::Result user_read_bytes(EepromCommand cmd, uint16_t address, void *pdata, uint16_t size) {
    if (size == 0) {
        return i2c::Result::ok;
    }

    i2c::Result result = i2c::Result::error;

    // receive retry requires new transmit
    for (uint32_t try_no = 0; (result != i2c::Result::ok) && (try_no < RETRIES); ++try_no) {
        st25dv64k_lock();
        result = user_read_bytes_without_lock(cmd, address, pdata, size);
        st25dv64k_unlock();
        try_fix_if_needed(result);
    }

    return result;
}

[[nodiscard]] i2c::Result user_write_bytes(EepromCommand cmd, uint16_t address, const void *pdata, uint16_t size) {
    if (size == 0) {
        return i2c::Result::ok;
    }

    i2c::Result result = i2c::Result::error;
    bool match = false;

    for (uint32_t try_no = 0; try_no < RETRIES; ++try_no) {

        st25dv64k_lock();
        result = user_write_bytes_without_lock(EepromCommandWrite::addr_memory, address, pdata, size);
        if (result == i2c::Result::ok) {
            // Verify the data being written correctly
            uint8_t chunk_read[32];
            uint16_t read_pos = 0;
            match = true;
            while (read_pos < size) {
                uint16_t to_read = std::min<uint16_t>(sizeof(chunk_read), size - read_pos);
                result = user_read_bytes_without_lock(cmd, address + read_pos, chunk_read, to_read);
                if (result != i2c::Result::ok) {
                    break;
                }
                if (memcmp(chunk_read, ((uint8_t *)pdata) + read_pos, to_read)) {
                    match = false;
                    break;
                }
                read_pos += to_read;
            }
        }
        st25dv64k_unlock();
        try_fix_if_needed(result);

        match = match && result == i2c::Result::ok;
        // Stop retrying on match
        if (match) {
            break;
        }
    }

    return result;
}

[[nodiscard]] i2c::Result user_unverified_write_bytes(uint16_t address, const void *pdata, uint16_t size) {
    if (size == 0) {
        return i2c::Result::ok;
    }

    i2c::Result result = i2c::Result::error;

    for (uint32_t try_no = 0; try_no < RETRIES; ++try_no) {

        st25dv64k_lock();
        result = user_write_bytes_without_lock(EepromCommandWrite::addr_memory, address, pdata, size);
        st25dv64k_unlock();
        try_fix_if_needed(result);

        if (result == i2c::Result::ok) {
            break;
        }
    }

    return result;
}

} // namespace

#if HAS_NFC()

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
        type_name_format = 0b111,
        id_length_flag = 0x1 << 3,
        short_record = 0x1 << 4,
        chunk_flag = 0x1 << 5,
        message_end = 0x1 << 6,
        message_begin = 0x1 << 7,
    };

    enum Message_Type_Format {
        media_type = 0x2,
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

    [[maybe_unused]] auto result = user_write_bytes(EepromCommand::memory, dst_addr, &tlv, sizeof(tlv));
    dst_addr += sizeof(tlv);

    result = user_write_bytes(EepromCommand::memory, dst_addr, data, sizeof(data));
    dst_addr += sizeof(data);

    tlv = {
        .type = ndef::Type::terminator, // terminator
        .length = sizeof(data)
    };

    result = user_write_bytes(EepromCommand::memory, dst_addr, &tlv, sizeof(tlv));
    dst_addr += sizeof(tlv);

    // overwrite possible previous credentials,
    // the 128 is not a strict limit, there is 0x3ff bytes for NFC
    for (uint32_t zero { 0 }; dst_addr < 128; dst_addr += sizeof(zero)) {
        // intentionally writing small data; larger blocks could fail
        result = user_write_bytes(EepromCommand::memory, dst_addr, &zero, sizeof(zero));
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
    for (; from < to;) {
        ndef::MessageRecordID rec;
        auto result = user_read_bytes(EepromCommand::memory, from, &rec, sizeof(rec));
        if (result != i2c::Result::ok) {
            return {};
        }

        if ((rec.header & (ndef::message_begin | ndef::message_end | ndef::chunk_flag | ndef::type_name_format)) != (ndef::message_begin | ndef::message_end | ndef::Message_Type_Format::media_type)) {
            // can't parse those
            return {};
        }

        from += sizeof(ndef::MessageRecordID);
        if ((rec.header & ndef::id_length_flag) == 0) {
            // it was the short header
            --from; // there was no id_length actually
            rec.id_length = 0;
        }

        if (auto wifi = try_parse_wifi_message(from, rec.type_length, rec.payload_length, rec.id_length)) {
            // have credentials
            return wifi;
        }

        from += 1 /* header */ + rec.type_length + rec.payload_length + rec.id_length;
    }

    return {};
}

std::optional<WifiCredentials> try_detect_wifi_credentials() {
    static uint8_t old_state { 0 };
    uint8_t state { 0 };

    auto result = user_read_bytes(EepromCommand::memory, MEM_IT_STS_Dyn, &state, sizeof(state));
    if (result != i2c::Result::ok || state == old_state) {
        return {};
    }

    std::optional<WifiCredentials> credentials;

    if (state & nfc::field_falling) {
        ndef::TLVHeaderLong tlv;

        result = user_read_bytes(EepromCommand::memory, sizeof(nfc::CapabilityContainer), &tlv, sizeof(tlv));

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
    }
    old_state = state;

    if (credentials) {
        // reset the data
        nfc::init_printer_id();
    }

    return credentials;
}

} // namespace nfc

#endif

void st25dv64k_user_read_bytes(uint16_t address, void *pdata, uint16_t size) {
    auto result = user_read_bytes(EepromCommand::memory, address, pdata, size);
    rise_error_if_needed(result);
}

uint8_t st25dv64k_user_read(uint16_t address) {
    uint8_t data;
    st25dv64k_user_read_bytes(address, &data, sizeof(data));
    return data;
}

void st25dv64k_user_write_bytes(uint16_t address, const void *pdata, uint16_t size) {
    auto result = user_write_bytes(EepromCommand::memory, address, pdata, size);
    rise_error_if_needed(result);
}

void st25dv64k_user_unverified_write_bytes(uint16_t address, const void *pdata, uint16_t size) {
    auto result = user_unverified_write_bytes(address, pdata, size);
    rise_error_if_needed(result);
}

void st25dv64k_user_write(uint16_t address, uint8_t data) {
    st25dv64k_user_write_bytes(address, &data, sizeof(data));
}

uint8_t st25dv64k_rd_cfg(uint16_t address) {
    uint8_t data;
    auto result = user_read_bytes(EepromCommand::registers, address, &data, sizeof(data));
    rise_error_if_needed(result);
    return data;
}

void st25dv64k_wr_cfg(uint16_t address, uint8_t data) {
    auto result = user_write_bytes(EepromCommand::registers, address, &data, sizeof(data));
    rise_error_if_needed(result);
}

void st25dv64k_present_pwd(uint8_t *pwd) {
    uint8_t _out[19] = { 0x09, 0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0x09, 0, 0, 0, 0, 0, 0, 0, 0 };
    if (pwd) {
        memcpy(_out + 2, pwd, 8);
        memcpy(_out + 11, pwd, 8);
    }

    i2c::Result result = i2c::Result::error;

    for (uint32_t try_no = 0; (result != i2c::Result::ok) && (try_no < RETRIES); ++try_no) {
        st25dv64k_lock();
        result = eeprom_transmit(EepromCommandWrite::addr_registers, _out, sizeof(_out));
        st25dv64k_unlock();
        try_fix_if_needed(result);
    }

    rise_error_if_needed(result);
}

void st25dv64k_init() {
    if (st25dv64k_initialised) {
        return;
    }
    st25dv64k_initialised = 1;

    st25dv64k_present_pwd(0);
    st25dv64k_wr_cfg(REG_ENDA3, 0xFF);
    st25dv64k_present_pwd(0);
    st25dv64k_wr_cfg(REG_ENDA2, 0xFF); // AREA2 0x500 to end
    st25dv64k_present_pwd(0);
    st25dv64k_wr_cfg(REG_ENDA1, 0x27); // AREA1 0 to 0x4ff

    st25dv64k_wr_cfg(REG_RFA1SS, 0b0); // AREA 1 RF R/W
    st25dv64k_wr_cfg(REG_RFA2SS, 0b1101); // AREA 2 RF N/A
    st25dv64k_wr_cfg(REG_RFA3SS, 0b1101); // AREA 3 RF N/A

#if HAS_NFC()
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
    }

    nfc::init_printer_id();
#endif
}
