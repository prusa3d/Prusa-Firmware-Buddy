//
// Created by machaviprace on 22.07.21.
//
#pragma once

#include <stdio.h>
#include <stdint.h>

class gettext_hash_table {
public:
    gettext_hash_table() = default;

    enum { TranslationNotFound = -1,
        FileErrorOccurred = -2 };

    /// gets offset of translation of key
    /// \param key to translated string
    /// \return 0 or positive number on success, TranslationNotFound if translation is not found, ErrorOccurred if a file error has occurred
    int32_t GetOffset(const char *key) const;

    /// initializes and checks validity of MO file
    /// \param file pointer to an MO file
    /// \return true if file is valid and open, false otherwise
    bool Init(FILE *file);

private:
    /// finds index of key
    /// \param key which index to find
    /// \return positive number if key found, 0 if not, -1 if file error occurred
    int32_t getIndexOfKey(const char *key) const;

    /// Defines the so called `hashpjw' function by P.J. Weinberger
    /// [see Aho/Sethi/Ullman, COMPILERS: Principles, Techniques and Tools, 1986, 1987 Bell Telephone Laboratories, Inc.]
    static uint32_t hash_string(const char *key);

    /// returns index from pos in hashtable
    ///  \param pos where to look for index
    ///  \return positive number or zero on success, -1 if file error occurred
    int32_t getIndexOnPos(uint32_t pos) const;

    /// checks whether the string on index is the same as key
    /// \param index of the string
    /// \param key string to compare
    /// \return 1 if they match, 0 if not, -1 if file error occurred
    int8_t checkString(uint32_t index, const char *key) const;

    mutable FILE *m_File = nullptr;

    struct HashTableRec {
        uint32_t m_NumOfString = 0;
        uint32_t m_OrigOffset = 0;
        uint32_t m_TransOffset = 0;
        uint32_t m_HashSize = 0;
        uint32_t m_HashOffset = 0;
    };

    static_assert(sizeof(HashTableRec) == 20, "StringRec size doesn't match its binary representation in an MO file");

    mutable HashTableRec hashTable;
};
