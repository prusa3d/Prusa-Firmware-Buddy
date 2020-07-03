#pragma once

#include "string_view_utf8.hpp"
#include "translation_provider.hpp"
#include "string_hash.hpp"
#include "translator.hpp"

/// hash function suitable for EN texts from http://www.cse.yorku.ca/~oz/hash.html
struct hash_djb2 {
    uint32_t operator()(const uint8_t *str) const {
        uint32_t hash = 5381;
        uint32_t c;
        while ((c = *str++) != 0) {
            hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
        }
        return hash;
    }
};

/// hash function suitable for EN texts from http://www.cse.yorku.ca/~oz/hash.html
struct hash_sdbm {
    uint32_t operator()(const uint8_t *str) const {
        uint32_t hash = 0;
        uint32_t c;
        while ((c = *str++) != 0) {
            hash = c + (hash << 6) + (hash << 16) - hash;
        }
        return hash;
    }
};

/// Base class for all translation providers which have their data in CPUFLASH
/// The idea here is, that all the derived providers share the same search (hash)
/// structures, thus greatly reducing total FLASH footprint.
/// That is also the main reason why we don't use raw GNU/gettext MO files.
/// Sharing of the search structures can be used, because the firmware is being compiled at once and all the translations
/// will share the same sources (POT/PO files).
class CPUFLASHTranslationProviderBase : public ITranslationProvider {
    using SHashTable = string_hash_table<hash_djb2, 128, 256>;

public:
    virtual ~CPUFLASHTranslationProviderBase() = default;
    /// @returns translated string
    virtual string_view_utf8 GetText(const char *key) const {
        uint16_t stringIndex = hash_table.find((const uint8_t *)key);
        const uint8_t *utf8raw = StringTableAt(stringIndex);
        return utf8raw ? string_view_utf8::MakeCPUFLASH(utf8raw) : string_view_utf8::MakeNULLSTR();
    }

#ifndef TRANSLATIONS_UNITTEST
protected:
#endif
    static SHashTable hash_table; ///< shared among all of the derived providers

    /// @returns pointer to translated string (utf8 data) or nullptr if out of range
    virtual const uint8_t *StringTableAt(uint16_t stringIndex) const = 0;
};

template <typename RD>
class CPUFLASHTranslationProvider : public CPUFLASHTranslationProviderBase {
    typedef RD RawData;
    static const RawData rawData;
#ifndef TRANSLATIONS_UNITTEST
protected:
#else
public:
#endif
    virtual const uint8_t *StringTableAt(uint16_t stringIndex) const {
        return
            //           ( stringIndex < (sizeof(rawData.stringBegins) / sizeof(rawData.stringBegins[0])) ) ?
            rawData.utf8Raw + rawData.stringBegins[stringIndex];
        //                : nullptr ;
    }
};
