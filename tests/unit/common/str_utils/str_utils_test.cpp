/// str_utils tests

#include <string.h>
#include <iostream>

#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"
using Catch::Matchers::Equals;

#include "str_utils.cpp"
#include "str_utils.h"

#define n255 255
#define n511 511

TEST_CASE("Delete string", "[strdel]") {
    static constexpr char text[12] = "abcdXYZefgh";
    char str[255] = "abcdXYZefgh";
    char *nostr = nullptr;
    size_t n;

    SECTION("void input") {
        n = strdel(nostr);
        CHECK(n == 0);
    }

    SECTION("delete 0 chars") {
        n = strdel(str, 0);
        CHECK(n == 0);
        CHECK(0 == strcmp(str, text));
    }

    SECTION("empty string") {
        strcpy(str, "");
        n = strdel(str);
        CHECK(n == 0);
        CHECK(0 == strcmp(str, ""));
    }

    SECTION("single char at beginning") {
        n = strdel(str);
        CHECK(n == 1);
        CHECK(0 == strcmp(str, "bcdXYZefgh"));
    }

    SECTION("single char inside") {
        n = strdel(str + 4, 1);
        CHECK(n == 1);
        CHECK(0 == strcmp(str, "abcdYZefgh"));
    }

    SECTION("substring over the end") {
        n = strdel(str + 7, 5);
        CHECK(n == 4);
        CHECK(0 == strcmp(str, "abcdXYZ"));
    }

    SECTION("substring inside") {
        n = strdel(str + 7, 2);
        CHECK(n == 2);
        CHECK(0 == strcmp(str, "abcdXYZgh"));
    }
}

TEST_CASE("Insert string", "[strins]") {
    static constexpr char text[12] = "abcdXYZefgh";
    char str[n255] = "abcdXYZefgh";
    char *nostr = nullptr;
    int n;

    SECTION("void input 1") {
        n = strins(nostr, n255, str);
        CHECK(n < 0);
        REQUIRE_THAT(str, Equals(text));
    }

    SECTION("void input 2") {
        n = strins(str, n255, nostr);
        CHECK(n < 0);
        REQUIRE_THAT(str, Equals(text));
    }

    SECTION("insert a char 0 times") {
        n = strins(str, n255, "a", 0);
        CHECK(n == 0);
        REQUIRE_THAT(str, Equals(text));
    }

    SECTION("empty string 1") {
        n = strins(str, n255, "");
        CHECK(n == 0);
        REQUIRE_THAT(str, Equals(text));
    }

    SECTION("empty string 2") {
        char empty[255] = "";
        n = strins(empty, n255, str);
        CHECK(n == int(strlen(empty)));
        CHECK(strlen(str) == strlen(empty));
        REQUIRE_THAT(empty, Equals(text));
    }

    SECTION("single char at the beginning") {
        n = strins(str, n255, "a");
        CHECK(n == 1);
        REQUIRE_THAT(str, Equals("aabcdXYZefgh"));
    }

    SECTION("single char inside") {
        n = strins(str + 4, n255 - 4, "a");
        CHECK(n == 1);
        REQUIRE_THAT(str, Equals("abcdaXYZefgh"));
    }

    SECTION("string at the beginning") {
        n = strins(str, n255, "ABCD");
        CHECK(n == 4);
        REQUIRE_THAT(str, Equals("ABCDabcdXYZefgh"));
    }

    SECTION("string at the end") {
        n = strins(str + strlen(str), n255 - strlen(str), "ABC");
        CHECK(n == 3);
        REQUIRE_THAT(str, Equals("abcdXYZefghABC"));
    }

    SECTION("insert more times") {
        n = strins(str, n255, "123 ", 3);
        CHECK(n == 12);
        REQUIRE_THAT(str, Equals("123 123 123 abcdXYZefgh"));
    }

    SECTION("insert too much") {
        n = strins(str, strlen(str) + 9, "123 ", 3);
        CHECK(n < 0);
    }
}

TEST_CASE("Shift string", "[strshift]") {
    static constexpr char text[12] = "abcdXYZefgh";
    char str[n255] = "abcdXYZefgh";
    char *nostr = nullptr;
    int n;

    SECTION("void input") {
        n = strshift(nostr, n255);
        CHECK(n < 0);
    }

    SECTION("by 0") {
        n = strshift(str, n255, 0);
        CHECK(n == 0);
        REQUIRE_THAT(str, Equals(text));
    }

    SECTION("at the beginning; short text, long distance") {
        size_t size = strlen(str);
        int shift = 2 * size;
        n = strshift(str, n255, shift);
        CHECK(n == shift);
        CHECK(strlen(str) == size + shift);
        REQUIRE_THAT(str + shift, Equals(text));
    }

    SECTION("at the beginning; long text, short distance") {
        size_t size = strlen(str);
        int shift = size / 2;
        n = strshift(str, n255, shift);
        CHECK(n == shift);
        CHECK(strlen(str) == size + shift);
        REQUIRE_THAT(str + shift, Equals(text));
    }

    SECTION("in the middle") {
        n = strshift(str + 3, n255 - 3, 3);
        CHECK(n == 3);
        REQUIRE_THAT(str + 3 + 3, Equals("dXYZefgh"));
    }

    SECTION("too much") {
        n = strshift(str, strlen(str) + 3, 3);
        CHECK(n < 0);
    }
}

TEST_CASE("String to multi-line", "[str2multiline]") {
    char short_text[n511] = "Lorem ipsum dolor sit amet";
    char long_text[n511] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ";

    int n;
    size_t length;

    SECTION("short text, long line") {
        length = 15;
        n = str2multiline(short_text, n511, length);
        CHECK(n == 2);
        REQUIRE_THAT(short_text, Equals("Lorem ipsum\ndolor sit amet"));
    }

    SECTION("long text, long line") {
        length = 15;
        n = str2multiline(long_text, n511, length);
        CHECK(n == 7);
        REQUIRE_THAT(long_text, Equals("Lorem ipsum\ndolor sit amet,\nconsectetur\nadipiscing\nelit, sed do\neiusmod tempor\nincididunt "));
    }

    SECTION("short text, short line") {
        length = 8;
        n = str2multiline(short_text, n511, length);
        CHECK(n == 4);
        REQUIRE_THAT(short_text, Equals("Lorem\nipsum\ndolor\nsit amet"));
    }

    SECTION("long text, short line") {
        length = 8;
        n = str2multiline(long_text, n511, length);
        CHECK(n == 14);
        REQUIRE_THAT(long_text, Equals("Lorem\nipsum\ndolor\nsit\namet,\nconsecte\ntur\nadipisci\nng elit,\nsed do\neiusmod\ntempor\nincididu\nnt "));
    }

    SECTION("long text, very short line") {
        length = 4;
        n = str2multiline(long_text, n511, length);
        CHECK(n == 26);
        REQUIRE_THAT(long_text, Equals("Lore\nm\nipsu\nm\ndolo\nr\nsit\namet\n,\ncons\necte\ntur\nadip\nisci\nng\nelit\n,\nsed\ndo\neius\nmod\ntemp\nor\ninci\ndidu\nnt "));
    }

    SECTION("specific combination") {
        char str[n255] = "123 123 1234 1234";
        length = 3;
        n = str2multiline(str, n255, length);
        CHECK(n == 6);
        REQUIRE_THAT(str, Equals("123\n123\n123\n4\n123\n4"));
    }

    SECTION("long text, long line, forced new lines") {
        char str[n255] = "Lorem ipsum dolor sit\namet, consectetur adipiscing elit, sed\ndo eiusmod tempor incididunt ";
        length = 15;
        n = str2multiline(str, n255, length);
        CHECK(n == 9);
        REQUIRE_THAT(str, Equals("Lorem ipsum\ndolor sit\namet,\nconsectetur\nadipiscing\nelit, sed\ndo eiusmod\ntempor\nincididunt "));
    }

    SECTION("long text, shorter line, nonbreaking spaces") {
        char str[n511] = "Lorem ipsum dolor sit" NBSP "amet, consectetur adipiscing elit, sed" NBSP "do" NBSP "eiusmod tempor incididunt ";
        length = 12;
        n = str2multiline(str, n255, length);
        CHECK(n == 10);
        REQUIRE_THAT(str, Equals("Lorem ipsum\ndolor\nsit amet,\nconsectetur\nadipiscing\nelit,\nsed do\neiusmod\ntempor\nincididunt "));
    }

    SECTION("too small buffer") {
        n = str2multiline(short_text, strlen(short_text), 1);
        CHECK(n < 0);
    }
}
