#include "catch2/catch.hpp"

// #include "string_view_utf8.hpp"
#include "translator.hpp"
#include "translation_provider_CPUFLASH.hpp"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <deque>
#include <set>
#include "hash.hpp"

using namespace std;

template <typename C>
int DetectDuplicateHashes(C &c) {
    stable_sort(c.begin(), c.end());
    auto i = unique(c.begin(), c.end());
    return distance(i, c.end());
}

template <typename REDUCE>
std::pair<int, int> NumberOfCollisions(REDUCE r) {
    // read keys from a file and compute the hash values
    // beware - keys.txt is a "normal" text file, but it MUST be excluded from pre-commit
    // otherwise the pre-commit script will strip the trailing spaces on some texts.
    // In this case doing so prevents the tests from running on the genuine raw texts from the FW.
    ifstream f("keys.txt");
    deque<uint32_t> hashes_djb2;
    deque<uint32_t> hashes_sdbm;
    do {
        string s;
        getline(f, s);
        hash_djb2 hdjb2;
        hash_sdbm hsdbm;
        hashes_djb2.emplace_back(r(hdjb2((const unsigned char *)s.c_str())));
        hashes_sdbm.emplace_back(r(hsdbm((const unsigned char *)s.c_str())));
    } while (f.good());

    // how many there are and are there any collisions?
    return make_pair(DetectDuplicateHashes(hashes_djb2), DetectDuplicateHashes(hashes_sdbm));
}

TEST_CASE("string_hash_table::raw_hash_collision_test", "[translator]") {
    auto collisions = NumberOfCollisions([](uint32_t h) { return h & ~0U; });
    CHECK(collisions.first == 0);
    CHECK(collisions.second == 0);
}

TEST_CASE("string_hash_table::limited_hash_collision_test", "[translator]") {
    // do the same as in raw_hash_collision_test but limit the number of hash buckets to some number of "bits"
    auto collisionsffff = NumberOfCollisions([](uint32_t h) { return h & 0xffffU; });
    auto collisionsfff = NumberOfCollisions([](uint32_t h) { return h & 0xfffU; });
    auto collisionsff = NumberOfCollisions([](uint32_t h) { return h & 0xffU; });
    auto collisionsf = NumberOfCollisions([](uint32_t h) { return h & 0xfU; });
    // just to prevent unused var warning
    (void)collisionsffff;
    (void)collisionsfff;
    (void)collisionsff;
    (void)collisionsf;

    // number of collisions:
    // mask    djb2  sdbm
    // ffff    0     1
    // fff     2     6
    // ff     41    47
    // f     147   147 - obviously - having less buckets than input data must make collisions
    // On our specific test case the hash function of djb2 gives fewer collisions ... it may be a conincidence
    // For real implementation of our first-level hashing we may use something like 256 buckets
    // Having 1-2 items in each bucket is reasonable

    // How about scaling the hashing function down?
    auto collisions64k = NumberOfCollisions([](uint32_t h) { return h >> 16; });
    auto collisions4k = NumberOfCollisions([](uint32_t h) { return h >> 20; });
    auto collisions256 = NumberOfCollisions([](uint32_t h) { return h >> 24; });
    auto collisions16 = NumberOfCollisions([](uint32_t h) { return h >> 28; });

    (void)collisions64k;
    (void)collisions4k;
    (void)collisions256;
    (void)collisions16;

    // interesting, the results are comparable
    // 64k    9    3
    // 4k    18    8
    // 256   55   46
    // 16   147  147
}

struct hash_intentional_collision_F {
    uint32_t operator()(const uint8_t * /*s*/) const {
        return 0xdeadbeef;
    }
};

struct String {
    uint32_t hash;
    string s;
    uint16_t index; // index of string in input table
    inline String(uint32_t hash, const string &s, uint16_t index)
        : hash(hash)
        , s(s)
        , index(index) {}
};

template <typename HASH, uint32_t buckets, uint32_t maxStrings>
bool FillHashClass(string_hash_table<HASH, buckets, maxStrings> &sh, const char *fname, deque<string> &rawStrings, bool skipCollisionCheck) {
    using SHTable = string_hash_table<HASH, buckets, maxStrings>;
    deque<String> workStrings;

    {
        ifstream f(fname);
        if (!f.is_open()) {
            return false;
        }
        uint16_t index = 0;
        do {
            string s;
            getline(f, s);
            PreprocessRawLineStrings(s);
            if (!s.empty()) { // beware of empty strings
                rawStrings.push_back(s); // make a copy of the string
                workStrings.emplace_back(String(SHTable::Hash((const unsigned char *)s.c_str()), s, index));
                REQUIRE(rawStrings.back() == s);
                ++index;
            }
        } while (f.good());
    }
    // sort the strings lexicographically
    stable_sort(workStrings.begin(), workStrings.end(), [](const String &s0, const String &s1) { return s0.s < s1.s; });
    // STABLE_sort the strings by their reduced hash value
    // this trick makes the strings fall into the correct buckets while being kept sorted in the buckets as well
    stable_sort(workStrings.begin(), workStrings.end(), [](const String &s0, const String &s1) {
        return SHTable::ReducedHash(s0.hash) < SHTable::ReducedHash(s1.hash);
    });
    // this may be already dumped into the string rec array
    uint32_t shi = 0;
    for_each(workStrings.cbegin(), workStrings.cend(), [&](const String &s) {
        REQUIRE(shi < sh.MaxStrings()); // just make sure we have enough space for all the strings
        sh.stringRecArray[shi].firstLetters = (s.s[1] << 8) + s.s[0];
        sh.stringRecArray[shi].stringIndex = s.index; // where it is in the input table
        ++shi;
    });
    // now walk over the strings deque and record the buckets' start and finish
    decltype(workStrings)::const_iterator begin = workStrings.cbegin(), b = begin, e = workStrings.cend();
    while (b != e) {
        // find the first element after b where the reduced hash does not equal to (that's the end of the bucket)
        auto ie = find_if_not(b, e, [&b](const String &s) { return SHTable::ReducedHash(s.hash) == SHTable::ReducedHash(b->hash); });
        sh.hash_table[SHTable::ReducedHash(b->hash)].begin = distance(begin, b);
        sh.hash_table[SHTable::ReducedHash(b->hash)].end = distance(begin, ie);

        if (!skipCollisionCheck) {
            // check, that we do not have collisions which were unsolvable by comparing the first 2 letters
            // i.e. in each bucket <b, ie) the first letters must be unique
            set<string> firstLetters;
            for_each(b, ie, [&firstLetters](const String &s) { firstLetters.insert(s.s.substr(0, 2)); });
            // If this is not true, we have a collision which cannot be solved just by the first 2 letters
            // In such a case one must either enlarge the search hash map (more buckets) or do some other changes
            REQUIRE((long)firstLetters.size() == distance(b, ie));
        }
        b = ie; // move onto the next bucket
    }
    return true;
}

template <typename HASH, uint32_t buckets, uint32_t maxStrings>
bool CheckHashClass() {
    // read all the strings from the input file
    // compute their hashes, sort them into buckets, serialize the buckets into the arrays
    using SHTable = string_hash_table<HASH, buckets, maxStrings>;
    SHTable sh;
    deque<string> rawStrings; // this is just for verifying the algorithm later on - need to have the raw strings as they came from the file

    // warning - deliberately skips collision tests
    REQUIRE(FillHashClass<HASH, buckets, maxStrings>(sh, "keys.txt", rawStrings, true));

    // now the hash table is filled with data, let's query it ;)
    // every string must be found in the hash table
    for_each(rawStrings.begin(), rawStrings.end(), [&](const string &s) {
        REQUIRE(sh.find((const uint8_t *)s.c_str()) != 0xffffU);
    });

    // negative test
    REQUIRE(sh.find((const uint8_t *)"tento text tam neni") == 0xffffU);
    return true;
}

using TPBSH = CPUFLASHTranslationProviderBase::SHashTable;

TEST_CASE("string_hash_table::make_hash_table_256_buckets", "[translator]") {
    REQUIRE(CheckHashClass<hash_djb2, 256, TPBSH::MaxStrings()>());
}
TEST_CASE("string_hash_table::make_hash_table_128_buckets", "[translator]") {
    REQUIRE(CheckHashClass<hash_djb2, 128, TPBSH::MaxStrings()>());
}
TEST_CASE("string_hash_table::make_hash_table_100_buckets", "[translator]") {
    REQUIRE(CheckHashClass<hash_djb2, 100, TPBSH::MaxStrings()>());
}
TEST_CASE("string_hash_table::make_hash_table_96_buckets", "[translator]") {
    REQUIRE(CheckHashClass<hash_djb2, 96, TPBSH::MaxStrings()>());
}
TEST_CASE("string_hash_table::make_hash_table_64_buckets", "[translator]") {
    REQUIRE(CheckHashClass<hash_djb2, 64, TPBSH::MaxStrings()>());
}

// intentional creation of hash collision and handling it
TEST_CASE("string_hash_table::make_hash_table_intentional_collision", "[translator]") {
    using SHTable = string_hash_table<hash_intentional_collision_F, 64, 2>;
    SHTable sh;
    // there will be 2 strings
    static const char str1[] = "123456";
    static const char str2[] = "134567";

    sh.stringRecArray[0].firstLetters = (str1[1] << 8) + str1[0];
    sh.stringRecArray[0].stringIndex = 0; // where it is in the input table

    sh.stringRecArray[1].firstLetters = (str2[1] << 8) + str2[0];
    sh.stringRecArray[1].stringIndex = 1; // where it is in the input table

    uint8_t collidingBucket = SHTable::ReducedHash(SHTable::Hash(nullptr)); // can be nullptr, the hash returns a constant

    sh.hash_table[collidingBucket].begin = 0;
    sh.hash_table[collidingBucket].end = 2;

    REQUIRE(sh.find((const uint8_t *)str1) == 0);
    REQUIRE(sh.find((const uint8_t *)str2) == 1);
    REQUIRE(sh.find((const uint8_t *)"unknown") == 0xffff);
}

bool FillHashTableCPUFLASHProvider(CPUFLASHTranslationProviderBase::SHashTable &ht, const char *fname, std::deque<string> &rawStrings) {
    return FillHashClass(ht, fname, rawStrings, false); // just to hide the template FillHashClass within this source file
}

void FindAndReplaceAll(string &data, string toSearch, string replaceStr) {
    // copied from: https://thispointer.com/find-and-replace-all-occurrences-of-a-sub-string-in-c/
    // Get the first occurrence
    size_t pos = data.find(toSearch);
    // Repeat till end is reached
    while (pos != std::string::npos) {
        // Replace this occurrence of Sub String
        data.replace(pos, toSearch.size(), replaceStr);
        // Get the next occurrence from the current position
        pos = data.find(toSearch, pos + replaceStr.size());
    }
}

void PreprocessRawLineStrings(string &l) {
    // must convert the '\n' into \xa here
    FindAndReplaceAll(l, string("\\n"), string("\xa"));
    // must convert the "\"" sequence into a single character '"' here
    FindAndReplaceAll(l, string("\\\""), string("\""));
    // 0x7f symbol for degrees is a similar case
    FindAndReplaceAll(l, string("\\177"), string("\177"));
}
