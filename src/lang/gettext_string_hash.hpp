//
// Created by machaviprace on 22.07.21.
//
#pragma once

#include <stdio.h>
#include <stdint.h>

#define HASHWORDBITS 32

class gettext_hash_table {
public:
    gettext_hash_table() = default;
    /**
 * finds index of translated string
 * @param key string which to find index of translated string
 * @return 0 if not found, nonzero value on success
 */
    uint32_t GetIndexOfKey(const char *key) const;
    /**
 * initializes the hash table
 * @param file poiter to MO file
 * @param tableSize size of the table
 * @param hashOffset
 * @param stringOffset
 * @param numOfString
 */
    void init(FILE *file, uint32_t tableSize, uint32_t hashOffset, uint16_t stringOffset, uint32_t numOfString);

private:
    /* Defines the so called `hashpjw' function by P.J. Weinberger
   [see Aho/Sethi/Ullman, COMPILERS: Principles, Techniques and Tools,
   1986, 1987 Bell Telephone Laboratories, Inc.]  */
    uint32_t hash_string(const char *key) const;

    /**
     * gets index on position from hashtable
     * @param pos where to get index
     * @return uint32 index from pos
    */
    uint32_t getIndexOnPos(uint32_t pos) const;

    /**
 * checks whether the string on index is the same as key
 * @param index of the string
 * @param key string to compare
 * @return true if they match or false
 */
    bool checkString(uint32_t index, const char *key) const;

    FILE *m_File = nullptr;
    uint32_t m_TableSize = 0;
    uint32_t m_HashOffset = 0;
    uint32_t m_StringOffset = 0;
    uint32_t m_NumOfString = 0;

    static constexpr uint32_t m_BufferSize = 512;
};
