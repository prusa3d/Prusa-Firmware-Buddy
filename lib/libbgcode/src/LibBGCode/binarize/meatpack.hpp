#ifndef _BGCODE_BINARIZE_MEATPACK_HPP_
#define _BGCODE_BINARIZE_MEATPACK_HPP_

#include <cstdint>
#include <vector>
#include <string>
#include <array>

//
// Adaptation of MeatPack G-Code Compression taken from:
// https://github.com/scottmudge/OctoPrint-MeatPack/blob/master/OctoPrint_MeatPack/meatpack.py
// https://github.com/bubnikv/OctoPrint-MeatPack/blob/master/OctoPrint_MeatPack/meatpack.py
// https://github.com/scottmudge/Prusa-Firmware-MeatPack/blob/MK3_sm_MeatPack/Firmware/meatpack.h
// https://github.com/scottmudge/Prusa-Firmware-MeatPack/blob/MK3_sm_MeatPack/Firmware/meatpack.cpp
// 

namespace MeatPack {

static constexpr const uint8_t Flag_OmitWhitespaces{ 0x01 };
static constexpr const uint8_t Flag_RemoveComments{ 0x02 };

class MPBinarizer
{
public:
    explicit MPBinarizer(uint8_t flags = 0);

    void initialize(std::vector<uint8_t>& dst);
    void finalize(std::vector<uint8_t>& dst);

    void binarize_line(const std::string& line, std::vector<uint8_t>& dst);

private:
    unsigned char m_flags{ 0 };
    bool m_binarizing{ false };

    struct LookupTables
    {
        std::array<uint8_t, 256> packable;
        std::array<uint8_t, 256> value;
        bool initialized;
        unsigned char flags;
    };

    static LookupTables s_lookup_tables;

    void append_command(unsigned char cmd, std::vector<uint8_t>& dst);
    void initialize_lookup_tables();
};

extern void unbinarize(const std::vector<uint8_t>& src, std::string& dst);

} // namespace MeatPack

#endif // _BGCODE_BINARIZE_MEATPACK_HPP_
