#include "changed_path.hpp"

#include <cstring>
#include <string_view>

extern "C" {

// Inject for tests, which are compiled on systems without it in the header.
size_t strlcpy(char *, const char *, size_t);
}

using std::nullopt;
using std::optional;

namespace transfers {

void ChangedPath::changed_path(const char *filepath, Type changed_type, Incident action) {
    Lock lock(mutex);

    size_t size = std::min(strlen(filepath), strlen(path.data()));
    if (strcmp(filepath, path.data()) == 0 && changed_type == Type::Folder) {
        // when we get the same folder twice, get rid of the last /
        // so that the algoritmh below results in one directory up
        if (filepath[size - 1] == '/') {
            size--;
        }
    }
    // No path currently to be reported
    if (path[0] == '\0') {
        type = changed_type;
        incident = action;
        strlcpy(path.data(), filepath, path.size());
        return;
    }
    //combining more than one path, so no longer just a file
    type = Type::Folder;
    incident = Incident::Combined;
    //find the longest common path and store it

    size_t last_common_slash_index = 0;
    for (size_t i = 0; i < size; i++) {
        if (path[i] != filepath[i]) {
            break;
        }
        if (path[i] == '/') {
            last_common_slash_index = i;
        }
    }
    path[last_common_slash_index + 1] = '\0';
}

optional<ChangedPath::Status> ChangedPath::status() {
    Lock lock(mutex);

    if (path[0] == '\0') {
        return nullopt;
    }

    Status result { std::move(lock) };
    result.path = path.data();
    result.type = type;
    result.incident = incident;

    return result;
}

bool ChangedPath::Status::consume_path(char *out, size_t size) const {
    if (size <= strlen(path)) {
        return false;
    }

    strlcpy(out, path, size);
    path[0] = '\0';
    return true;
}

#ifdef UNITTESTS
const char *ChangedPath::Status::get_path() const {
    return path;
}
#endif

bool ChangedPath::Status::is_file() const {
    return type == Type::File;
}

ChangedPath::Incident ChangedPath::Status::what_happend() const {
    return incident;
}

ChangedPath ChangedPath::instance;

}
