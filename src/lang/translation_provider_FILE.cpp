#include "translation_provider_FILE.hpp"

extern "C" size_t strlcpy(char *dst, const char *src, size_t dsize);

//path cannot be longer than 16 characters
FILETranslationProvider fileProviderInternal("/internal/ts.mo");
FILETranslationProvider fileProviderUSB("/usb/lang/ts.mo");

FILETranslationProvider::FILETranslationProvider(const char *path) {
    strlcpy(m_Path, path, sizeof(m_Path));
}

string_view_utf8 FILETranslationProvider::GetText(const char *key) const {
    //check if file is valid, if not try to open it again
    if (!EnsureFile()) {
        return string_view_utf8::MakeCPUFLASH((const uint8_t *)key);
    }

    //find translation for key, if not found return the original string
    int32_t offset = 0;
    switch (offset = m_HashTable.GetOffset(key)) {
    case gettext_hash_table::FileErrorOccurred:
        fclose(m_File);
        m_File = nullptr;
        [[fallthrough]];
    case gettext_hash_table::TranslationNotFound:
        return string_view_utf8::MakeCPUFLASH((const uint8_t *)key);
    default:
        return string_view_utf8::MakeFILE(m_File, offset);
    }
}

bool FILETranslationProvider::EnsureFile() const {

    //check if there is open file, if yes it must have been open with this function and is valid
    if (m_File != nullptr) {
        return true;
    }
    FileRAII file(m_File = fopen(m_Path, "rb")); //now we know that the FILE* is valid
    if (m_File == nullptr)
        return false;

    if (!m_HashTable.Init(m_File)) {
        m_File = nullptr;
        return false;
    }
    file.Release();
    return true;
}
