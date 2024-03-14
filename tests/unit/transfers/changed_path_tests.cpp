#include <transfers/changed_path.hpp>

// The path length constants live in gui :-(
#include <gui/file_list_defs.h>
#include <catch2/catch.hpp>
#include <cstring>
#include <string_view>

using namespace transfers;
using std::nullopt;
using std::optional;
using std::string_view;
using Type = ChangedPath::Type;
using Incident = ChangedPath::Incident;

void check_path_consume(ChangedPath &changed_path, string_view exp_path, Type exp_type, Incident exp_incident, optional<uint32_t> command_id = nullopt) {
    // scope to limit the status life
    {
        auto status { changed_path.status() };
        REQUIRE(status.has_value());
        char path[FILE_PATH_BUFFER_LEN];
        status->consume(path, sizeof(path));
        REQUIRE(exp_path == path);
        bool exp_is_file = exp_type == Type::File;
        REQUIRE(status->is_file() == exp_is_file);
        REQUIRE(status->what_happend() == exp_incident);
        REQUIRE(status->triggered_command_id() == command_id);
    }
    REQUIRE(changed_path.status() == std::nullopt);
}

void check_path_peek(ChangedPath &changed_path, string_view exp_path, Type exp_type, Incident exp_incident) {
    auto status { changed_path.status() };
    REQUIRE(status.has_value());
    REQUIRE(exp_path == status->get_path());
    bool exp_is_file = exp_type == Type::File;
    REQUIRE(status->is_file() == exp_is_file);
    REQUIRE(status->what_happend() == exp_incident);
}

TEST_CASE("ChangedPath test") {
    ChangedPath changed_path;

    SECTION("file created") {
        changed_path.changed_path("/usb/a/b/some.gcode", Type::File, Incident::Created);
        check_path_consume(changed_path, "/usb/a/b/some.gcode", Type::File, Incident::Created);
    }

    SECTION("file deleted") {
        changed_path.changed_path("/usb/a/b/some.gcode", Type::File, Incident::Deleted);
        check_path_consume(changed_path, "/usb/a/b/some.gcode", Type::File, Incident::Deleted);
    }

    SECTION("dir created") {
        changed_path.changed_path("/usb/a/b", Type::Folder, Incident::Created);
        check_path_consume(changed_path, "/usb/a/b", Type::Folder, Incident::Created);
    }

    SECTION("dir deleted") {
        changed_path.changed_path("/usb/a/b", Type::Folder, Incident::Deleted);
        check_path_consume(changed_path, "/usb/a/b", Type::Folder, Incident::Deleted);
    }

    SECTION("more files modifies") {
        changed_path.changed_path("/usb/some_name/b/c/folder_name/some.gcode", Type::File, Incident::Created);
        check_path_peek(changed_path, "/usb/some_name/b/c/folder_name/some.gcode", Type::File, Incident::Created);

        changed_path.changed_path("/usb/some_name/b/c/another.gcode", Type::File, Incident::Created);
        check_path_peek(changed_path, "/usb/some_name/b/c/", Type::Folder, Incident::Combined);

        changed_path.changed_path("/usb/some_name/b/d/yet_another.gcode", Type::File, Incident::Deleted);
        check_path_peek(changed_path, "/usb/some_name/b/", Type::Folder, Incident::Combined);

        changed_path.changed_path("/usb/some_name/one_more.gcode", Type::File, Incident::Created);
        // even tho all modifications are files, the result is a modified directory
        check_path_consume(changed_path, "/usb/some_name/", Type::Folder, Incident::Combined);
    }

    SECTION("a lot of things happened") {
        changed_path.changed_path("/usb/a/b/c/d/e/some.gcode", Type::File, Incident::Created);
        check_path_peek(changed_path, "/usb/a/b/c/d/e/some.gcode", Type::File, Incident::Created);

        changed_path.changed_path("/usb/a/b/c/d/some_folder", Type::Folder, Incident::Deleted);
        check_path_peek(changed_path, "/usb/a/b/c/d/", Type::Folder, Incident::Combined);

        changed_path.changed_path("/usb/a/b/c/d/some_other.gcode", Type::File, Incident::Created);
        check_path_peek(changed_path, "/usb/a/b/c/d/", Type::Folder, Incident::Combined);

        changed_path.changed_path("/usb/a/b/random_name", Type::Folder, Incident::Created);
        check_path_peek(changed_path, "/usb/a/b/", Type::Folder, Incident::Combined);

        changed_path.changed_path("/usb/a/another.gcode", Type::File, Incident::Deleted);
        check_path_peek(changed_path, "/usb/a/", Type::Folder, Incident::Combined);

        changed_path.changed_path("/usb/some_other_name/b/z/a/b/csome.gcode", Type::File, Incident::Created);
        check_path_peek(changed_path, "/usb/", Type::Folder, Incident::Combined);

        changed_path.changed_path("/usb/c/n", Type::Folder, Incident::Deleted);
        check_path_peek(changed_path, "/usb/", Type::Folder, Incident::Combined);

        changed_path.changed_path("/usb/b/b/ff", Type::Folder, Incident::Created);

        check_path_consume(changed_path, "/usb/", Type::Folder, Incident::Combined);
    }

    SECTION("file created and deleted") {
        changed_path.changed_path("/usb/some/path/file.gcode", Type::File, Incident::Created);
        changed_path.changed_path("/usb/some/path/file.gcode", Type::File, Incident::Deleted);
        check_path_consume(changed_path, "/usb/some/path/", Type::Folder, Incident::Combined);
    }

    SECTION("folder created and deleted with slash at the end") {
        changed_path.changed_path("/usb/some/path/", Type::Folder, Incident::Created);
        changed_path.changed_path("/usb/some/path/", Type::Folder, Incident::Deleted, 42);
        check_path_consume(changed_path, "/usb/some/", Type::Folder, Incident::Combined, 42);
    }

    SECTION("folder created and deleted no slash at the end") {
        changed_path.changed_path("/usb/some/path", Type::Folder, Incident::Created, 42);
        changed_path.changed_path("/usb/some/path", Type::Folder, Incident::Deleted);
        check_path_consume(changed_path, "/usb/some/", Type::Folder, Incident::Combined, 42);
    }

    SECTION("Reset command_id on consume") {
        changed_path.changed_path("/usb/some/path", Type::Folder, Incident::Created, 42);
        check_path_consume(changed_path, "/usb/some/path", Type::Folder, Incident::Created, 42);
        changed_path.changed_path("/usb/some/path", Type::Folder, Incident::Deleted);
        check_path_consume(changed_path, "/usb/some/path", Type::Folder, Incident::Deleted);
    }
}

TEST_CASE("Changed path - hashes") {
    ChangedPath paths;

    uint32_t v1 = paths.change_chain_hash("/usb");
    REQUIRE(v1 != 0);
    // No change happened in between
    REQUIRE(v1 == paths.change_chain_hash("/usb"));
    // But it is different for different path
    REQUIRE(v1 != paths.change_chain_hash("/usb/another"));
    // Doing a file change also changes it.
    paths.changed_path("/usb/x.gcode", Type::File, Incident::Created);
    REQUIRE(v1 != paths.change_chain_hash("/usb"));

    paths.media_inserted(true);
    uint32_t v2 = paths.change_chain_hash("/usb");
    paths.media_inserted(true);
    REQUIRE(v2 == paths.change_chain_hash("/usb"));
    paths.media_inserted(false);
    uint32_t v3 = paths.change_chain_hash("/usb");
    REQUIRE(v2 != v3);
    REQUIRE(v3 == paths.change_chain_hash("/usb"));
}
