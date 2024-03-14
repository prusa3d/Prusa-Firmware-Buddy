#pragma once

#include <string.h>

// Filesystems like FAT doesn't distinguish between permissions for user/group/other,
// therefore in the implamentation they always uses all 3 permissions together.
#define IS_IRALL (S_IRUSR | S_IRGRP | S_IROTH) /* read permission, all */
#define IS_IWALL (S_IWUSR | S_IWGRP | S_IWOTH) /* write permission, all */
#define IS_IXALL (S_IXUSR | S_IXGRP | S_IXOTH) /* execute/search permission, all */

#define IS_EMPTY(s) (!s || !s[0])

#if defined(__cplusplus)
extern "C" {
#endif // defined(__cplusplus)

void filesystem_init();
void init_only_littlefs();

static inline const char *process_path(const char *path, const char *dev_name) {
    unsigned int dev_name_len = strlen(dev_name);
    const char *device_path = path;
    while (*device_path == '/') {
        // Skip leading slashes
        device_path++;
    }
    if (strncmp(device_path, dev_name, dev_name_len) == 0) {
        if (strlen(device_path) == dev_name_len) {
            // Device name is the whole path, return root
            return "/";
        }
        // Skip device name
        return device_path + dev_name_len;
    }
    // Device name not in the path, don't do any change
    return path;
}

#if defined(__cplusplus)
} // extern "C"
#endif // defined(__cplusplus)
