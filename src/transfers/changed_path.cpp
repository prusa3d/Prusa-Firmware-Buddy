#include "changed_path.hpp"

#include <crc32.h>
#include <random.h>

#include <cassert>
#include <cstring>
#include <string_view>

extern "C" {

// Inject for tests, which are compiled on systems without it in the header.
size_t strlcpy(char *, const char *, size_t);
}

using std::nullopt;
using std::optional;

namespace transfers {

void ChangedPath::changed_path(const char *filepath, Type changed_type, Incident action, optional<uint32_t> command_id) {
    Lock lock(mutex);

    if (command_id.has_value()) {
        assert(!this->command_id.has_value());
        this->command_id = command_id;
    }

    // Update the hash base, for generating etags on directories.
    //
    // This may "race" with something else updating the hash. But that's OK, we
    // just need it to change, not to be true representation of the chain of
    // events.
    ensure_chain_init();
    uint32_t crc = changed_chain_hash_base.load();
    crc = crc32_calc_ex(crc, reinterpret_cast<const uint8_t *>(&changed_type), sizeof changed_type);
    crc = crc32_calc_ex(crc, reinterpret_cast<const uint8_t *>(&action), sizeof action);
    crc = crc32_calc_ex(crc, reinterpret_cast<const uint8_t *>(filepath), strlen(filepath));
    changed_chain_hash_base.store(crc);

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
    // combining more than one path, so no longer just a file
    type = Type::Folder;
    incident = Incident::Combined;
    // find the longest common path and store it

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

    Status result(std::move(lock), *this);
    result.type = type;
    result.incident = incident;
    result.command_id = command_id;

    return result;
}

bool ChangedPath::Status::consume(char *out, size_t size) const {
    if (size <= strlen(owner.path.data())) {
        return false;
    }

    strlcpy(out, owner.path.data(), size);
    owner.path[0] = '\0';
    owner.command_id.reset();
    return true;
}

#ifdef UNITTESTS
const char *ChangedPath::Status::get_path() const {
    return owner.path.data();
}
#endif

bool ChangedPath::Status::is_file() const {
    return type == Type::File;
}

ChangedPath::Incident ChangedPath::Status::what_happend() const {
    return incident;
}

optional<uint32_t> ChangedPath::Status::triggered_command_id() const {
    return command_id;
}

void ChangedPath::ensure_chain_init() {
    // We lazy-init the base if it is 0.
    //
    // Technically, there is a race if two different threads try to init it at
    // the same time (both could write some init value). But as we don't care
    // about the value, only that it "is different" on boot and "is different"
    // on change, this is not a problem. The worst that can happen is false
    // cache miss.
    //
    // Similarly, there's a chance this would "naturally" be 0 when we read it
    // and at that point we would just replace it with random number. Again,
    // the worst is a false cache miss.
    uint32_t val = changed_chain_hash_base.load();
    if (val == 0) {
        val = rand_u();
        changed_chain_hash_base.store(val);
    }
}

uint32_t ChangedPath::change_chain_hash(const char *path) {
    uint32_t crc = changed_chain_hash_base.load();
    crc = crc32_calc_ex(crc, reinterpret_cast<const uint8_t *>(path), strlen(path));
    return crc;
}

void ChangedPath::media_inserted(bool inserted) {
    bool previous = last_media_inserted.exchange(inserted);
    if (previous != inserted) {
        // Note: This "update" is technically racy, another update may come in
        // between. As the hash has no actual meaning, that's OK, we just need
        // it changes and it does.
        uint32_t crc = changed_chain_hash_base.load();
        uint8_t v = inserted;
        crc = crc32_calc_ex(crc, &v, 1);
        changed_chain_hash_base.store(crc);
    }
}

ChangedPath ChangedPath::instance;

} // namespace transfers
