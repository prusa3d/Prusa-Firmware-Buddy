add_library(
  sysbase
  chdir.c
  chmod.c
  dirent.c
  fchmod.c
  fstat.c
  ftruncate.c
  fsync.c
  handle_manager.c
  iosupport.c
  link.c
  lseek.c
  lstat.c
  mkdir.c
  open.c
  path.c
  read.c
  rename.c
  rmdir.c
  scandir.c
  stat.c
  statvfs.c
  syscall_support.c
  truncate.c
  unlink.c
  utime.c
  write.c
  )

target_include_directories(sysbase PUBLIC .)

add_library(sysbase::sysbase ALIAS sysbase)
