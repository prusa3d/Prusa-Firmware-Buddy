#pragma once

#include <cstdint>
#include <cstddef>

/// Hash (search) table implementation - tailored for our domain (beware, it is not meant to be a generic solution to the problems of the whole universe)
/// It has some amount of buckets, e.g. 256
/// Each bucket is represented by a begin-end (min-max) pair representing the range of bucket items.
/// If the begin==end-1, the bucket contains only one item.
/// If there are more, one has to find the correct item being searched for.
/// This is now done by simple sequential search (complexity O(n))? Why? Because there actually very few items (two or three) in the buckets,
/// so going through them sequentially is much more cache-friendly than doing binary search.
/// To distinguish among hash collisions each item has recorded 2 first letters - these are being checked upon lookup.
/// Finding the correct string is based on these first two letters.
/// It turned to be sufficient for our domain.
template <typename HASH, uint32_t buckets = 256, uint32_t maxStrings = 256>
class string_hash_table { // @@TODO vymyslet tomu lepsi jmeno
public:
    struct BucketRange {
        uint16_t begin, end;
        inline constexpr BucketRange()
            : begin(0xffff)
            , end(0xffff) {}
        inline constexpr BucketRange(uint16_t begin, uint16_t end)
            : begin(begin)
            , end(end) {}
    };

#ifndef TRANSLATIONS_UNITTEST
    static const
#endif
        BucketRange hash_table[buckets];

    struct BucketItem {
        uint16_t firstLetters; ///< first 2 letters of string
        uint16_t stringIndex; ///< index of string in string table
        inline constexpr BucketItem()
            : firstLetters(0)
            , stringIndex(0) {}
        inline constexpr BucketItem(uint16_t firstLetters, uint16_t stringIndex)
            : firstLetters(firstLetters)
            , stringIndex(stringIndex) {}
    };

    /// The size may be limited exactly on the amount of strings available, in our case ~163 and some more will come later
    /// Definitely there won't be more than 64K strings
#ifndef TRANSLATIONS_UNITTEST
    static const
#endif
        BucketItem stringRecArray[maxStrings];

public:
    string_hash_table() {}

    /// Searches for the input string represented by the key parameter
    /// @returns the index of the string in a string table (translation)
    uint16_t find(const uint8_t *key) const {
        uint32_t hashIndex = ReducedHash(Hash(key));
        const BucketRange &b = hash_table[hashIndex];
        if (b.begin == 0xffffU) {
            return 0xffffU; // string not found, directs to an empty bucket
        }
        uint16_t first2Chars = (key[1] << 8) + key[0];
        if ((b.begin + 1) == b.end) { // no collision - remember - end is one element past the
            // we may silently expect the string is there, but let's check the first 2 letters as well
            return (stringRecArray[b.begin].firstLetters == first2Chars) ? stringRecArray[b.begin].stringIndex : 0xffffU;
        } else { // multiple strings in this bucket, we must find the correct one upon the first 2 characters
            // we assume there are few strings in the bucket, so instead of preparing a huge infrastructure for binary searching,
            // running through the array of "4B integers" may be more cache-friendly and thus faster
            for (size_t i = b.begin; i < b.end; ++i) {
                if (stringRecArray[i].firstLetters == first2Chars) {
                    return stringRecArray[i].stringIndex;
                }
            }
            return 0xffffU; // not found
        }
    }
    inline static constexpr uint32_t MaxStrings() { return maxStrings; }
    inline static constexpr uint32_t Buckets() { return buckets; }

    /// @returns hash function reduced to our number of buckets
    static uint32_t ReducedHash(uint32_t h) {
        return h % buckets;
    }

    /// @returns hash function of the input string key
    static uint32_t Hash(const uint8_t *key) {
        HASH h;
        return h(key);
    }
};
