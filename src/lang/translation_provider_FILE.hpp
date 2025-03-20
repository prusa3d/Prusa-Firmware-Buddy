#pragma once

#include "translation_provider.hpp"
#include <stdio.h>
#include <string.h>
#include "gettext_string_hash.hpp"
#include "translator.hpp"

class FILETranslationProvider : public ITranslationProvider {

    char m_Path[128];
    mutable FILE *m_File = nullptr;
    mutable gettext_hash_table m_HashTable;
    mutable uint32_t m_TransTableOff;

public:
    FILETranslationProvider(const char *path);

    /// translates key according to MO file
    /// \param key string to translate
    /// \return translated string in string view, If translation is not found returns string view with original string
    string_view_utf8 GetText(const char *key) const override;

    /// tries to open the file added in constructor and also checks if it is valid MO file
    /// \return true if successfully opened and checked, false if anything fails
    bool EnsureFile() const;
};
