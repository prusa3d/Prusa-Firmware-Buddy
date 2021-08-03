#pragma once

#include "translation_provider.hpp"
#include <stdio.h>
#include <string.h>
#include "gettext_string_hash.hpp"
#include "../common/file_raii.hpp"

//TODO fix including from string.h
extern "C" size_t strlcpy(char *dst, const char *src, size_t dsize);

class FILETranslationProvider : public ITranslationProvider {

    char m_Path[16];
    mutable FILE *m_File = nullptr;
    mutable gettext_hash_table m_HashTable;
    mutable uint32_t m_TransTableOff;

public:
    FILETranslationProvider(const char *path) {
        strlcpy(m_Path, path, sizeof(m_Path));
    }

    ~FILETranslationProvider() override = default;
    /// translates key according to MO file
    /// \param key string to translate
    /// \return translated string in string view, If translation is not found returns string view with original string
    string_view_utf8 GetText(const char *key) const override {

        //check if file is valid, if not try to open it again
        if (!EnsureFile()) {
            return string_view_utf8::MakeCPUFLASH((const uint8_t *)key);
        }
        FileRaii file(m_File);

        //find translation for key, if not found return the original string
        int32_t offset = m_HashTable.GetOffset(key);
        if (offset >= 0) {
            file.Release();
            return string_view_utf8::MakeFILE(m_File, offset);
        } else if (offset == -1) {
            file.Release();
            return string_view_utf8::MakeCPUFLASH((const uint8_t *)key);
        } else {
            m_File = nullptr;
            return string_view_utf8::MakeCPUFLASH((const uint8_t *)key);
        }
    }

    /// tries to open the file added in constructor and also checks if it is valid MO file
    /// \return true if successfully opened and checked, false if anything fails
    bool EnsureFile() const {

        //check if there is open file, if yes it must have been open with this function and is valid
        if (m_File) {
            return true;
        }
        m_File = fopen(m_Path, "rb");
        FileRaii file(m_File); //now we know that the FILE* is valid
        if (m_File) {
            if (!m_HashTable.Init(m_File)) {
                m_File = nullptr;
                return false;
            }
            file.Release();
            return true;
        }
        return false;
    }
};

extern FILETranslationProvider fileProviderUSB;
extern FILETranslationProvider fileProviderInternal;
