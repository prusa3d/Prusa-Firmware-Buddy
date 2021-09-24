if(NOT TARGET FatFs_Config)
  message(FATAL_ERROR "Target FatFs_Config does not exist.")
endif()

add_library(FatFs src/diskio.c src/ff.c src/ff_gen_drv.c src/ffsystem.c src/ffunicode.c)

target_include_directories(FatFs PUBLIC src)

target_link_libraries(FatFs PUBLIC FatFs_Config)

add_library(Middlewares::FatFs ALIAS FatFs)
