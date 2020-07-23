#pragma once

#include <cstdint>
//#include "unicode.h"
#include <string.h>
#include <stdio.h>

#define UTF8_IS_NONASCII(ch) ((ch)&0x80)
#define UTF8_IS_CONT(ch)     (((ch)&0xC0) == 0x80)

using unichar = std::uint32_t;

/// string_view_utf8 allows for iteration over utf8 characters
/// There will be multiple implementations which will differ in processes of:
/// - reading a character from input (not necessarily some memory)
/// - rewinding to the beginning of the source
/// Typically, CPU FLASH implementation will be different than reading from a USB flash file
///
/// Beware:
/// 1. The string wrapped in a string_view_utf8 doesn't exist beyond the scope of this class
///    i.e. don't be tempted to save a pointer to it. The string may not be represented by a memory address at all!
/// 2. string_view_utf8 is not a container - it neither allocates any memory nor copies the input string
///    and it is the user's responsibility to ensure correct lifetime of the strings created with MakeRAM()
/// 3. Due to implementation reasons:
///    - the whole string_view_utf8 must be created on stack and must remain on stack (to prevent fragmentation of heap)
///    - while allowing for multiple different sources of data
///    need to implement some kind of virtual methods but within an instance, which is achieved by pointers to functions and a union of all possible attributes
class string_view_utf8 {
    union Attrs {
        /// interface for utf-8 strings stored in the CPU FLASH.
        struct FromCPUFLASH_RAM {
            const uint8_t *utf8raw; ///< pointer to raw utf8 data
            const uint8_t *readp;   ///< read pointer, aka read iterator
        } cpuflash;
        /// interface for utf-8 string stored in a FILE - used for validation of the whole translation infrastructure
        struct FromFile {
            ::FILE *f;         ///< shared FILE pointer with other instances accessing the same file
                               ///< @@TODO beware - need some synchronization mechanism to prevent reading from another offset in the file when other instances read as well
            uint32_t startOfs; ///< start offset in input file
        } file;
    };
    Attrs attrs;

    /// Extracts one byte from source media and advances internal read ptr, implementation defined in derived classes
    /// The caller of this function makes sure it does not get called repeatedly after returning the end of input data.
    /// @return 0 in case of end of input data or an error
    uint8_t (*getbyte)(Attrs &);

    /// Rewinds the input data stream to its beginning
    /// Simple for memory-based sources, more complex for files
    void (*rewind_impl)(Attrs &);

    /// cached length of string. Computed at various spots where not too costly.
    /// Deliberately typed as signed int, because -1 means "not computed yet"
    /// @@TODO implement into getUtf8Char somehow
    mutable int16_t utf8Length;

    enum class EType : uint8_t { RAM,
        CPUFLASH,
        SPIFLASH,
        USBFLASH,
        FILE,
        NULLSTR };
    EType type;

    mutable uint8_t s; ///< must remember the last read character

    static uint8_t CPUFLASH_getbyte(Attrs &attrs) {
        return *attrs.cpuflash.readp++; // beware - expecting, that the input string is null-terminated! No other checks are done
    }
    static void CPUFLASH_rewind(Attrs &attrs) {
        attrs.cpuflash.readp = attrs.cpuflash.utf8raw;
    }

    static uint8_t FILE_getbyte(Attrs &attrs) {
        uint8_t c;
        fread(&c, 1, 1, attrs.file.f);
        return c;
    }
    static void FILE_rewind(Attrs &attrs) {
        if (attrs.file.f) {
            fseek(attrs.file.f, attrs.file.startOfs, SEEK_SET);
        }
    }

    static uint8_t NULLSTR_getbyte(Attrs & /*attrs*/) {
        return 0;
    }
    static void NULLSTR_rewind(Attrs & /*attrs*/) {
    }

public:
    inline string_view_utf8()
        : utf8Length(-1)
        , type(EType::NULLSTR)
        , s(0xff) {}
    ~string_view_utf8() = default;

    /// @returns one UTF-8 character from the input data
    /// and advances internal pointers (in derived classes) to the next one
    unichar getUtf8Char() {
        if (s == 0xff) { // in case we don't have any character from the last run, get a new one from the input stream
            s = getbyte(attrs);
        }
        unichar ord = s;
        if (!UTF8_IS_NONASCII(ord)) {
            s = 0xff; // consumed, not available for next run
            return ord;
        }
        ord &= 0x7F;
        for (unichar mask = 0x40; ord & mask; mask >>= 1) {
            ord &= ~mask;
        }
        s = getbyte(attrs);
        while (UTF8_IS_CONT(s)) {
            ord = (ord << 6) | (s & 0x3F);
            s = getbyte(attrs);
        }
        return ord;
    }

    /// @returns number of UTF-8 characters
    /// Beware: this may be something different than byte-length of the string
    /// Takes O(n) and involves calling getbyte(), thus it may take some time on files.
    /// Moreover, it modifies the reading position of the stream, the stream is at its end after this method finishes.
    /// Therefore it is not a const method.
    uint16_t computeNumUtf8CharsAndRewind() {
        if (utf8Length < 0) {
            rewind_impl(attrs);
            do {
                ++utf8Length;
            } while (getUtf8Char());
        }
        rewind_impl(attrs); // always return stream back to the beginning @@TODO subject to change
        // now we have either 0 or some positive number in utf8Length, can be safely cast to unsigned int
        return uint32_t(utf8Length);
    }

    /// rewind the stream to its beginning
    void rewind() {
        rewind_impl(attrs);
    }

    /// returns true if the string if of type NULLSTR - typically used as a replacement for nullptr or "" strings
    inline bool isNULLSTR() const {
        return type == EType::NULLSTR;
    }

    /// Copy the string byte-by-byte into some RAM buffer for later processing,
    /// typically used to obtain a translated version of a format string for s(n)printf
    /// @param dst target buffer to copy the bytes to
    /// @param max_size size of dst in bytes
    /// Using sprintf to format some string is possible with translations, but it requires one more step than usually -
    /// one must first fetch the translated format string into a RAM buffer and then feed the format string into standard sprintf
    void copyToRAM(char *dst, size_t max_size) {
        for (size_t i = 0; i < max_size; ++i) {
            *dst = getbyte(attrs);
            if (*dst == 0)
                return;
            ++dst;
        }
        *dst = 0; // safety termination in case of reaching the end of the buffer
    }

    /// Construct string_view_utf8 to provide data from CPU FLASH
    static string_view_utf8 MakeCPUFLASH(const uint8_t *utf8raw) {
        string_view_utf8 s;
        s.attrs.cpuflash.readp = s.attrs.cpuflash.utf8raw = utf8raw;
        s.getbyte = &string_view_utf8::CPUFLASH_getbyte;
        s.rewind_impl = &string_view_utf8::CPUFLASH_rewind;
        s.type = EType::CPUFLASH;
        return s;
    }

    /// Construct string_view_utf8 to provide data from RAM
    /// basically the same as from CPU FLASH, only the string_view_utf8's type differs of course
    static string_view_utf8 MakeRAM(const uint8_t *utf8raw) {
        string_view_utf8 s;
        s.attrs.cpuflash.readp = s.attrs.cpuflash.utf8raw = utf8raw;
        s.getbyte = &string_view_utf8::CPUFLASH_getbyte;
        s.rewind_impl = &string_view_utf8::CPUFLASH_rewind;
        s.type = EType::RAM;
        return s;
    }

    /// Construct string_view_utf8 to provide data from FILE
    /// The FILE *f shall aready be positioned to the spot, where the string starts
    static string_view_utf8 MakeFILE(::FILE *f) {
        string_view_utf8 s;
        s.attrs.file.f = f;
        if (f) {
            s.attrs.file.startOfs = ftell(f);
        }
        s.getbyte = &string_view_utf8::FILE_getbyte;
        s.rewind_impl = &string_view_utf8::FILE_rewind;
        s.type = EType::FILE;
        return s;
    }

    /// Construct an empty string_view_utf8 - behaves like a "" but is a special type NULL
    static string_view_utf8 MakeNULLSTR() {
        string_view_utf8 s;
        s.getbyte = &string_view_utf8::NULLSTR_getbyte;
        s.rewind_impl = &string_view_utf8::NULLSTR_rewind;
        s.type = EType::NULLSTR;
        return s;
    }
};
