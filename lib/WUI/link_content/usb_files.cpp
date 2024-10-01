#include "usb_files.h"
#include "../nhttp/headers.h"
#include <str_utils.hpp>

// Why does FILE_PATH_BUFFER_LEN lives in *gui*!?
#include "../../src/gui/file_list_defs.h"

namespace nhttp::link_content {

using http::Method;
using http::Status;
using std::nullopt;
using std::optional;
using std::string_view;
using namespace handler;

Selector::Accepted UsbFiles::accept(const RequestParser &parser, handler::Step &out) const {
    const string_view uri = parser.uri();
    const constexpr string_view prefix = "/usb/";

    // We claim the /usb/ namespace.
    if (uri.size() < prefix.size() || uri.substr(0, prefix.size()) != prefix) {
        return Accepted::NextSelector;
    }

    // Content of the USB drive is only for authenticated, don't ever try anything without it.
    if (!parser.check_auth(out)) {
        return Accepted::Accepted;
    }

    char fname[FILE_PATH_BUFFER_LEN];
    if (!parser.uri_filename(fname, sizeof(fname))) {
        out.next = StatusPage(Status::NotFound, parser, "This doesn't look like file name");
        return Accepted::Accepted;
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
            out.next = SendFile(f, fname, guess_content_by_ext(fname), parser.can_keep_alive(), parser.accepts_json, parser.if_none_match, hdrs);
            /*
             * Some browsers reportedly mishandle combination of attachment +
             * etags/caching. We give up caching in this particular case, as it
             * is not super useful anyway.
             */
            get<SendFile>(get<ConnectionState>(out.next)).disable_caching();
        } else {
            out.next = StatusPage(Status::NotFound, parser);
        }
    } else {
        out.next = StatusPage(Status::MethodNotAllowed, parser);
    }
    return Accepted::Accepted;
}

const UsbFiles usb_files;

} // namespace nhttp::link_content
