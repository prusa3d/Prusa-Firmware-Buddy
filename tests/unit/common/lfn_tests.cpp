#include <catch2/catch.hpp>

#include <lfn.h>
#include <gui/file_list_defs.h>

#include "../gui/lazyfilelist/ourPosix.hpp"

#include <cstring>
#include <string>

using std::string;

TEST_CASE("get_SFN_path_and_LFN relocates LFN when the path shrinks") {
    testFiles0 = {
        { "very_long_filename.gcode", 0, false },
    };

    char buffer[FILE_PATH_BUFFER_LEN + FILE_NAME_BUFFER_LEN] = {};
    strcpy(buffer, "/usb/very_long_filename.gcode");

    char *old_name = buffer + strlen(buffer) + 1;
    get_SFN_path_and_LFN(buffer, old_name, FILE_NAME_BUFFER_LEN);

    char *new_name = buffer + strlen(buffer) + 1;
    REQUIRE(new_name != old_name);
    REQUIRE(string(buffer) == "/usb/very_l~00.GCO");
    REQUIRE(string(new_name) == "very_long_filename.gcode");
}

TEST_CASE("get_SFN_path_and_LFN keeps standalone LFN buffers untouched") {
    testFiles0 = {
        { "very_long_filename.gcode", 0, false },
    };

    char path[FILE_PATH_BUFFER_LEN] = {};
    char lfn[FILE_NAME_BUFFER_LEN] = {};
    strcpy(path, "/usb/very_long_filename.gcode");

    get_SFN_path_and_LFN(path, lfn, FILE_NAME_BUFFER_LEN);

    REQUIRE(string(path) == "/usb/very_l~00.GCO");
    REQUIRE(string(lfn) == "very_long_filename.gcode");
}
