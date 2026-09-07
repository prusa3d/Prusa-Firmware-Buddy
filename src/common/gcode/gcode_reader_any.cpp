#include "gcode_reader_any.hpp"

#include <i18n.h>
#include "transfers/transfer.hpp"
#include <errno.h> // for EAGAIN
#include <filename_type.hpp>
#include <sys/stat.h>
#include <type_traits>

#include <config_store/store_instance.hpp>
#if HAS_E2EE_SUPPORT()
    #include <e2ee/identity_check_levels.hpp>
#endif

AnyGcodeFormatReader::~AnyGcodeFormatReader() {
}

AnyGcodeFormatReader::AnyGcodeFormatReader(AnyGcodeFormatReader &&other)
    : storage { std::move(other.storage) } {
    other.close();
}

AnyGcodeFormatReader &AnyGcodeFormatReader::operator=(AnyGcodeFormatReader &&other) {
    storage = std::move(other.storage);
    other.close();
    return *this;
}

AnyGcodeFormatReader::AnyGcodeFormatReader()
    : storage { ClosedReader {} } {
}

AnyGcodeFormatReader::AnyGcodeFormatReader(const char *filename, bool allow_decryption
#if HAS_E2EE_SUPPORT()
    ,
    e2ee::IdentityCheckLevel identity_check_lvl
#endif
    )
    : storage { ClosedReader {} } {

    open(filename, allow_decryption
#if HAS_E2EE_SUPPORT()
        ,
        identity_check_lvl
#endif
    );
}

bool AnyGcodeFormatReader::open(const char *filename, bool allow_decryption
#if HAS_E2EE_SUPPORT()
    ,
    e2ee::IdentityCheckLevel identity_check_lvl
#endif
) {
    close();

    transfers::Transfer::Path path(filename);
    struct stat info {};

    // check if file is partially downloaded file
    bool is_partial = false;

    if (stat(path.as_destination(), &info) != 0) {
        return false;
    }

    if (S_ISDIR(info.st_mode)) {
        if (stat(path.as_partial(), &info) != 0) {
            return false;
        }

        is_partial = true;
    }

    unique_file_ptr file = unique_file_ptr(fopen(is_partial ? path.as_partial() : path.as_destination(), "rb"));
    if (!file) {
        return false;
    }

    if (filename_is_bgcode(filename)) {
        storage.emplace<PrusaPackGcodeReader>(std::move(file), info, allow_decryption
#if HAS_E2EE_SUPPORT()
            ,
            identity_check_lvl
#endif
        );
    }

    else if (filename_is_plain_gcode(filename)) {
        storage.emplace<PlainGcodeReader>(std::move(file), info);
    }

    else {
        return false;
    }

    if (is_partial) {
        get()->update_validity(path.as_destination());
    }

    return true;
}

void AnyGcodeFormatReader::close() {
    storage.emplace<ClosedReader>();
}

IGcodeReader *AnyGcodeFormatReader::get() {
    return std::visit([](auto &t) -> IGcodeReader * { return &t; }, storage);
}

bool AnyGcodeFormatReader::is_open() const {
    return !std::holds_alternative<ClosedReader>(storage);
}
