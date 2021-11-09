/// gcode_process tests

#include <algorithm>
#include <limits.h>
#include <string.h>

#include "catch2/catch.hpp"
#include "path.h"

/// PATH_MAX is not defined in MinGW32 with __STRICT_ANSI__ defined
#ifndef PATH_MAX
    #define PATH_MAX 4096
#endif

using Catch::Matchers::Equals;

TEST_CASE("path_update", "") {
    int res;
    char path[PATH_MAX] = "/";

    res = update_path(path, "a", PATH_MAX);
    REQUIRE(res == 0);
    REQUIRE_THAT(path, Equals("/a/"));

    res = update_path(path, "b/./", PATH_MAX);
    REQUIRE(res == 0);
    REQUIRE_THAT(path, Equals("/a/b/"));

    res = update_path(path, "d/../c", PATH_MAX);
    REQUIRE(res == 0);
    REQUIRE_THAT(path, Equals("/a/b/c/"));

    res = update_path(path, "../d", PATH_MAX);
    REQUIRE(res == 0);
    REQUIRE_THAT(path, Equals("/a/b/d/"));

    res = update_path(path, ".", PATH_MAX);
    REQUIRE(res == 0);
    REQUIRE_THAT(path, Equals("/a/b/d/"));

    res = update_path(path, "./", PATH_MAX);
    REQUIRE(res == 0);
    REQUIRE_THAT(path, Equals("/a/b/d/"));

    res = update_path(path, "..", PATH_MAX);
    REQUIRE(res == 0);
    REQUIRE_THAT(path, Equals("/a/b/"));

    res = update_path(path, "../", PATH_MAX);
    REQUIRE(res == 0);
    REQUIRE_THAT(path, Equals("/a/"));

    res = update_path(path, "../../x", PATH_MAX);
    REQUIRE(res != 0);

    res = update_path(path, "/a", PATH_MAX);
    REQUIRE(res == 0);
    REQUIRE_THAT(path, Equals("/a/"));

    res = update_path(path, "/a/b/../", PATH_MAX);
    REQUIRE(res == 0);
    REQUIRE_THAT(path, Equals("/a/"));

    res = update_path(path, "/a/./b/", PATH_MAX);
    REQUIRE(res == 0);
    REQUIRE_THAT(path, Equals("/a/b/"));

    res = update_path(path, "/a/./b/d/../c", PATH_MAX);
    REQUIRE(res == 0);
    REQUIRE_THAT(path, Equals("/a/b/c/"));
}
