#pragma once

#include "translation_provider.hpp"
#include <stdio.h>
#include <string.h>
#include "gettext_string_hash.hpp"

//TODO fix including from string.h
extern "C" size_t strlcpy(char *dst, const char *src, size_t dsize);

class FILEtranslationProvider : public ITranslationProvider {

    /// finds message with key in MO file
    /// \param key string to translate
    /// \param file file pointer to the file, does not matter to where
    /// \return offset of the message in file and the file pointer will point to the start of the message. If not found returns 0
    uint16_t findMessage(const char *key, FILE *file) const {

        uint32_t index = m_HashTable.GetIndexOfKey(key);

        //nothing is on index 0
        if (index == 0) {
            return 0;
        }

        //get to position of translated string
        fseek(file, m_TransTableOff + (8 * index) + 4, SEEK_SET);

        uint32_t posOfTrans = 0;
        if (fread(&posOfTrans, 4, 1, file) == 0) {
            return 0;
        }
        if (fseek(file, posOfTrans, SEEK_SET) != 0) {
            return 0;
        }
        return posOfTrans;
    }

    char m_Path[16];
    mutable FILE *m_File = nullptr;
    mutable gettext_hash_table m_HashTable;
    mutable uint32_t m_TransTableOff;

public:
    FILEtranslationProvider(const char *path) {
        strlcpy(m_Path, path, sizeof(m_Path));
    }

    ~FILEtranslationProvider() override = default;
    /// translates key according to MO file
    /// \param key string to translate
    /// \return translated string in string view, If translation is not found returns string view with original string
    string_view_utf8 GetText(const char *key) const override {

        //check if file is valid, if not try to open it again
        if (!m_File) {
            if (!OpenFile()) {
                return string_view_utf8::MakeCPUFLASH((const uint8_t *)key);
            }
        }

        //find translation for key, if not found return the original string
        uint16_t offset = 0;
        if ((offset = findMessage(key, m_File)) != 0) {
            return string_view_utf8::MakeFILE(m_File, offset);
        } else {
            return string_view_utf8::MakeCPUFLASH((const uint8_t *)key);
        }
    }

    /// tries to open the file added in constructor and also checks if it is valid MO file
    /// \return true if successfully opened and checked, false if anything fails
    bool OpenFile() const {
        m_File = fopen(m_Path, "rb");
        if (m_File) {
            //            check validity of MO file
            //check magick number
            uint32_t magicNumber = 0;
            if (fread(&magicNumber, 4, 1, m_File) == 0) {
                return false;
            }

            if (magicNumber != 0x950412de) {
                fclose(m_File);
                return false;
            }

            //check revision
            uint32_t revision = 1;
            if (fread(&revision, 4, 1, m_File) == 0) {
                return false;
            }

            if (revision != 0) {
                fclose(m_File);
                return false;
            }

            uint32_t origTableOf = 0;
            uint32_t transTableOf = 0;
            uint32_t hashTableOf = 0;
            uint32_t hashTableSize = 0;
            uint32_t numOfStrings = 0;

            if (fread(&numOfStrings, 4, 1, m_File) == 0) {
                return false;
            }
            if (fread(&origTableOf, 4, 1, m_File) == 0) {
                return false;
            }
            if (fread(&transTableOf, 4, 1, m_File) == 0) {
                return false;
            }
            if (fread(&hashTableSize, 4, 1, m_File) == 0) {
                return false;
            }
            if (fread(&hashTableOf, 4, 1, m_File) == 0) {
                return false;
            }

            m_TransTableOff = transTableOf;
            m_HashTable.init(m_File, hashTableSize, hashTableOf, origTableOf, numOfStrings);
            return true;
        }
        return false;
    }
};

extern FILEtranslationProvider fileProviderUSB;
extern FILEtranslationProvider fileProviderInternal;
