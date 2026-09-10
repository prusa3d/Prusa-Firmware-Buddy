#pragma once
#include <string_view>

// Deduplicates successive slashes from a path, in-place.
void dedup_slashes(char *filename);

[[nodiscard]] bool file_exists(const char *path);

bool make_dirs(std::string_view path);

// Is this path under /usb and free of "/../" traversal?
[[nodiscard]] bool path_allowed(const char *path);
