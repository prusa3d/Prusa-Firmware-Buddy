#pragma once

#include "translation_provider.hpp"
#include <stdio.h>
#include <string.h>
#include "gettext_string_hash.hpp"

//TODO fix including from string.h
extern "C" size_t strlcpy(char *dst, const char *src, size_t dsize);

class FILEtranslationProvider : public ITranslationProvider {

public:
    bool findMessage(const char *key, FILE *file) const {
        fseek(file, 8, SEEK_SET);

        int32_t origTableOf = 0;
        int32_t transTableOf = 0;
        int32_t hashTableOf = 0;
        uint32_t hashTableSize = 0;
        uint32_t numOfStrings = 0;

        fread(&numOfStrings, 4, 1, file);
        fread(&origTableOf, 4, 1, file);
        fread(&transTableOf, 4, 1, file);
        fread(&hashTableSize, 4, 1, file);
        fread(&hashTableOf, 4, 1, file);

        static gettext_hash_table hashTable(file, hashTableSize, hashTableOf, origTableOf, numOfStrings);

        uint32_t index = hashTable.GetIndexOfKey(key);

        //nothing is on index 0
        if (index == 0) {
            return false;
        }

        //get to position of translated string
        fseek(file, transTableOf + (8 * index) + 4, SEEK_SET);

        int32_t posOfTrans = 0;
        fread(&posOfTrans, 4, 1, file);
        fseek(file, posOfTrans, SEEK_SET);
        return true;
    }

    char m_Path[16];
    FILE *m_File = nullptr;

public:
    FILEtranslationProvider(const char *path) {

        strncpy(m_Path, path, sizeof(m_Path));
        //        strlcpy(m_Path, path, sizeof(m_Path));
    }

    ~FILEtranslationProvider() override = default;
    string_view_utf8 GetText(const char *key) const override {

        //@@TODO recover from file error

        if (!m_File) {

            return string_view_utf8::MakeCPUFLASH((const uint8_t *)key);
        }

        if (findMessage(key, m_File)) {
            return string_view_utf8::MakeFILE(m_File);
        } else {
            return string_view_utf8::MakeCPUFLASH((const uint8_t *)key);
        }
    }

    bool OpenFile() {
        m_File = fopen(m_Path, "rb");
        return m_File;
    }
};

extern FILEtranslationProvider fileProvider;
