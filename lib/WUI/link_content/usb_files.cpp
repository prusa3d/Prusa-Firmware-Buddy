#include "usb_files.h"
#include "../nhttp/headers.h"

// Why does FILE_PATH_BUFFER_LEN lives in *gui*!?
#include "../../src/gui/file_list_defs.h"

namespace nhttp::link_content {

using std::nullopt;
using std::optional;
using std::string_view;
using namespace handler;

optional<ConnectionState> UsbFiles::accept(const RequestParser &parser) const {
    const string_view uri = parser.uri();
    const constexpr string_view prefix = "/usb/";

    // We claim the /usb/ namespace.
    if (uri.size() < prefix.size() || uri.substr(0, prefix.size()) != prefix) {
        return nullopt;
    }

    // Content of the USB drive is only for authenticated, don't ever try anything without it.
    if (!parser.authenticated()) {
        return StatusPage(Status::Unauthorized, parser.status_page_handling(), parser.accepts_json);
    }

    char fname[FILE_PATH_BUFFER_LEN];
    if (!parser.uri_filename(fname, sizeof(fname))) {
        return StatusPage(Status::NotFound, parser.status_page_handling(), parser.accepts_json, "This doesn't look like file name");
    }

    if (parser.method == Method::Get) {
        FILE *f = fopen(fname, "rb");

        if (f != nullptr) {
            /*
             * This can send any file on the flash drive. But we are
             * protected by the API key and it's the user's files, so
             * that's probably fine.
             */
            static const char *const hdrs[] = {
                "Content-Disposition: attachment\r\n",
                nullptr,
            };
            SendFile step(f, fname, guess_content_by_ext(fname), parser.can_keep_alive(), parser.accepts_json, parser.if_none_match, hdrs);
            /*
             * Some browsers reportedly mishandle combination of attachment +
             * etags/caching. We give up caching in this particular case, as it
             * is not super useful anyway.
             */
            step.disable_caching();
            return std::move(step);
        }

        return StatusPage(Status::NotFound, parser.status_page_handling(), parser.accepts_json);
    } else {
        return StatusPage(Status::MethodNotAllowed, parser.status_page_handling(), parser.accepts_json);
    }
}

const UsbFiles usb_files;

}
