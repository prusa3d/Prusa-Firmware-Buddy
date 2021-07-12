add_library(littlefs lfs.c lfs_util.c)

# Required printfs for debug print is not available, therefore disable all prints
set_property(
  SOURCE lfs.c
  APPEND
  PROPERTY COMPILE_DEFINITIONS
            LFS_NO_DEBUG=1
            LFS_NO_WARN=1
            LFS_NO_ERROR=1
  )

target_include_directories(littlefs PUBLIC .)

add_library(Middlewares::littlefs ALIAS littlefs)
