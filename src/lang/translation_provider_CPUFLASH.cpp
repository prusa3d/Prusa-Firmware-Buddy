#include "string_view_utf8.hpp"
#include "translation_provider.hpp"
#include "string_hash.hpp"
#include "translator.hpp"
#include "translation_provider_CPUFLASH.hpp"

#ifdef _DEBUG
//    #define ENABLE_TRANSLATION_CZ
//    #define ENABLE_TRANSLATION_DE
//    #define ENABLE_TRANSLATION_ES
//    #define ENABLE_TRANSLATION_FR
//    #define ENABLE_TRANSLATION_IT
//    #define ENABLE_TRANSLATION_PL
#else
    #define ENABLE_TRANSLATION_CZ
    #define ENABLE_TRANSLATION_DE
    #define ENABLE_TRANSLATION_ES
    #define ENABLE_TRANSLATION_FR
    #define ENABLE_TRANSLATION_IT
    #define ENABLE_TRANSLATION_PL
#endif

using TPBSH = CPUFLASHTranslationProviderBase::SHashTable;
#ifndef TRANSLATIONS_UNITTEST
const
#endif
    TPBSH CPUFLASHTranslationProviderBase::hash_table;

#ifndef TRANSLATIONS_UNITTEST
template <>
const TPBSH::BucketRange TPBSH::hash_table[TPBSH::Buckets()] =
    #include "hash_table_buckets.ipp"

    template <>
    const TPBSH::BucketItem TPBSH::stringRecArray[TPBSH::MaxStrings()] =
    #include "hash_table_string_indices.ipp"
#endif

/// Wrappers of statically precomputed translation data for each language

#if defined(ENABLE_TRANSLATION_CZ) || defined(TRANSLATIONS_UNITTEST)
        struct StringTableCS {            // why the hell does clang-format indent this line so weird?
    static const uint16_t stringBegins[]; ///< this will get statically precomputed for each translation language separately
    static const uint8_t utf8Raw[];       ///< a piece of memory where the null-terminated strings are situated
};

using CPUFLASHTranslationProviderCS = CPUFLASHTranslationProvider<StringTableCS>;

    // precomputed indices and strings for the CS(CZ) language
    #include "stringBegins.cs.hpp"
    #include "utf8Raw.cs.hpp"
template <>
const CPUFLASHTranslationProviderCS::RawData CPUFLASHTranslationProviderCS::rawData;

namespace {
static const CPUFLASHTranslationProviderCS cs;
ProviderRegistrator csReg("cs", &cs);
}
#endif

#if defined(ENABLE_TRANSLATION_DE) || defined(TRANSLATIONS_UNITTEST)
struct StringTableDE {
    static const uint16_t stringBegins[]; ///< this will get statically precomputed for each translation language separately
    static const uint8_t utf8Raw[];       ///< a piece of memory where the null-terminated strings are situated
};

using CPUFLASHTranslationProviderDE = CPUFLASHTranslationProvider<StringTableDE>;

    // precomputed indices and strings for the DE language
    #include "stringBegins.de.hpp"
    #include "utf8Raw.de.hpp"
template <>
const CPUFLASHTranslationProviderDE::RawData CPUFLASHTranslationProviderDE::rawData;

namespace {
static const CPUFLASHTranslationProviderDE de;
ProviderRegistrator deReg("de", &de);
}
#endif

// intentionally disable additional translations in debug mode (to fit within FLASH space)
#if defined(ENABLE_TRANSLATION_ES) || defined(TRANSLATIONS_UNITTEST)
struct StringTableES {
    static const uint16_t stringBegins[]; ///< this will get statically precomputed for each translation language separately
    static const uint8_t utf8Raw[];       ///< a piece of memory where the null-terminated strings are situated
};

using CPUFLASHTranslationProviderES = CPUFLASHTranslationProvider<StringTableES>;

    // precomputed indices and strings for the ES language
    #include "stringBegins.es.hpp"
    #include "utf8Raw.es.hpp"
template <>
const CPUFLASHTranslationProviderES::RawData CPUFLASHTranslationProviderES::rawData;

namespace {
static const CPUFLASHTranslationProviderES es;
ProviderRegistrator esReg("es", &es);
}
#endif

#if defined(ENABLE_TRANSLATION_FR) || defined(TRANSLATIONS_UNITTEST)
struct StringTableFR {
    static const uint16_t stringBegins[]; ///< this will get statically precomputed for each translation language separately
    static const uint8_t utf8Raw[];       ///< a piece of memory where the null-terminated strings are situated
};

using CPUFLASHTranslationProviderFR = CPUFLASHTranslationProvider<StringTableFR>;

    // precomputed indices and strings for the FR language
    #include "stringBegins.fr.hpp"
    #include "utf8Raw.fr.hpp"
template <>
const CPUFLASHTranslationProviderFR::RawData CPUFLASHTranslationProviderFR::rawData;

namespace {
static const CPUFLASHTranslationProviderFR fr;
ProviderRegistrator frReg("fr", &fr);
}
#endif

#if defined(ENABLE_TRANSLATION_IT) || defined(TRANSLATIONS_UNITTEST)
struct StringTableIT {
    static const uint16_t stringBegins[]; ///< this will get statically precomputed for each translation language separately
    static const uint8_t utf8Raw[];       ///< a piece of memory where the null-terminated strings are situated
};

using CPUFLASHTranslationProviderIT = CPUFLASHTranslationProvider<StringTableIT>;

    // precomputed indices and strings for the IT language
    #include "stringBegins.it.hpp"
    #include "utf8Raw.it.hpp"
template <>
const CPUFLASHTranslationProviderIT::RawData CPUFLASHTranslationProviderIT::rawData;

namespace {
static const CPUFLASHTranslationProviderIT it;
ProviderRegistrator itReg("it", &it);
}
#endif

#if defined(ENABLE_TRANSLATION_PL) || defined(TRANSLATIONS_UNITTEST)
struct StringTablePL {
    static const uint16_t stringBegins[]; ///< this will get statically precomputed for each translation language separately
    static const uint8_t utf8Raw[];       ///< a piece of memory where the null-terminated strings are situated
};

using CPUFLASHTranslationProviderPL = CPUFLASHTranslationProvider<StringTablePL>;

    // precomputed indices and strings for the PL language
    #include "stringBegins.pl.hpp"
    #include "utf8Raw.pl.hpp"
template <>
const CPUFLASHTranslationProviderPL::RawData CPUFLASHTranslationProviderPL::rawData;

namespace {
static const CPUFLASHTranslationProviderPL pl;
ProviderRegistrator plReg("pl", &pl);
}
#endif
