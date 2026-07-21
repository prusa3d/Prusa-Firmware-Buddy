#include <buddy/storage.hpp>

#include <bsod/bsod.h>
#include <cstring>

namespace storage {

static constexpr Mount mount_table[] = {
    { Type::usb, "/usb" },
#if HAS_INTERNAL_STORAGE_FLASH()
    { Type::internal, "/internal" },
#endif
};

std::span<const Mount> mounts() {
    return mount_table;
}

std::optional<Type> type(std::string_view path) {
    for (const Mount &mount : mount_table) {
        const std::string_view root(mount.root);
        if (path.substr(0, root.size()) == root
            && (path.size() == root.size() || path[root.size()] == '/')) {
            return mount.type;
        }
    }
    return std::nullopt;
}

std::optional<Type> type(const char *path) {
    return type(std::string_view(path));
}

const char *root(Type type) {
    for (const Mount &mount : mount_table) {
        if (mount.type == type) {
            return mount.root;
        }
    }
    bsod_unreachable();
}

} // namespace storage
