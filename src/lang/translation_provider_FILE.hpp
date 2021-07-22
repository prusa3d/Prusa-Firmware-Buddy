#pragma once

#include "translation_provider.hpp"
#include <stdio.h>
#include <string.h>
#include "gettext_string_hash.hpp"

//TODO fix including from string.h
extern "C" size_t strlcpy(char *dst, const char *src, size_t dsize);

class FILEtranslationProvider : public ITranslationProvider {

private:
    bool findMessage(const char *key, FILE *file) const {
        char messageBuffer[1024];
        uint32_t numOfStrings = 0;
        fseek(file, 8, SEEK_SET);
        fread(&numOfStrings, 4, 1, file);

        int32_t posInTable = 0;
        int32_t transTable = 0;
        fread(&posInTable, 4, 1, file);
        fread(&transTable, 4, 1, file);

        for (uint32_t i = 0; i < numOfStrings; ++i, posInTable += 8) {
            uint32_t len = 0;
            int32_t pos = 0;

            fseek(file, posInTable, SEEK_SET);

            //read len and pos of next string
            fread(&len, 4, 1, file);
            fread(&pos, 4, 1, file);

            fseek(file, pos, SEEK_SET);

            fread(&messageBuffer, 1, len, file);

            if (len != 0 && strncmp(messageBuffer, key, len) == 0) {
                fseek(file, transTable + (i * 8) + 4, SEEK_SET);
                fread(&pos, 4, 1, file);
                fseek(file, pos, SEEK_SET);
                return true;
            }
        }
        return false;
    }

    char m_Path[16];
    FILE *m_File = nullptr;

public:
    FILEtranslationProvider(const char *path) {
        strlcpy(m_Path, path, sizeof(m_Path));
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
        return m_Path;
    }
};

extern FILEtranslationProvider fileProvider;

//how does the hash table works
/* This should be explained:
     Each string has an associate hashing value V, computed by a fixed
     function.  To locate the string we use open addressing with double
     hashing.  The first index will be V % M, where M is the size of the
     hashing table.  If no entry is found, iterating with a second,
     independent hashing function takes place.  This second value will
     be 1 + V % (M - 2).
     The approximate number of probes will be

       for unsuccessful search:  (1 - N / M) ^ -1
       for successful search:    - (N / M) ^ -1 * ln (1 - N / M)

     where N is the number of keys.

     If we now choose M to be the next prime bigger than 4 / 3 * N,
     we get the values
                         4   and   1.85  resp.
     Because unsuccessful searches are unlikely this is a good value.
     Formulas: [Knuth, The Art of Computer Programming, Volume 3,
                Sorting and Searching, 1973, Addison Wesley]  */