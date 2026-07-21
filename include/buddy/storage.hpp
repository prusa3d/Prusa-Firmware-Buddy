/// @file
/// The storages available for user files and their mount points.
#pragma once

#include <optional>
#include <span>
#include <string_view>

#include <option/has_internal_storage_flash.h>

namespace storage {

enum class Type {
    usb,
#if HAS_INTERNAL_STORAGE_FLASH()
    internal,
#endif
};

struct Mount {
    Type type;
    const char *root;
};

/// All storages that can hold user files. Some of them (USB) may be currently absent.
std::span<const Mount> mounts();

/// Returns the storage type for a given path or nullopt if it isn't a known
/// storage. The mount root matches as a whole path component, including bare
/// (e.g. both "/usb" and "/usb/x", but not "/usbx").
std::optional<Type> type(std::string_view path);
std::optional<Type> type(const char *path);

/// The mount root of a storage, e.g. "/usb".
const char *root(Type type);

} // namespace storage
