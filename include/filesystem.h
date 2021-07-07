#pragma once

// Filesystems like FAT doesn't distinguish between permissions for user/group/other,
// therefore in the implamentation they always uses all 3 permissions together.
#define IS_IRALL (S_IRUSR | S_IRGRP | S_IROTH) /* read permission, all */
#define IS_IWALL (S_IWUSR | S_IWGRP | S_IWOTH) /* write permission, all */
#define IS_IXALL (S_IXUSR | S_IXGRP | S_IXOTH) /* execute/search permission, all */

#define IS_EMPTY(s) (!s || !s[0])

#if defined(__cplusplus)
extern "C" {
#endif //defined(__cplusplus)

void filesystem_init();

#if defined(__cplusplus)
} //extern "C"
#endif //defined(__cplusplus)
