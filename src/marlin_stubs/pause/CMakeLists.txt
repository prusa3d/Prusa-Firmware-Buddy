target_sources(firmware PRIVATE G27.cpp M125.cpp M603.cpp)

if(HAS_PAUSE)
  target_sources(
    firmware
    PRIVATE M1700_1_parse.cpp
            M600.cpp
            M601_2.cpp
            M701_2.cpp
            M701_2_parse.cpp
            M70X_preheat.cpp
            pause.cpp
            pause_settings.cpp
    )

endif()

if(HAS_MMU2)
  target_sources(firmware PRIVATE M704-6_parse.cpp M707-9_parse.cpp M70X_mmu.cpp)
endif()
