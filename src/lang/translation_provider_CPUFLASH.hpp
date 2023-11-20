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

// obtain number of buckets from the python generator
#include "hash_table_buckets_count.ipp"

/// Base class for all translation providers which have their data in CPUFLASH
/// The idea here is, that all the derived providers share the same search (hash)
/// structures, thus greatly reducing total FLASH footprint.
/// That is also the main reason why we don't use raw GNU/gettext MO files.
/// Sharing of the search structures can be used, because the firmware is being compiled at once and all the translations
/// will share the same sources (POT/PO files).
class CPUFLASHTranslationProviderBase : public ITranslationProvider {
public:
    virtual ~CPUFLASHTranslationProviderBase() = default;
    /// @returns translated string
    virtual string_view_utf8 GetText(const char *key) const {
        uint16_t stringIndex = hash_table.find((const uint8_t *)key);
        if (stringIndex == 0xffff) {
            // if the translated string was not found, return the source string (assuming it is somewhere in memory, i.e. CPUFLASH or RAM)
            return string_view_utf8::MakeCPUFLASH((const uint8_t *)key);
        }
        const uint8_t *utf8raw = StringTableAt(stringIndex);
        return string_view_utf8::MakeCPUFLASH(utf8raw);
    }

    // bucket_count is being computed at compile time (lang.py is searching for the lowest possible number of buckets where collisions do not occur)
    // 355 is the maximum total number of strings the translator array can hold. To be increased in the future as new strings come into the FW
    using SHashTable = string_hash_table<hash_djb2, buckets_count, 1500>; ///< beware of low numbers of buckets - collisions may occur unexpectedly
#ifndef TRANSLATIONS_UNITTEST
protected:
#endif
#ifndef TRANSLATIONS_UNITTEST
    static const
#else
    static
#endif
        SHashTable hash_table; ///< shared among all of the derived providers

    /// @returns pointer to translated string (utf8 data) or nullptr if out of range
    virtual const uint8_t *StringTableAt(uint16_t stringIndex) const = 0;

#ifdef TRANSLATIONS_UNITTEST
    /// just accessors for raw data of derived classes for binary comparison inside unit tests
    virtual const uint16_t *StringBegins() const = 0;
    virtual const uint8_t *Utf8Raw() const = 0;
#endif
};

template <typename RD>
class CPUFLASHTranslationProvider : public CPUFLASHTranslationProviderBase {
#ifndef TRANSLATIONS_UNITTEST
private:
#else
public:
#endif
    typedef RD RawData;
    static const RawData rawData;
#ifndef TRANSLATIONS_UNITTEST
protected:
#else
public:
#endif
    virtual const uint8_t *StringTableAt(uint16_t stringIndex) const override {
        return
            //           ( stringIndex < (sizeof(rawData.stringBegins) / sizeof(rawData.stringBegins[0])) ) ?
            rawData.utf8Raw + rawData.stringBegins[stringIndex];
        //                : nullptr ;
    }
#ifdef TRANSLATIONS_UNITTEST
    /// just accessors for raw data of derived classes for binary comparison inside unit tests
    virtual const uint16_t *StringBegins() const override { return rawData.stringBegins; }
    virtual const uint8_t *Utf8Raw() const override { return rawData.utf8Raw; }
#endif
};
