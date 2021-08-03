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

    /// gets offset of translation of key
    /// \param key to translated string
    /// \return 0 or positive number on success, -1 if translation is not found, -2 if a file error has occurred
    int32_t GetOffset(const char *key) const;

    /// initializes and checks validity of MO file
    /// \param file file pointer to a file
    /// \return true if file is valid and open, false otherwise
    bool Init(FILE *file);

private:
    /// finds index of key
    /// \param key which index to find
    /// \return positive number if key found, 0 if not, -1 if file error occurred
    int32_t getIndexOfKey(const char *key) const;

    /* Defines the so called `hashpjw' function by P.J. Weinberger
   [see Aho/Sethi/Ullman, COMPILERS: Principles, Techniques and Tools,
   1986, 1987 Bell Telephone Laboratories, Inc.]  */
    uint32_t hash_string(const char *key) const;

    ///returns index from pos in hashtable
    /// \param pos where to look for index
    /// \return positive number or zero on success, -1 if file error occurred
    int32_t getIndexOnPos(uint32_t pos) const;

    /// checks whether the string on index is the same as key
    /// \param index of the string
    /// \param key string to compare
    /// \return 1 if they match, 0 if not, -1 if error with file occurred
    int8_t checkString(uint32_t index, const char *key) const;

    mutable FILE *m_File = nullptr;
    mutable uint32_t m_HashSize = 0;
    mutable uint32_t m_HashOffset = 0;
    mutable uint32_t m_OrigOffset = 0;
    mutable uint32_t m_TransOffset = 0;
    mutable uint32_t m_NumOfString = 0;
};
