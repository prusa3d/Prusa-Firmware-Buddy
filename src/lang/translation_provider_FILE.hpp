#pragma once

#include "translation_provider.hpp"
#include <stdio.h>
#include <string.h>
#include "gettext_string_hash.hpp"

//TODO fix including from string.h
extern "C" size_t strlcpy(char *dst, const char *src, size_t dsize);

class FILEtranslationProvider : public ITranslationProvider {

    uint16_t findMessage(const char *key, FILE *file) const {

        uint32_t index = m_HashTable.GetIndexOfKey(key);

        //nothing is on index 0
        if (index == 0) {
            return false;
        }

        //get to position of translated string
        fseek(file, m_TransTableOff + (8 * index) + 4, SEEK_SET);

        uint32_t posOfTrans = 0;
        fread(&posOfTrans, 4, 1, file);
        fseek(file, posOfTrans, SEEK_SET);
        return posOfTrans;
    }

    char m_Path[16];
    FILE *m_File = nullptr;
    gettext_hash_table m_HashTable;
    uint32_t m_TransTableOff;

public:
    FILEtranslationProvider(const char *path) {
#ifdef TRANSLATIONS_UNITTEST
        strncpy(m_Path, path, sizeof(m_Path));
#else
        strlcpy(m_Path, path, sizeof(m_Path));
#endif
    }

    ~FILEtranslationProvider() override = default;
    string_view_utf8 GetText(const char *key) const override {

        //check if file is valid, if not try to open it again
        if (!m_File) {
            if (!(const_cast<FILEtranslationProvider *>(this)->OpenFile())) {
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

    bool OpenFile() {
        m_File = fopen(m_Path, "rb");
        if (m_File) {
            //            check validity of MO file
            uint32_t magicNumber = 0;
            fread(&magicNumber, 4, 1, m_File);
            if (magicNumber != 0x950412de && magicNumber != 0xde120495) {
                fclose(m_File);
                return false;
            }

            fseek(m_File, 8, SEEK_SET);
            uint32_t origTableOf = 0;
            uint32_t transTableOf = 0;
            uint32_t hashTableOf = 0;
            uint32_t hashTableSize = 0;
            uint32_t numOfStrings = 0;

            fread(&numOfStrings, 4, 1, m_File);
            fread(&origTableOf, 4, 1, m_File);
            fread(&transTableOf, 4, 1, m_File);
            fread(&hashTableSize, 4, 1, m_File);
            fread(&hashTableOf, 4, 1, m_File);

            m_TransTableOff = transTableOf;
            m_HashTable.init(m_File, hashTableSize, hashTableOf, origTableOf, numOfStrings);
        }
        return m_File;
    }
};

extern FILEtranslationProvider fileProviderUSB;
extern FILEtranslationProvider fileProviderInternal;
