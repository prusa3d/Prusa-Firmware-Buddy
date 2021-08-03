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
        hval += (uint8_t)*str++;
        g = hval & ((uint32_t)0xf << (HASHWORDBITS - 4));
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

int32_t gettext_hash_table::getIndexOfKey(const char *key) const {

    uint32_t hashVal = hash_string(key);
    uint32_t posInHash = hashVal % m_HashSize;
    uint32_t incr = 1 + (hashVal % (m_HashSize - 2));
    uint32_t index = 0;

    //get index from hash table
    for (index = getIndexOnPos(posInHash); index > 0; index = getIndexOnPos(posInHash)) {

        index--;

        if (index > m_NumOfString) {
            index -= m_NumOfString;
        }

        //check string if it matches or error with file occurred return comparison result
        if (checkString(index, key) != 0) {
            return index;
        }

        //if not rehash the string and check it again
        if (posInHash >= m_HashSize - incr) {
            posInHash -= m_HashSize - incr;
        } else {
            posInHash += incr;
        }
    }
    //return 0, only if no matching string is found
    return index;
}

int32_t gettext_hash_table::getIndexOnPos(uint32_t pos) const {

    if (pos > m_HashSize) {
        return 0;
    }
    //move to position in hash table
    if (fseek(m_File, m_HashOffset + 4 * pos, SEEK_SET) != 0) {
        return -1;
    }

    //read index in position
    uint32_t indexOfString = 0;
    if (fread(&indexOfString, 4, 1, m_File) != 1) {
        return -1;
    }

    return indexOfString;
}
int8_t gettext_hash_table::checkString(uint32_t index, const char *key) const {

    uint32_t strLen = 0;
    int32_t pos = 0;
    //move to position of length a nd offset of the string to check
    if (fseek(m_File, m_OrigOffset + (index * 8), SEEK_SET) != 0) {
        return -1;
    }
    //compute len of key
    uint32_t keyLen = strlen(key);

    //read len and pos of next string
    if (fread(&strLen, 4, 1, m_File) != 1 || fread(&pos, 4, 1, m_File) != 1) {
        return -1;
    }

    //check if the lengths are equal
    if (strLen == 0 || strLen < keyLen) {
        return 0;
    }

    //move to position of string to check
    if (fseek(m_File, pos, SEEK_SET) != 0) {
        return -1;
    }

    //check if the string matches
    //@@TODO: add build procedure to check minimal number of chars to check if the string are the same
    for (uint32_t i = 0; i < strLen; ++i) {
        char c;
        if (fread(&c, 1, 1, m_File) != 1) {
            return -1;
        }
        if (c != key[i]) {
            return 0;
        }
    }
    return 1;
}
bool gettext_hash_table::Init(FILE *file) {

    m_File = file;

    //check validity of MO file

    //check magick number
    uint32_t magicNumber = 0;
    if (fread(&magicNumber, 4, 1, m_File) != 1) {
        return false;
    }

    if (magicNumber != 0x950412de) {
        return false;
    }

    //check revision
    uint32_t revision = 1;
    if (fread(&revision, 4, 1, m_File) != 1) {
        return false;
    }

    if (revision != 0) {
        return false;
    }

    if (fread(&m_NumOfString, 4, 1, m_File) != 1) {
        return false;
    }
    if (fread(&m_OrigOffset, 4, 1, m_File) != 1) {
        return false;
    }
    if (fread(&m_TransOffset, 4, 1, m_File) != 1) {
        return false;
    }
    if (fread(&m_HashSize, 4, 1, m_File) != 1) {
        return false;
    }
    if (fread(&m_HashOffset, 4, 1, m_File) != 1) {
        return false;
    }
    return true;
}
int32_t gettext_hash_table::GetOffset(const char *key) const {

    int32_t index = getIndexOfKey(key);
    if (index <= 0) {
        //0 = string not found, -1=error while reading file
        //shift it by -1, because we can have offset 0
        return index - 1;
    }

    //get to position of translated string
    if (fseek(m_File, m_TransOffset + (8 * index) + 4, SEEK_SET) != 0) {
        return -2;
    }

    int32_t posOfTrans = 0;
    if (fread(&posOfTrans, 4, 1, m_File) != 1) {
        return -2;
    }
    if (fseek(m_File, posOfTrans, SEEK_SET) != 0) {
        return -2;
    }
    return posOfTrans;
}
