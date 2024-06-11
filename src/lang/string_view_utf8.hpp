#pragma once

#include <cstdint>
// #include "unicode.h"
#include <string.h>
#include <stdio.h>
#include "assert.h"
#include <cstdlib>
#include <span>

#define UTF8_IS_NONASCII(ch) ((ch)&0x80)
#define UTF8_IS_CONT(ch)     (((ch)&0xC0) == 0x80)

using unichar = std::uint32_t;

/// string_view_utf8 allows for iteration over utf8 characters
/// There will be multiple implementations which will differ in processes of:
/// - reading a character from input (not necessarily some memory)
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
    friend class StringReaderUtf8;

public:
    using Length = int16_t;

    enum class Type : uint8_t {
        /// No string.
        /// Both \p file and \p memory_ptr are null
        null_string,

        /// The string is stored in the memory (RAM/CPU FLASH)
        /// \p file is null, \p memory_ptr contains the pointer
        memory_string,

        /// The strnig is stored in the file
        /// \p file contains the file handle, \p file_offset contains the offset
        file_string,
    };

private:
    /// If the string view points to a file string, contains the file handle. Otherwise null.
    FILE *file = nullptr;

    union {
        /// If file is null, this is a poitner to the memory
        const uint8_t *memory_ptr = nullptr;

        /// If file is not null, this is used as an offset to the file
        uint32_t file_offset;
    };

public:
    string_view_utf8() = default;
    string_view_utf8(const string_view_utf8 &) = default;
    string_view_utf8(string_view_utf8 &&) = default;

    /// @returns number of UTF-8 characters
    /// Beware: this may be something different than byte-length of the string
    /// Takes O(n) and involves calling getbyte(), thus it may take some time on files.
    Length computeNumUtf8Chars() const;

    unichar getFirstUtf8Char() const;

    constexpr inline Type type() const {
        if (file) {
            return Type::file_string;

        } else if (memory_ptr) {
            return Type::memory_string;

        } else {
            return Type::null_string;
        }
    }

    /// returns true if the string is of type NULLSTR - typically used as a replacement for nullptr or "" strings
    constexpr bool isNULLSTR() const {
        return type() == Type::null_string;
    }

    /// Copy the string byte-by-byte into some RAM buffer for later processing, without multibyte cutting check
    /// typically used to obtain a translated version of a format string for s(n)printf
    /// @param dst target buffer to copy the bytes to
    /// @param buffer_size size of dst in bytes
    /// @returns number of bytes (not utf8 characters) copied not counting the terminating '\0'
    /// Using sprintf to format some string is possible with translations, but it requires one more step than usually -
    /// one must first fetch the translated format string into a RAM buffer and then feed the format string into standard sprintf
    size_t copyBytesToRAM(char *dst, size_t buffer_size) const;

    /// Copy the string byte-by-byte into some RAM buffer for later processing,
    /// typically used to obtain a translated version of a format string for s(n)printf, it also checks cut off characters in truncated strings
    /// @param dst target buffer to copy the chars to
    /// @param buffer_size size of dst buffer in bytes
    /// @returns number of bytes (not utf8 characters) copied not counting the terminating '\0'
    /// Using sprintf to format some string is possible with translations, but it requires one more step than usually -
    /// one must first fetch the translated format string into a RAM buffer and then feed the format string into standard sprintf
    size_t copyToRAM(char *dst, size_t buffer_size) const;

    size_t copyToRAM(std::span<char> target) const {
        return copyToRAM(target.data(), target.size());
    }

    /// \returns substring of the current string, starting at the position \p pos, ending at the end of the string
    string_view_utf8 substr(size_t pos) const;

    /// Construct string_view_utf8 to provide data from CPU FLASH
    static string_view_utf8 MakeCPUFLASH(const uint8_t *utf8raw) {
        string_view_utf8 s;
        s.memory_ptr = utf8raw;
        return s;
    }

    inline static string_view_utf8 MakeCPUFLASH(const char *utf8raw) {
        return MakeCPUFLASH(reinterpret_cast<const uint8_t *>(utf8raw));
    }

    /// Construct string_view_utf8 to provide data from RAM
    /// basically the same as from CPU FLASH, only the string_view_utf8's type differs of course
    static string_view_utf8 MakeRAM(const uint8_t *utf8raw) {
        string_view_utf8 s;
        s.memory_ptr = utf8raw;
        return s;
    }

    static inline string_view_utf8 MakeRAM(const char *utf8raw) {
        return MakeRAM(reinterpret_cast<const uint8_t *>(utf8raw));
    }

    /// Construct string_view_utf8 to provide data from FILE
    /// The FILE *f shall aready be positioned to the spot, where the string starts
    static string_view_utf8 MakeFILE(::FILE *f, uint32_t offset) {
        string_view_utf8 s;
        if (f) {
            s.file = f;
            s.file_offset = offset;
        }
        return s;
    }

    /// Construct an empty string_view_utf8 - behaves like a "" but is a special type NULL
    static string_view_utf8 MakeNULLSTR() {
        return string_view_utf8();
    }

    string_view_utf8 &operator=(const string_view_utf8 &other) = default;

    /// Use is_same_ref instead
    bool operator==(const string_view_utf8 &other) const = delete;

    /// Use !is_same_ref instead
    bool operator!=(const string_view_utf8 &other) const = delete;

    /// string view has the same resource
    bool is_same_ref(const string_view_utf8 &other) const {
        return (file == other.file) && (memory_ptr == other.memory_ptr);
    }
};
static_assert(std::is_trivially_copyable_v<string_view_utf8>);

/// Class for reading the characters of a string_view_utf8
class StringReaderUtf8 {
    using Type = string_view_utf8::Type;

public:
    /// \param view string to be read (reader makes a copy, it does not need to exist during the reader existence)
    explicit StringReaderUtf8(const string_view_utf8 &view)
        : view_(view) {
    }

    /// No copying, no moving, just reading :)
    StringReaderUtf8(const StringReaderUtf8 &) = delete;

public:
    /// \returns remaining string that hasn't been read yet
    inline string_view_utf8 remaining_string() const {
        return view_;
    }

    /// @returns one UTF-8 character from the input data
    /// and advances internal pointers (in derived classes) to the next one
    unichar getUtf8Char();

    /// \returns one byte from source media and advances internal read ptr, implementation defined in derived classes
    /// The caller of this function makes sure it does not get called repeatedly after returning the end of input data.
    /// \returns 0 in case of end of input data or an error
    uint8_t getbyte();

private:
    uint8_t FILE_getbyte();

private:
    string_view_utf8 view_;
    uint8_t last_read_byte_ = 0xff;
};
