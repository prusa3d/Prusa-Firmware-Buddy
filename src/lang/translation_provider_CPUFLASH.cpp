#include "string_view_utf8.hpp"
#include "translation_provider.hpp"
#include "string_hash.hpp"
#include "translator.hpp"
#include "translation_provider_CPUFLASH.hpp"

CPUFLASHTranslationProviderBase::SHashTable CPUFLASHTranslationProviderBase::hash_table;

/// Wrappers of statically precomputed translation data for each language
struct StringTableCS {
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

struct StringTableDE {
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

using CPUFLASHTranslationProviderCS = CPUFLASHTranslationProvider<StringTableCS>;
using CPUFLASHTranslationProviderPL = CPUFLASHTranslationProvider<StringTablePL>;
using CPUFLASHTranslationProviderES = CPUFLASHTranslationProvider<StringTableES>;
using CPUFLASHTranslationProviderFR = CPUFLASHTranslationProvider<StringTableFR>;
using CPUFLASHTranslationProviderDE = CPUFLASHTranslationProvider<StringTableDE>;
using CPUFLASHTranslationProviderIT = CPUFLASHTranslationProvider<StringTableIT>;

// this will be filled at compile time by precomputed indices and strings
const uint16_t StringTableCS::stringBegins[] = { 1, 2, 3, 4 };
const uint8_t StringTableCS::utf8Raw[] = "abcd";
template <>
const CPUFLASHTranslationProviderCS::RawData CPUFLASHTranslationProviderCS::rawData;

const uint16_t StringTableDE::stringBegins[] = { 1, 2, 3, 4 };
const uint8_t StringTableDE::utf8Raw[] = "abcd";
template <>
const CPUFLASHTranslationProviderDE::RawData CPUFLASHTranslationProviderDE::rawData;

const uint16_t StringTableES::stringBegins[] = { 1, 2, 3, 4 };
const uint8_t StringTableES::utf8Raw[] = "abcd";
template <>
const CPUFLASHTranslationProviderES::RawData CPUFLASHTranslationProviderES::rawData;

const uint16_t StringTableFR::stringBegins[] = { 1, 2, 3, 4 };
const uint8_t StringTableFR::utf8Raw[] = "abcd";
template <>
const CPUFLASHTranslationProviderFR::RawData CPUFLASHTranslationProviderFR::rawData;

const uint16_t StringTableIT::stringBegins[] = { 1, 2, 3, 4 };
const uint8_t StringTableIT::utf8Raw[] = "abcd";
template <>
const CPUFLASHTranslationProviderIT::RawData CPUFLASHTranslationProviderIT::rawData;

const uint16_t StringTablePL::stringBegins[] = { 1, 2, 3, 4 };
const uint8_t StringTablePL::utf8Raw[] = "abcd";
template <>
const CPUFLASHTranslationProviderPL::RawData CPUFLASHTranslationProviderPL::rawData;

namespace {
CPUFLASHTranslationProviderCS cs;
ProviderRegistrator csReg("cs", &cs);

CPUFLASHTranslationProviderDE de;
ProviderRegistrator deReg("de", &de);

CPUFLASHTranslationProviderES es;
ProviderRegistrator esReg("es", &es);

CPUFLASHTranslationProviderFR fr;
ProviderRegistrator frReg("fr", &fr);

CPUFLASHTranslationProviderIT it;
ProviderRegistrator itReg("it", &it);

CPUFLASHTranslationProviderPL pl;
ProviderRegistrator plReg("pl", &pl);
}
