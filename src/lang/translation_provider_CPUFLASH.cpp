#include "string_view_utf8.hpp"
#include "translation_provider.hpp"
#include "string_hash.hpp"
#include "translator.hpp"
#include "translation_provider_CPUFLASH.hpp"

#ifdef _DEBUG
    #define SKIP_SOME_TRANSLATIONS
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
    struct StringTableCS {
    // this will get statically precomputed for each translation language separately
    static const uint16_t stringBegins[];
    // a piece of memory where the null-terminated strings are situated
    static const uint8_t utf8Raw[];
};

struct StringTableDE {
    // this will get statically precomputed for each translation language separately
    static const uint16_t stringBegins[];
    // a piece of memory where the null-terminated strings are situated
    static const uint8_t utf8Raw[];
};

// intentionally disable additional translations in debug mode (to fit within FLASH space)
#ifndef SKIP_SOME_TRANSLATIONS
struct StringTableES {
    // this will get statically precomputed for each translation language separately
    static const uint16_t stringBegins[];
    // a piece of memory where the null-terminated strings are situated
    static const uint8_t utf8Raw[];
};

struct StringTableFR {
    // this will get statically precomputed for each translation language separately
    static const uint16_t stringBegins[];
    // a piece of memory where the null-terminated strings are situated
    static const uint8_t utf8Raw[];
};

struct StringTableIT {
    // this will get statically precomputed for each translation language separately
    static const uint16_t stringBegins[];
    // a piece of memory where the null-terminated strings are situated
    static const uint8_t utf8Raw[];
};

struct StringTablePL {
    // this will get statically precomputed for each translation language separately
    static const uint16_t stringBegins[];
    // a piece of memory where the null-terminated strings are situated
    static const uint8_t utf8Raw[];
};
#endif

using CPUFLASHTranslationProviderCS = CPUFLASHTranslationProvider<StringTableCS>;
using CPUFLASHTranslationProviderDE = CPUFLASHTranslationProvider<StringTableDE>;
#ifndef SKIP_SOME_TRANSLATIONS
using CPUFLASHTranslationProviderES = CPUFLASHTranslationProvider<StringTableES>;
using CPUFLASHTranslationProviderFR = CPUFLASHTranslationProvider<StringTableFR>;
using CPUFLASHTranslationProviderIT = CPUFLASHTranslationProvider<StringTableIT>;
using CPUFLASHTranslationProviderPL = CPUFLASHTranslationProvider<StringTablePL>;
#endif

// precomputed indices and strings for the CS(CZ) language
#include "stringBegins.cs.hpp"
#include "utf8Raw.cs.hpp"
template <>
const CPUFLASHTranslationProviderCS::RawData CPUFLASHTranslationProviderCS::rawData;

// precomputed indices and strings for the DE language
#include "stringBegins.de.hpp"
#include "utf8Raw.de.hpp"
template <>
const CPUFLASHTranslationProviderDE::RawData CPUFLASHTranslationProviderDE::rawData;

#ifndef SKIP_SOME_TRANSLATIONS
    // precomputed indices and strings for the ES language
    #include "stringBegins.es.hpp"
    #include "utf8Raw.es.hpp"
template <>
const CPUFLASHTranslationProviderES::RawData CPUFLASHTranslationProviderES::rawData;

    // precomputed indices and strings for the FR language
    #include "stringBegins.fr.hpp"
    #include "utf8Raw.fr.hpp"
template <>
const CPUFLASHTranslationProviderFR::RawData CPUFLASHTranslationProviderFR::rawData;

    // precomputed indices and strings for the IT language
    #include "stringBegins.it.hpp"
    #include "utf8Raw.it.hpp"
template <>
const CPUFLASHTranslationProviderIT::RawData CPUFLASHTranslationProviderIT::rawData;

    // precomputed indices and strings for the PL language
    #include "stringBegins.pl.hpp"
    #include "utf8Raw.pl.hpp"
template <>
const CPUFLASHTranslationProviderPL::RawData CPUFLASHTranslationProviderPL::rawData;
#endif

namespace {
static const CPUFLASHTranslationProviderCS cs;
ProviderRegistrator csReg("cs", &cs);

static const CPUFLASHTranslationProviderDE de;
ProviderRegistrator deReg("de", &de);

#ifndef SKIP_SOME_TRANSLATIONS
static const CPUFLASHTranslationProviderES es;
ProviderRegistrator esReg("es", &es);

static const CPUFLASHTranslationProviderFR fr;
ProviderRegistrator frReg("fr", &fr);

static const CPUFLASHTranslationProviderIT it;
ProviderRegistrator itReg("it", &it);

static const CPUFLASHTranslationProviderPL pl;
ProviderRegistrator plReg("pl", &pl);
#endif
}
