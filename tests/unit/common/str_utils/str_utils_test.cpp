/// str_utils tests

#include <string.h>
#include <iostream>

#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"
using Catch::Matchers::Equals;

#include "str_utils.cpp"
#include "str_utils.h"

TEST_CASE("Delete string", "[strdel]") {
    static constexpr char text[12] = "abcdXYZefgh";
    char str[255] = "abcdXYZefgh";
    char *nostr = nullptr;
    size_t n;

    SECTION("void input") {
        n = strdel(nostr);
        REQUIRE(n == 0);
        REQUIRE(nostr == nullptr);
    }

    SECTION("delete 0 chars") {
        n = strdel(str, 0);
        REQUIRE(n == 0);
        REQUIRE(0 == strcmp(str, text));
    }

    SECTION("empty string") {
        strcpy(str, "");
        n = strdel(str);
        REQUIRE(n == 0);
        REQUIRE(0 == strcmp(str, ""));
    }

    SECTION("single char at beginning") {
        n = strdel(str);
        REQUIRE(n == 1);
        REQUIRE(0 == strcmp(str, "bcdXYZefgh"));
    }

    SECTION("single char inside") {
        n = strdel(str + 4, 1);
        REQUIRE(n == 1);
        REQUIRE(0 == strcmp(str, "abcdYZefgh"));
    }

    SECTION("substring over the end") {
        n = strdel(str + 7, 5);
        REQUIRE(n == 4);
        REQUIRE(0 == strcmp(str, "abcdXYZ"));
    }

    SECTION("substring inside") {
        n = strdel(str + 7, 2);
        REQUIRE(n == 2);
        REQUIRE(0 == strcmp(str, "abcdXYZgh"));
    }
}

TEST_CASE("Insert string", "[strins]") {
    static constexpr char text[12] = "abcdXYZefgh";
    char str[255] = "abcdXYZefgh";
    char *nostr = nullptr;
    size_t n;

    SECTION("void input 1") {
        n = strins(nostr, str);
        REQUIRE(n == 0);
        REQUIRE(nostr == nullptr);
        REQUIRE_THAT(str, Equals(text));
    }

    SECTION("void input 2") {
        n = strins(str, nostr);
        REQUIRE(n == 0);
        REQUIRE(nostr == nullptr);
        REQUIRE_THAT(str, Equals(text));
    }

    SECTION("insert 0 chars") {
        n = strins(str, 0);
        REQUIRE(n == 0);
        REQUIRE_THAT(str, Equals(text));
    }

    SECTION("empty string 1") {
        n = strins(str, "");
        REQUIRE(n == 0);
        REQUIRE_THAT(str, Equals(text));
    }

    SECTION("empty string 2") {
        char empty[255] = "";
        n = strins(empty, str);
        REQUIRE(n == strlen(empty));
        REQUIRE(strlen(str) == strlen(empty));
        REQUIRE_THAT(empty, Equals(text));
    }

    SECTION("single char at the beginning") {
        n = strins(str, "a");
        CHECK(n == 1);
        REQUIRE_THAT(str, Equals("aabcdXYZefgh"));
    }

    SECTION("single char inside") {
        n = strins(str + 4, "a");
        CHECK(n == 1);
        REQUIRE_THAT(str, Equals("abcdaXYZefgh"));
    }

    SECTION("string at the beginning") {
        n = strins(str, "ABCD");
        CHECK(n == 4);
        REQUIRE_THAT(str, Equals("ABCDabcdXYZefgh"));
    }

    SECTION("string at the end") {
        n = strins(str + strlen(str), "ABC");
        CHECK(n == 3);
        REQUIRE_THAT(str, Equals("abcdXYZefghABC"));
    }

    SECTION("insert more times") {
        n = strins(str, "123 ", 3);
        CHECK(n == 12);
        REQUIRE_THAT(str, Equals("123 123 123 abcdXYZefgh"));
    }
}

TEST_CASE("Shift string", "[strshift]") {
    static constexpr char text[12] = "abcdXYZefgh";
    char str[255] = "abcdXYZefgh";
    char *nostr = nullptr;
    size_t n;

    SECTION("void input") {
        n = strshift(nostr);
        CHECK(n == 0);
    }

    SECTION("by 0") {
        n = strshift(str, 0);
        CHECK(n == 0);
        REQUIRE_THAT(str, Equals(text));
    }

    SECTION("at the beginning; short text, long distance") {
        size_t size = strlen(str);
        size_t shift = 2 * size;
        n = strshift(str, shift);
        CHECK(n == shift);
        CHECK(strlen(str) == size + shift);
        REQUIRE_THAT(str + shift, Equals(text));
    }

    SECTION("at the beginning; long text, short distance") {
        size_t size = strlen(str);
        size_t shift = size / 2;
        n = strshift(str, shift);
        CHECK(n == shift);
        CHECK(strlen(str) == size + shift);
        REQUIRE_THAT(str + shift, Equals(text));
    }

    SECTION("in the middle") {
        n = strshift(str + 3, 3);
        CHECK(n == 3);
        REQUIRE_THAT(str + 3 + 3, Equals("dXYZefgh"));
    }
}

// TEST_CASE("String to multi-line conversion(s)", "[tools][all]") {
// #define EXAMPLE_STR  "bla b" QT_HSPACE "l blabla bla" QT_HYPHEN "bla*bla" QT_NL "BLA BLA"
// #define EXAMPLE_STR2 "bla xxx" QT_HSPACE "bla bla bla" QT_HYPHEN "bla*bla" QT_NL "BLA BLA"

// #define LINE_WIDTH 8
//     char str[255];
//     size_t n;

//     SECTION("conversion to plain / example 1") {
//         strcpy(str, "ab" QT_HYPHEN "cd-*+XYZef" QT_HSPACE "gh");
//         n = str2plain(str);
//         CHECK(n == 2);
//         REQUIRE_THAT(str, Equals("abcd-*+XYZef gh"));
//     }

//     SECTION("conversion to plain / example 2") {
//         set_withdraw_set("+-");
//         strcpy(str, "abcd-*+XYZefgh");
//         n = str2plain(str, true);
//         CHECK(n == 2);
//         REQUIRE_THAT(str, Equals("abcd*XYZefgh"));
//     }

//     SECTION("conversion to plain / example 3") {
//         strcpy(str, "abcd-*+XYZefgh");
//         n = str2plain(str, "*+-");
//         CHECK(n == 3);
//         REQUIRE_THAT(str, Equals("abcdXYZefgh"));
//     }

//     SECTION("conversion to plain / example 4") {
//         strcpy(str, "abcd-*+XYZefgh\n123\n456");
//         n = str2plain(str, "", "\n", '|');
//         CHECK(n == 2);
//         REQUIRE_THAT(str, Equals("abcd-*+XYZefgh|123|456"));
//     }

// #define RESULT1 "bla b l" QT_NL "blabla" QT_NL "bla-" QT_NL "bla*bla" QT_NL "BLA BLA"
//     SECTION("conversion to multiline / example 1") {
//         set_hyphen_distance(HYPHEN_ALLWAYS);
//         strcpy(str, EXAMPLE_STR);
//         n = str2multiline(str, LINE_WIDTH);
//         CHECK(n == 5);
//         REQUIRE_THAT(str, Equals(RESULT1));
//     }

// #define RESULT2 "bla b l" QT_NL "blabla" QT_NL "blabla*b" QT_NL "la" QT_NL "BLA BLA"
//     SECTION("conversion to multiline / example 2") {
//         strcpy(str, EXAMPLE_STR);
//         n = str2multiline(str, LINE_WIDTH);
//         CHECK(n == 5);
//         REQUIRE_THAT(str, Equals(RESULT2));
//     }

// #define RESULT3 "bla b l" QT_NL "blabla" QT_NL "blabla*" QT_NL "bla" QT_NL "BLA BLA"
//     SECTION("conversion to multiline / example 3") {
//         set_custom_set("*");
//         strcpy(str, EXAMPLE_STR);
//         n = str2multiline(str, LINE_WIDTH);
//         CHECK(n == 5);
//         REQUIRE_THAT(str, Equals(RESULT3));
//     }

// #define RESULT4 "bla b l" QT_NL "blabla" QT_NL "blablabl" QT_NL "a" QT_NL "BLA BLA"
//     SECTION("conversion to multiline / example 4") {
//         set_withdraw_set("*");
//         strcpy(str, EXAMPLE_STR);
//         n = str2multiline(str, LINE_WIDTH);
//         CHECK(n == 5);
//         REQUIRE_THAT(str, Equals(RESULT4));
//     }

// #define RESULT5 "bla b l blabla blabla*bla" QT_NL "BLA BLA"
//     SECTION("conversion to multiline / example 5") {
//         strcpy(str, EXAMPLE_STR);
//         n = str2multiline(str, LINE_WIDTH_UNLIMITED);
//         CHECK(n == 2);
//         REQUIRE_THAT(str, Equals(RESULT5));
//     }

// #define RESULT6 "bla" QT_NL "xxx bla" QT_NL "bla" QT_NL "blabla*b" QT_NL "la" QT_NL "BLA BLA"
//     SECTION("conversion to multiline / example 6") {
//         strcpy(str, EXAMPLE_STR2);
//         n = str2multiline(str, LINE_WIDTH);
//         CHECK(n == 6);
//         REQUIRE_THAT(str, Equals(RESULT6));
//     }
// }
