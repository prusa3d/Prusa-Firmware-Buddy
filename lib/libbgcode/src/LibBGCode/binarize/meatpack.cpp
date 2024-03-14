#include "meatpack.hpp"

#include <unordered_map>
#include <algorithm>
#include <cassert>

namespace MeatPack {

static constexpr const uint8_t Command_None{ 0 };
//#Command_TogglePacking = 253 -- Currently unused, byte 253 can be reused later.
static constexpr const uint8_t Command_EnablePacking{ 251 };
static constexpr const uint8_t Command_DisablePacking{ 250 };
static constexpr const uint8_t Command_ResetAll{ 249 };
static constexpr const uint8_t Command_QueryConfig{ 248 };
static constexpr const uint8_t Command_EnableNoSpaces{ 247 };
static constexpr const uint8_t Command_DisableNoSpaces{ 246 };
static constexpr const uint8_t Command_SignalByte{ 0xFF };

static constexpr const uint8_t BothUnpackable{ 0b11111111 };
static constexpr const char SpaceReplacedCharacter{ 'E' };

static constexpr const unsigned char SecondNotPacked{ 0b11110000 };
static constexpr const unsigned char FirstNotPacked{ 0b00001111 };
static constexpr const unsigned char NextPackedFirst{ 0b00000001 };
static constexpr const unsigned char NextPackedSecond{ 0b00000010 };

static const std::unordered_map<char, uint8_t> ReverseLookupTbl = {
    { '0',  0b00000000 },
    { '1',  0b00000001 },
    { '2',  0b00000010 },
    { '3',  0b00000011 },
    { '4',  0b00000100 },
    { '5',  0b00000101 },
    { '6',  0b00000110 },
    { '7',  0b00000111 },
    { '8',  0b00001000 },
    { '9',  0b00001001 },
    { '.',  0b00001010 },
    { ' ',  0b00001011 },
    { '\n', 0b00001100 },
    { 'G',  0b00001101 },
    { 'X',  0b00001110 },
    { '\0', 0b00001111 } // never used, 0b1111 is used to indicate the next 8-bits is a full character
};

MPBinarizer::LookupTables MPBinarizer::s_lookup_tables = { { 0 }, { 0 }, false, 0 };

MPBinarizer::MPBinarizer(uint8_t flags) : m_flags(flags) {}

void MPBinarizer::initialize(std::vector<uint8_t>& dst)
{
    initialize_lookup_tables();
    append_command(Command_EnablePacking, dst);
    if ((m_flags & Flag_OmitWhitespaces) != 0)
        append_command(Command_EnableNoSpaces, dst);
    m_binarizing = true;
}

void MPBinarizer::finalize(std::vector<uint8_t>& dst)
{
    if ((m_flags & Flag_RemoveComments) != 0) {
        assert(m_binarizing);
        append_command(Command_ResetAll, dst);
        m_binarizing = false;
    }
}

void MPBinarizer::binarize_line(const std::string& line, std::vector<uint8_t>& dst)
{
    auto unified_method = [this](const std::string& line) {
        const std::string::size_type g_idx = line.find('G');
        if (g_idx != std::string::npos) {
            if (g_idx + 1 < line.size() && line[g_idx + 1] >= '0' && line[g_idx + 1] <= '9') {
                if ((m_flags & Flag_OmitWhitespaces) != 0) {
                    std::string result = line;
                    std::replace(result.begin(), result.end(), 'e', 'E');
                    std::replace(result.begin(), result.end(), 'x', 'X');
                    std::replace(result.begin(), result.end(), 'g', 'G');
                    result.erase(std::remove(result.begin(), result.end(), ' '), result.end());
                    if (result.find('*') != std::string::npos) {
                        size_t checksum = 0;
                        result.erase(std::remove(result.begin(), result.end(), '*'), result.end());
                        for (size_t i = 0; i < result.size(); ++i) {
                            checksum ^= static_cast<uint8_t>(result[i]);
                        }
                        result += "*" + std::to_string(checksum);
                    }
                    result += '\n';
                    return result;
                }
                else {
                    std::string result = line;
                    std::replace(result.begin(), result.end(), 'x', 'X');
                    std::replace(result.begin(), result.end(), 'g', 'G');
                    result.erase(std::remove(result.begin(), result.end(), ' '), result.end());
                    if (result.find('*') != std::string::npos) {
                        size_t checksum = 0;
                        result.erase(std::remove(result.begin(), result.end(), '*'), result.end());
                        for (size_t i = 0; i < result.size(); ++i) {
                            checksum ^= static_cast<uint8_t>(result[i]);
                        }
                        result += "*" + std::to_string(checksum);
                    }
                    result += '\n';
                    return result;
                }
            }
        }
        return line;
    };
    auto is_packable = [](char c) {
        return (s_lookup_tables.packable[static_cast<uint8_t>(c)] != 0);
    };
    auto pack_chars = [](char low, char high) {
        return (((s_lookup_tables.value[static_cast<uint8_t>(high)] & 0xF) << 4) |
            (s_lookup_tables.value[static_cast<uint8_t>(low)] & 0xF));
    };

    if (!line.empty()) {
        if ((m_flags & Flag_RemoveComments) == 0) {
            if (line[0] == ';') {
                if (m_binarizing) {
                    append_command(Command_DisablePacking, dst);
                    m_binarizing = false;
                }

                dst.insert(dst.end(), line.begin(), line.end());
                return;
            }
        }

        if (line[0] == ';' ||
            line[0] == '\n' ||
            line[0] == '\r' ||
            line.size() < 2)
            return;

        std::string modifiedLine = line.substr(0, line.find(';'));
        if (modifiedLine.empty())
            return;
        auto trim_right = [](const std::string& str) {
            if (str.back() != ' ')
                return str;
            auto bit = str.rbegin();
            while (bit != str.rend() && *bit == ' ') {
                ++bit;
            }
            return str.substr(0, std::distance(str.begin(), bit.base()));
        };
        modifiedLine = trim_right(modifiedLine);
        modifiedLine = unified_method(modifiedLine);
        if (modifiedLine.back() != '\n')
            modifiedLine.push_back('\n');
        const size_t line_len = modifiedLine.size();
        std::vector<uint8_t> temp_buffer;
        temp_buffer.reserve(line_len);

        for (size_t line_idx = 0; line_idx < line_len; line_idx += 2) {
            const bool skip_last = line_idx == (line_len - 1);
            const char char_1 = modifiedLine[line_idx];
            const char char_2 = (skip_last ? '\n' : modifiedLine[line_idx + 1]);
            const bool c1_p = is_packable(char_1);
            const bool c2_p = is_packable(char_2);

            if (c1_p) {
                if (c2_p)
                    temp_buffer.emplace_back(static_cast<uint8_t>(pack_chars(char_1, char_2)));
                else {
                    temp_buffer.emplace_back(static_cast<uint8_t>(pack_chars(char_1, '\0')));
                    temp_buffer.emplace_back(static_cast<uint8_t>(char_2));
                }
            }
            else {
              if (c2_p) {
                  temp_buffer.emplace_back(static_cast<uint8_t>(pack_chars('\0', char_2)));
                  temp_buffer.emplace_back(static_cast<uint8_t>(char_1));
              }
              else {
                  temp_buffer.emplace_back(static_cast<uint8_t>(BothUnpackable));
                  temp_buffer.emplace_back(static_cast<uint8_t>(char_1));
                  temp_buffer.emplace_back(static_cast<uint8_t>(char_2));
              }
            }
        }

        if (!m_binarizing && !temp_buffer.empty()) {
            append_command(Command_EnablePacking, dst);
            m_binarizing = true;
        }

        dst.insert(dst.end(), temp_buffer.begin(), temp_buffer.end());
    }
}

void MPBinarizer::append_command(unsigned char cmd, std::vector<uint8_t>& dst) {
    dst.emplace_back(Command_SignalByte);
    dst.emplace_back(Command_SignalByte);
    dst.emplace_back(cmd);
}

void MPBinarizer::initialize_lookup_tables() {
    if (s_lookup_tables.initialized && m_flags == s_lookup_tables.flags)
        return;

    for (const auto& [c, value] : ReverseLookupTbl) {
        const int index = static_cast<int>(c);
        s_lookup_tables.packable[index] = 1;
        s_lookup_tables.value[index] = value;
    }

    if ((m_flags & Flag_OmitWhitespaces) != 0) {
        s_lookup_tables.value[static_cast<uint8_t>(SpaceReplacedCharacter)] = ReverseLookupTbl.at(' ');
        s_lookup_tables.packable[static_cast<uint8_t>(SpaceReplacedCharacter)] = 1;
        s_lookup_tables.packable[static_cast<uint8_t>(' ')] = 0;
    }
    else {
        s_lookup_tables.packable[static_cast<uint8_t>(SpaceReplacedCharacter)] = 0;
        s_lookup_tables.packable[static_cast<uint8_t>(' ')] = 1;
    }

    s_lookup_tables.initialized = true;
    s_lookup_tables.flags = m_flags;
}

// See for reference: https://github.com/scottmudge/Prusa-Firmware-MeatPack/blob/MK3_sm_MeatPack/Firmware/meatpack.cpp
void unbinarize(const std::vector<uint8_t>& src, std::string& dst)
{
    bool unbinarizing = false;
    bool nospace_enabled = false;
    bool cmd_active = false;             // Is a command pending
    uint8_t char_buf = 0;                // Buffers a character if dealing with out-of-sequence pairs
    size_t cmd_count = 0;                // Counts how many command bytes are received (need 2)
    size_t full_char_queue = 0;          // Counts how many full-width characters are to be received
    std::array<uint8_t, 2> char_out_buf; // Output buffer for caching up to 2 characters
    size_t char_out_count = 0;           // Stores number of characters to be read out

    auto handle_command = [&](uint8_t c) {
        switch (c)
        {
        case Command_EnablePacking:   { unbinarizing = true; break; }
        case Command_DisablePacking:  { unbinarizing = false; break; }
        case Command_EnableNoSpaces:  { nospace_enabled = true; break; }
        case Command_DisableNoSpaces: { nospace_enabled = false; break; }
        case Command_ResetAll:        { unbinarizing = false; break; }
        default:
        case Command_QueryConfig:     { break; }
        }
    };

    auto handle_output_char = [&](uint8_t c) {
        char_out_buf[char_out_count++] = c;
    };

    auto get_char = [&](uint8_t c) {
        switch (c)
        {
        case 0b0000: { return '0'; }
        case 0b0001: { return '1'; }
        case 0b0010: { return '2'; }
        case 0b0011: { return '3'; }
        case 0b0100: { return '4'; }
        case 0b0101: { return '5'; }
        case 0b0110: { return '6'; }
        case 0b0111: { return '7'; }
        case 0b1000: { return '8'; }
        case 0b1001: { return '9'; }
        case 0b1010: { return '.'; }
        case 0b1011: { return nospace_enabled ? 'E' : ' '; }
        case 0b1100: { return '\n'; }
        case 0b1101: { return 'G'; }
        case 0b1110: { return 'X'; }
        }
        return '\0';
    };

    auto unpack_chars = [&](uint8_t pk, std::array<uint8_t, 2>& chars_out) {
        uint8_t out = 0;

        // If lower 4 bytes is 0b1111, the higher 4 are unused, and next char is full.
        if ((pk & FirstNotPacked) == FirstNotPacked)
            out |= NextPackedFirst;
        else
            chars_out[0] = get_char(pk & 0xF); // Assign lower char

        // Check if upper 4 bytes is 0b1111... if so, we don't need the second char.
        if ((pk & SecondNotPacked) == SecondNotPacked)
            out |= NextPackedSecond;
        else
            chars_out[1] = get_char((pk >> 4) & 0xF); // Assign upper char

        return out;
    };

    auto handle_rx_char = [&](uint8_t c) {
        if (unbinarizing) {
            if (full_char_queue > 0) {
                handle_output_char(c);
                if (char_buf > 0) {
                    handle_output_char(char_buf);
                    char_buf = 0;
                }
                --full_char_queue;
            }
            else {
                std::array<uint8_t, 2> buf = { 0, 0 };
                const uint8_t res = unpack_chars(c, buf);

                if ((res & NextPackedFirst) != 0) {
                    ++full_char_queue;
                    if ((res & NextPackedSecond) != 0)
                        ++full_char_queue;
                    else
                        char_buf = buf[1];
                }
                else {
                    handle_output_char(buf[0]);
                    if (buf[0] != '\n') {
                        if ((res & NextPackedSecond) != 0)
                            ++full_char_queue;
                        else
                            handle_output_char(buf[1]);
                    }
                }
            }
        }
        else // Packing not enabled, just copy character to output
            handle_output_char(c);
    };

    auto get_result_char = [&](std::array<char, 2>& chars_out) {
        if (char_out_count > 0) {
            const size_t res = char_out_count;
            for (uint8_t i = 0; i < char_out_count; ++i) {
                chars_out[i] = (char)char_out_buf[i];
            }
            char_out_count = 0;
            return res;
        }
        return (size_t)0;
    };

    std::vector<uint8_t> unbin_buffer(2 * src.size(), 0);
    auto it_unbin_end = unbin_buffer.begin();

    bool add_space = false;

    auto begin = src.begin();
    auto end = src.end();

    auto it_bin = begin;
    while (it_bin != end) {
        uint8_t c_bin = *it_bin;
        if (c_bin == Command_SignalByte) {
            if (cmd_count > 0) {
                cmd_active = true;
                cmd_count = 0;
            }
            else
              ++cmd_count;
        }
        else {
            if (cmd_active) {
                handle_command(c_bin);
                cmd_active = false;
            }
            else {
                if (cmd_count > 0) {
                    handle_rx_char(Command_SignalByte);
                    cmd_count = 0;
                }

                handle_rx_char(c_bin);
            }
        }

        auto is_gline_parameter = [](const char c) {
            static const std::vector<char> parameters = {
                // G0, G1
                'X', 'Y', 'Z', 'E', 'F',
                // G2, G3
                'I', 'J', 'R',
                // G29
                'P', 'W', 'H', 'C', 'A'
            };
            return std::find(parameters.begin(), parameters.end(), c) != parameters.end();
        };

        std::array<char, 2> c_unbin{ 0, 0 };
        const size_t char_count = get_result_char(c_unbin);
        for (size_t i = 0; i < char_count; ++i) {
            // GCodeReader::parse_line_internal() is unable to parse a G line where the data are not separated by spaces
            // so we add them where needed
            const size_t curr_unbin_buffer_length = std::distance(unbin_buffer.begin(), it_unbin_end);
            if (c_unbin[i] == 'G' && (curr_unbin_buffer_length == 0 || *std::prev(it_unbin_end, 1) == '\n'))
                add_space = true;
            else if (c_unbin[i] == '\n')
                add_space = false;

            if (add_space && (curr_unbin_buffer_length == 0 || *std::prev(it_unbin_end, 1) != ' ') &&
                is_gline_parameter(c_unbin[i])) {
                *it_unbin_end = ' ';
                ++it_unbin_end;
            }

            if (c_unbin[i] != '\n' || std::distance(unbin_buffer.begin(), it_unbin_end) == 0 || *std::prev(it_unbin_end, 1) != '\n') {
                *it_unbin_end = c_unbin[i];
                ++it_unbin_end;
            }
        }

        ++it_bin;
    }

    dst.insert(dst.end(), unbin_buffer.begin(), it_unbin_end);
}

} //  namespace MeatPack
