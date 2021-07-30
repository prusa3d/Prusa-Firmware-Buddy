//
// Created by machaviprace on 22.07.21.
//

#include "gettext_string_hash.hpp"
#include <string.h>

uint32_t gettext_hash_table::hash_string(const char *key) const {
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

uint32_t gettext_hash_table::GetIndexOfKey(const char *key) const {

    uint32_t hashVal = hash_string(key);
    uint32_t posInHash = hashVal % m_TableSize;
    uint32_t incr = 1 + (hashVal % (m_TableSize - 2));
    uint32_t index = 0;

    //get index from hash table
    for (index = getIndexOnPos(posInHash); index != 0; index = getIndexOnPos(posInHash)) {

        uint32_t index = getIndexOnPos(posInHash);

        if (index > m_NumOfString) {
            index -= m_NumOfString;
        }
        //check string if it matches return the index
        if (checkString(index, key)) {
            return index;
        }
        //if not rehash the string and check it again
        if (posInHash >= m_TableSize - incr) {
            posInHash -= m_TableSize - incr;
        } else {
            posInHash += incr;
        }
    }
    //return 0, only if no matching string is found
    return index;
}

/**
 * gets index from hash table from pos
 * @param pos where to get index
 * @return uint32 index from pos
 */
uint32_t gettext_hash_table::getIndexOnPos(uint32_t pos) const {

    if (pos > m_TableSize) {
        return false;
    }
    //move to position in hash table
    if (fseek(m_File, m_HashOffset + 4 * pos, SEEK_SET) != 0) {
        return 0;
    }

    //read index in position
    uint32_t indexOfString = 0;
    fread(&indexOfString, 4, 1, m_File);

    return indexOfString - 1;
}
bool gettext_hash_table::checkString(uint32_t index, const char *key) const {

    uint32_t strLen = 0;
    int32_t pos = 0;
    //move to position of length a nd offset of the string to check
    if (fseek(m_File, m_StringOffset + (index * 8), SEEK_SET) != 0) {
        return false;
    }
    //compute len of key
    uint32_t keyLen = strlen(key);

    //read len and pos of next string
    fread(&strLen, 4, 1, m_File);
    fread(&pos, 4, 1, m_File);

    //check if the lengths are equal
    if (strLen == 0 || strLen < keyLen) {
        return false;
    }

    //move to position of string to check
    if (fseek(m_File, pos, SEEK_SET)) {
        return false;
    }

    //check if the string matches
    //@@TODO: add build procedure to check minimal number of chars to check if the string are the same
    for (uint32_t i = 0; i < strLen; ++i) {
        char c;
        fread(&c, 1, 1, m_File);
        if (c != key[i]) {
            return false;
        }
    }
    return true;
}
void gettext_hash_table::init(FILE *file, uint32_t tableSize, uint32_t hashOffset, uint16_t stringOffset, uint32_t numOfString) {
    m_File = file;
    m_TableSize = tableSize;
    m_HashOffset = hashOffset;
    m_StringOffset = stringOffset;
    m_NumOfString = numOfString;
}
