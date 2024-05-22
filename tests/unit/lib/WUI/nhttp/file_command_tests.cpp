#include <nhttp/file_command.h>
#include <nhttp/handler.h>

#include <catch2/catch.hpp>

namespace nhttp::printer {

/*
 * In the real application, these call into marlin and that would be
 * pain to deal with in tests. So we have a separate implementation of
 * them here.
 *
 * No tests here for now, just "plugging" the missing method for now.
 */

FileCommand::StartResult FileCommand::start() {
    return StartResult::Started;
}

} // namespace nhttp::printer
