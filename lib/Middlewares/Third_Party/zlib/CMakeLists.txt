add_library(
  zlib
  adler32.c
  compress.c
  deflate.c
  gzclose.c
  gzlib.c
  gzread.c
  gzwrite.c
  infback.c
  inffast.c
  inflate.c
  inftrees.c
  trees.c
  uncompr.c
  zutil.c
  )

target_compile_options(zlib PUBLIC -mno-unaligned-access)

target_include_directories(zlib PUBLIC .)
