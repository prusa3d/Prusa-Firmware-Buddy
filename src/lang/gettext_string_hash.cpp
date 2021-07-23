//
// Created by machaviprace on 22.07.21.
//

#include "gettext_string_hash.hpp"
#include <string.h>
#include <array>

uint32_t gettext_hash_table::hash_string(const char *key) {
    uint32_t hval, g;
    const char *str = key;

    /* Compute the hash value for the given string.  */
    hval = 0;
    while (*str != '\0') {
        hval <<= 4;
        hval += (unsigned char)*str++;
        g = hval & ((unsigned long int)0xf << (HASHWORDBITS - 4));
        if (g != 0) {
            hval ^= g >> (HASHWORDBITS - 8);
            hval ^= g;
        }
    }
    return hval;
}

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

uint32_t gettext_hash_table::IndexOfKey(const char *key) {

    uint32_t hashVal = hash_string(key);
    uint32_t posInHash = hashVal % m_TableSize;

    uint32_t index = getIndexOnPos(posInHash);

    while (index != 0 && !checkString(index, key)) {
        uint32_t incr = 1 + (hashVal % (m_TableSize - 2));

        if (posInHash >= m_TableSize - incr) {
            posInHash -= m_TableSize - incr;
        } else {
            posInHash += incr;
        }
        index = getIndexOnPos(posInHash);
    }

    //    if (!checkString(index, key)) {
    //        uint32_t incr = 1 + (hashVal % (m_TableSize - 2));
    //        do {
    //            size_t cnt = 0;
    //            if (posInHash >= m_TableSize - incr) {
    //                posInHash -= m_TableSize - incr;
    //            } else {
    //                posInHash += incr;
    //            }
    //            index = getIndexOnPos(posInHash);
    //            cnt++;
    //        } while (!checkString(index, key) && cnt > 3);
    //        return 0;
    //    }
    return index;
}

/**
 * gets index from hash table from pos
 * @param pos where to get index
 * @return uint32 index from pos
 */
uint32_t gettext_hash_table::getIndexOnPos(uint32_t pos) {
    if (pos > m_TableSize) {
        return false;
    }
    fseek(m_File, m_HashOffset + 4 * pos, SEEK_SET);

    uint32_t indexOfString = 0;
    fread(&indexOfString, 4, 1, m_File);

    return indexOfString - 1;
}
bool gettext_hash_table::checkString(uint32_t index, const char *key) {
    char messageBuffer[m_BufferSize];
    uint32_t len = 0;
    int32_t pos = 0;

    fseek(m_File, m_StringOffset + (index * 8), SEEK_SET);

    //read len and pos of next string
    fread(&len, 4, 1, m_File);
    fread(&pos, 4, 1, m_File);

    fseek(m_File, pos, SEEK_SET);

    fread(&messageBuffer, 1, len, m_File);


    if (len != 0 && strncmp(messageBuffer, key, std::min(len, m_BufferSize)) == 0) {
        return true;
    }
    return false;
}
