/// str_utils tests

#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"
using Catch::Matchers::Equals;

#include "str_utils.cpp"
#include "str_utils.h"
#include <string.h>

TEST_CASE("Delete string","[strdel]") {
    char str[255] = "abcdXYZefgh";
    char *nostr = nullptr;
    size_t n;

    SECTION("void input") {
        n = strdel(nostr);
        REQUIRE(n == 0);
        REQUIRE(nostr==nullptr);
    }

    SECTION("delete 0 chars") {
        n = strdel(str,0);
        REQUIRE(n == 0) ;
        REQUIRE(0 == strcmp(str, "abcdXYZefgh"));
    }

    SECTION("empty string") {
        strcpy(str, "");
        n = strdel(str);
        REQUIRE(n == 0) ;
        REQUIRE(0 == strcmp(str, ""));
    }

    SECTION("single char at beginning") {
        n = strdel(str);
        REQUIRE(n == 1) ;
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

TEST_CASE("Insert string","[strins]") {
    // char str[255] = "abcdXYZefgh";
    // char *nostr = nullptr;
    // size_t n;

    // SECTION("void input") {
    //     n = strins(nostr);
    //     REQUIRE(n == 0);
    //     REQUIRE(nostr==nullptr);
    // }

    // SECTION("delete 0 chars") {
    //     n = strdel(str,0);
    //     REQUIRE(n == 0) ;
    //     REQUIRE(0 == strcmp(str, "abcdXYZefgh"));
    // }

    // SECTION("empty string") {
    //     strcpy(str, "");
    //     n = strdel(str);
    //     REQUIRE(n == 0) ;
    //     REQUIRE(0 == strcmp(str, ""));
    // }

    // SECTION("single char at beginning") {
    //     n = strdel(str);
    //     REQUIRE(n == 1) ;
    //     REQUIRE(0 == strcmp(str, "bcdXYZefgh"));
    // }


    // SECTION("single char inside") {
    //     n = strdel(str + 4, 1);
    //     REQUIRE(n == 1);
    //     REQUIRE(0 == strcmp(str, "abcdYZefgh"));
    // }

    // SECTION("substring over the end") {
    //     n = strdel(str + 7, 5);
    //     REQUIRE(n == 4);
    //     REQUIRE(0 == strcmp(str, "abcdXYZ"));
    // }

    // SECTION("substring inside") {
    //     n = strdel(str + 7, 2);
    //     REQUIRE(n == 2);
    //     REQUIRE(0 == strcmp(str, "abcdXYZgh"));
    // }

//     SECTION("helpers / insert-example 1") {
//         strcpy(str, "abcdefgh");
//         n = strins(str + 3, "XYZ");
//         CHECK(n == 3);
//         REQUIRE_THAT(str, Equals("abcdXYZefgh"));
//     }

//     SECTION("helpers / insert-example 2") {
//         strcpy(str, "abcdefgh");
//         n = strins(str, "XYZ", 1);
//         CHECK(n == 3);
//         REQUIRE_THAT(str, Equals("XYZabcdefgh"));
//     }

//     SECTION("helpers / insert-example 3") {
//         strcpy(str, "abcdefgh");
//         n = strins(str + 3, "*", 4);
//         CHECK(n == 4);
//         REQUIRE_THAT(str, Equals("abcd****efgh"));
//     }

//     SECTION("helpers / insert-example 4") {
//         strcpy(str, "abcdefgh");
//         n = strins(str, "");
//         CHECK(n == 0);
//         REQUIRE_THAT(str, Equals("abcdefgh"));
//     }
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
