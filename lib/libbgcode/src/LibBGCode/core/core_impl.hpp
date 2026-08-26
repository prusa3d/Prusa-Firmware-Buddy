#ifndef CORE_IMPL_HPP
#define CORE_IMPL_HPP

#include <iterator>
#include <type_traits>
#include <climits>
#include <cstdint>

#include "core.hpp"

namespace bgcode { namespace core {

// Max size of checksum buffer data, in bytes
// Increase this value if you implement a checksum algorithm needing a bigger buffer
static constexpr const size_t MAX_CHECKSUM_SIZE = 4;

static constexpr const std::array<char, 4> MAGIC{ 'G', 'C', 'D', 'E' };

// Highest binary gcode file version supported.
static constexpr const uint32_t VERSION = 1;

template<class I, class T = I>
using IntegerOnly = std::enable_if_t<std::is_integral_v<I>, T>;

template<class T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template<class BufT> constexpr bool IsBufferType =
    std::is_convertible_v<remove_cvref_t<BufT>, std::byte> ||
    std::is_convertible_v<remove_cvref_t<BufT>, unsigned char> ||
    std::is_convertible_v<remove_cvref_t<BufT>, char>;

template<class It> constexpr bool IsBufferIterator =
    IsBufferType< typename std::iterator_traits<It>::value_type>;

template<class BufT, class T = BufT>
using BufferTypeOnly = std::enable_if_t<IsBufferType<BufT>, T>;

template<class It, class T = It>
using BufferIteratorOnly = std::enable_if_t<IsBufferIterator<It>, T>;

// For LE byte sequences only
template<class IntT, class It, class = BufferIteratorOnly<It>>
constexpr IntegerOnly<IntT> load_integer(It from, It to) noexcept
{
    IntT result{};

    size_t i = 0;
    for (It it = from; it != to && i < sizeof(IntT); ++it) {
        result |= (static_cast<IntT>(*it) << (i++ * sizeof(std::byte) * CHAR_BIT));
    }

    return result;
}

template<class IntT, class OutIt>
constexpr BufferIteratorOnly<OutIt, void>
store_integer_le(IntT value, OutIt out, size_t sz = sizeof(IntT))
{
    for (size_t i = 0; i < std::min(sizeof(IntT), sz); ++i)
    {
        *out++ = static_cast<typename std::iterator_traits<OutIt>::value_type>(
            (value >> (i * CHAR_BIT)) & UCHAR_MAX
            );
    }
}

template<class IntT, class OutIt>
std::enable_if_t<!IsBufferIterator<OutIt>, void>
store_integer_le(IntT value, OutIt out, size_t sz = sizeof(IntT))
{
    for (size_t i = 0; i < std::min(sizeof(IntT), sz); ++i)
    {
        *out++ = (value >> (i * CHAR_BIT)) & UCHAR_MAX;
    }
}

template<class It, class = BufferIteratorOnly<It>>
static constexpr uint32_t crc32_sw(It from, It to, uint32_t crc)
{
    constexpr uint32_t ui32Max = 0xFFFFFFFF;
    constexpr uint32_t crcMagic = 0xEDB88320;

    uint32_t value = crc ^ ui32Max;
    for (auto it = from; it != to; ++it) {
        value ^= load_integer<uint32_t>(it, std::next(it));
        for (int bit = 0; bit < CHAR_BIT; bit++) {
            if (value & 1)
                value = (value >> 1) ^ crcMagic;
            else
                value >>= 1;
        }
    }
    value ^= ui32Max;

    return value;
}

template<class Enum>
constexpr auto to_underlying(Enum enumval) noexcept
{
    return static_cast<std::underlying_type_t<Enum>>(enumval);
}

class BGCODE_CORE_EXPORT Checksum
{
public:
    // Constructs a checksum of the given type.
    // The checksum data are sized accordingly.
    explicit Checksum(EChecksumType type);

    EChecksumType get_type() const noexcept { return m_type; }

    // Append vector of data to checksum
    void append(const std::vector<std::byte>& data);

    // Append data to the checksum
    template<class BufT>
    void append(const BufT* data, size_t size);

    // Append any aritmetic data to the checksum (shorthand for aritmetic types)
    template<typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
    void append(T& data) { append(reinterpret_cast<const std::byte*>(&data), sizeof(data)); }

    // Returns true if the given checksum is equal to this one
    bool matches(Checksum& other);

    EResult write(FILE& file);
    EResult read(FILE& file);

private:
    EChecksumType m_type;
    // actual size of checksum buffer, type dependent
    size_t m_size;
    std::array<std::byte, MAX_CHECKSUM_SIZE> m_checksum;
};

// Updates the given checksum with the data of this BlockHeader
inline void update_checksum(Checksum& checksum, const BlockHeader &block_header)
{
    checksum.append(block_header.type);
    checksum.append(block_header.compression);
    checksum.append(block_header.uncompressed_size);
    if (block_header.compression != to_underlying(ECompressionType::None))
        checksum.append(block_header.compressed_size);
}

template<class BufT>
void Checksum::append(const BufT *data, size_t size)
{
    if (data == nullptr || size == 0)
        return;

    switch (m_type)
    {
    case EChecksumType::None:
    {
        break;
    }
    case EChecksumType::CRC32:
    {
        static_assert(sizeof(m_checksum) >= sizeof(uint32_t), "CRC32 checksum requires at least 4 bytes");
        const auto old_crc = load_integer<uint32_t>(m_checksum.begin(), m_checksum.end()); //*(uint32_t*)m_checksum.data();
        const uint32_t new_crc = crc32_sw(data, data + size, old_crc);
        store_integer_le(new_crc, m_checksum.begin(), m_checksum.size());
        break;
    }
    }
}

static constexpr auto MAGICi32 = load_integer<uint32_t>(std::begin(MAGIC), std::end(MAGIC));

constexpr auto checksum_types_count() noexcept { auto v = to_underlying(EChecksumType::CRC32); ++v; return v;}
constexpr auto block_types_count() noexcept { auto v = to_underlying(EBlockType::Thumbnail); ++v; return v; }
constexpr auto compression_types_count() noexcept { auto v = to_underlying(ECompressionType::Heatshrink_12_4); ++v; return v; }

} // namespace core
} // namespace bgcode

#endif // CORE_IMPL_HPP
