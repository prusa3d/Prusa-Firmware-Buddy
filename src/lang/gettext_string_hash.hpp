//
// Created by machaviprace on 22.07.21.
//
#pragma once

#include <stdio.h>
#include <stdint.h>
#include "../common/timing.h"

#define HASHWORDBITS 32

//template <uint32_t TEST>
class CTracker {
public:
    CTracker(volatile int32_t *duration);
    ~CTracker();

private:
    volatile int32_t *Duration;
    uint32_t start = 0;
};
//template <uint32_t TEST>
//template <uint32_t TEST>

class gettext_hash_table {
public:
    gettext_hash_table(FILE *file, uint32_t tableSize, int32_t hashOffset, int32_t stringOffset, uint32_t numOfString)
        : m_File(file)
        , m_TableSize(tableSize)
        , m_HashOffset(hashOffset)
        , m_StringOffset(stringOffset)
        , m_NumOfString(numOfString) {};

    //returns position of string in hash table
    uint32_t GetIndexOfKey(const char *key);

    //returns next possible position for previous string

private:
    /* Defines the so called `hashpjw' function by P.J. Weinberger
   [see Aho/Sethi/Ullman, COMPILERS: Principles, Techniques and Tools,
   1986, 1987 Bell Telephone Laboratories, Inc.]  */
    uint32_t hash_string(const char *key);

    uint32_t getIndexOnPos(uint32_t pos);

    bool checkString(uint32_t index, const char *key);

    FILE *m_File;
    uint32_t m_TableSize;
    int32_t m_HashOffset;
    int32_t m_StringOffset;
    uint32_t m_NumOfString;

    static constexpr uint32_t m_BufferSize = 512;
};