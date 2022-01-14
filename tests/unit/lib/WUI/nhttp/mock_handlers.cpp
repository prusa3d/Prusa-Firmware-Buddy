/*
 * A mock/stub variant of the gcode upload handler.
 *
 * Currently, the tests need the definitions of all the handlers (because it's
 * in the variant in server's buffers). However, the real one brings marlin
 * vars as a transitive false dependency (it brings in a header it needs and
 * something else from that header needs marlin vars) and bringing that one
 * into unit tests is practically impossible.
 *
 * We will likely solve this fake dependency later, once the connection between
 * the new server and the upload handling is cleaned up (right now it suffers
 * scars of being transplated from the old server and needs some refactoring +
 * making it possible to run multiple parallel uploads). But for now, we
 * provide stubs to satisfy the linker. These are not currently called from the
 * tests, which is verified by the aborts.
 */

#include <nhttp/gcode_upload.h>
#include <nhttp/req_parser.h>
#include <nhttp/handler.h>

#include <cstdlib>

namespace nhttp::printer {

using handler::RequestParser;
using handler::Step;
using std::string_view;

bool GcodeUpload::have_instance = false;

void GcodeUpload::UploaderDeleter::operator()(Uploader *uploader) {
    abort();
}

GcodeUpload::GcodeUpload(UploaderPtr uploader, size_t length)
    : uploader(std::move(uploader))
    , size_rest(length) {
    abort();
}

GcodeUpload::GcodeUpload(GcodeUpload &&other)
    : uploader(std::move(other.uploader))
    , size_rest(other.size_rest) {
    abort();
}

GcodeUpload &GcodeUpload::operator=(GcodeUpload &&other) {
    abort();
}

GcodeUpload::~GcodeUpload() {
    abort();
}

GcodeUpload::UploadResult GcodeUpload::start(const RequestParser &parser) {
    abort();
}

Step GcodeUpload::step(string_view input, bool terminated_by_client, uint8_t *, size_t) {
    abort();
}

}
