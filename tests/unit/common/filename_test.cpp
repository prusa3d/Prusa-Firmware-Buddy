#include <catch2/catch_test_macros.hpp>
#include <cstring>

#include <common/filename.hpp>

TEST_CASE("Filename keeps distinct canonical and display names") {
    Filename f("PART~1.GCO", "Fun part.gcode");
    CHECK(strcmp(f.canonical(), "PART~1.GCO") == 0);
    CHECK(strcmp(f.display(), "Fun part.gcode") == 0);
    // They are stored in one buffer but at different offsets.
    CHECK(f.canonical() != f.display());
    CHECK(!f.empty());
}

TEST_CASE("Filename with a single name aliases display onto canonical") {
    Filename f("myfile.gcode");
    CHECK(strcmp(f.canonical(), "myfile.gcode") == 0);
    CHECK(strcmp(f.display(), "myfile.gcode") == 0);
    // No second copy is stored: display() points at canonical().
    CHECK(f.canonical() == f.display());
}

TEST_CASE("Filename aliases when both names are equal") {
    Filename f("name.gcode", "name.gcode");
    CHECK(f.canonical() == f.display());
}

TEST_CASE("Filename assign resets previous content") {
    Filename f("PART~1.GCO", "Fun part.gcode");
    f.assign("other.gcode");
    CHECK(strcmp(f.canonical(), "other.gcode") == 0);
    CHECK(f.canonical() == f.display());
}

TEST_CASE("Filename default is empty") {
    Filename f;
    CHECK(f.empty());
    CHECK(strcmp(f.canonical(), "") == 0);
    CHECK(f.canonical() == f.display());
}

TEST_CASE("Filename truncates an overlong name to a valid string") {
    char overlong[512];
    memset(overlong, 'a', sizeof(overlong) - 1);
    overlong[sizeof(overlong) - 1] = '\0';

    Filename f(overlong);
    CHECK(strlen(f.canonical()) < filename_defs::short_filename_buffer_size + filename_defs::filename_buffer_size);
    CHECK(f.display()[strlen(f.display())] == '\0');
}
