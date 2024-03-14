add_library(
  LIS2DH12
  lis2dh12-pid/lis2dh12_reg.c
)

target_include_directories(LIS2DH12 PUBLIC lis2dh12-pid)
