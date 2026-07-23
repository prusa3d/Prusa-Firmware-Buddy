#include "translation_provider_FILE.hpp"

#include <option/has_translations.h>
#include <option/enable_translation_cs.h>
#include <option/enable_translation_de.h>
#include <option/enable_translation_es.h>
#include <option/enable_translation_fr.h>
#include <option/enable_translation_it.h>
#include <option/enable_translation_pl.h>
#include <option/enable_translation_ja.h>
#include <option/enable_translation_uk.h>

FILETranslationProvider::FILETranslationProvider(const char *path)
    : m_Path { path } {
}

string_view_utf8 FILETranslationProvider::GetText(const char *key) const {
    // check if file is valid, if not try to open it again
    if (!EnsureFile()) {
        return string_view_utf8::MakeCPUFLASH(key);
    }

    // find translation for key, if not found return the original string
    int32_t offset = 0;
    switch (offset = m_HashTable.GetOffset(key)) {
    case gettext_hash_table::FileErrorOccurred:
        fclose(m_File);
        m_File = nullptr;
        [[fallthrough]];
    case gettext_hash_table::TranslationNotFound:
        return string_view_utf8::MakeCPUFLASH(key);
    default:
        return string_view_utf8::MakeFILE(m_File, offset);
    }
}

bool FILETranslationProvider::EnsureFile() const {
    // check if there is open file, if yes it must have been open with this function and is valid
    if (m_File != nullptr) {
        return true;
    }
    m_File = fopen(m_Path, "rb"); // now we know that the FILE* is valid
    if (m_File == nullptr) {
        return false;
    }

    // set file buffer to 64B -> most reads are very short, so it will be more efficient to have relatively small buffer.
    setvbuf(m_File, nullptr, _IOFBF, 64);

    if (!m_HashTable.Init(m_File)) {
        fclose(m_File);
        m_File = nullptr;
        return false;
    }

    return true;
}

#if HAS_TRANSLATIONS()
namespace {

    #if ENABLE_TRANSLATION_CS()
static const FILETranslationProvider cs("/system/res/lang/cs.mo");
ProviderRegistrator csReg("cs", &cs);
    #endif

    #if ENABLE_TRANSLATION_DE()
static const FILETranslationProvider de("/system/res/lang/de.mo");
ProviderRegistrator deReg("de", &de);
    #endif

    #if ENABLE_TRANSLATION_ES()
static const FILETranslationProvider es("/system/res/lang/es.mo");
ProviderRegistrator esReg("es", &es);
    #endif

    #if ENABLE_TRANSLATION_FR()
static const FILETranslationProvider fr("/system/res/lang/fr.mo");
ProviderRegistrator frReg("fr", &fr);
    #endif

    #if ENABLE_TRANSLATION_IT()
static const FILETranslationProvider it("/system/res/lang/it.mo");
ProviderRegistrator itReg("it", &it);
    #endif

    #if ENABLE_TRANSLATION_PL()
static const FILETranslationProvider pl("/system/res/lang/pl.mo");
ProviderRegistrator plReg("pl", &pl);
    #endif

    #if ENABLE_TRANSLATION_JA()
static const FILETranslationProvider ja("/system/res/lang/ja.mo");
ProviderRegistrator jaReg("ja", &ja);
    #endif

    #if ENABLE_TRANSLATION_UK()
static const FILETranslationProvider uk("/system/res/lang/uk.mo");
ProviderRegistrator ukReg("uk", &uk);
    #endif

} // namespace
#endif
