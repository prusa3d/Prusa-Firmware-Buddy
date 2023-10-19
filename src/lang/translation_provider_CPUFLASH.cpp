#include "string_view_utf8.hpp"
#include "translation_provider.hpp"
#include "string_hash.hpp"
#include "translator.hpp"
#include "translation_provider_CPUFLASH.hpp"
#include "../../include/option/has_translations.h" // #include <option/has_translations.h> does not work

#ifdef TRANSLATIONS_UNITTEST
    #define ENABLE_TRANSLATION_CZ
    #define ENABLE_TRANSLATION_DE
    #define ENABLE_TRANSLATION_ES
    #define ENABLE_TRANSLATION_FR
    #define ENABLE_TRANSLATION_IT
    #define ENABLE_TRANSLATION_PL
#else
    #ifndef _DEBUG
        #if HAS_TRANSLATIONS()
            #define ENABLE_TRANSLATION_CZ
            #define ENABLE_TRANSLATION_DE
            #define ENABLE_TRANSLATION_ES
            #define ENABLE_TRANSLATION_FR
            #define ENABLE_TRANSLATION_IT
            #define ENABLE_TRANSLATION_PL
        #endif // HAS_TRANSLATIONS()
    #endif //!_DEBUG
#endif // TRANSLATIONS_UNITTEST

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

#ifdef ENABLE_TRANSLATION_CZ
        struct StringTableCS { // why the hell does clang-format indent this line so weird?
    static const uint16_t stringBegins[]; ///< this will get statically precomputed for each translation language separately
    static const uint8_t utf8Raw[]; ///< a piece of memory where the null-terminated strings are situated
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
} // namespace
#endif

#ifdef ENABLE_TRANSLATION_DE
struct StringTableDE {
    static const uint16_t stringBegins[]; ///< this will get statically precomputed for each translation language separately
    static const uint8_t utf8Raw[]; ///< a piece of memory where the null-terminated strings are situated
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
} // namespace
#endif

// intentionally disable additional translations in debug mode (to fit within FLASH space)
#ifdef ENABLE_TRANSLATION_ES
struct StringTableES {
    static const uint16_t stringBegins[]; ///< this will get statically precomputed for each translation language separately
    static const uint8_t utf8Raw[]; ///< a piece of memory where the null-terminated strings are situated
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
} // namespace
#endif

#ifdef ENABLE_TRANSLATION_FR
struct StringTableFR {
    static const uint16_t stringBegins[]; ///< this will get statically precomputed for each translation language separately
    static const uint8_t utf8Raw[]; ///< a piece of memory where the null-terminated strings are situated
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
} // namespace
#endif

#ifdef ENABLE_TRANSLATION_IT
struct StringTableIT {
    static const uint16_t stringBegins[]; ///< this will get statically precomputed for each translation language separately
    static const uint8_t utf8Raw[]; ///< a piece of memory where the null-terminated strings are situated
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
} // namespace
#endif

#ifdef ENABLE_TRANSLATION_PL
struct StringTablePL {
    static const uint16_t stringBegins[]; ///< this will get statically precomputed for each translation language separately
    static const uint8_t utf8Raw[]; ///< a piece of memory where the null-terminated strings are situated
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
} // namespace
#endif
