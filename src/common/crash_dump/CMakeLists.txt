target_sources(
  firmware
  PRIVATE ../../../lib/CrashCatcher/Core/src/CrashCatcher.c
          ../../../lib/CrashCatcher/Core/src/CrashCatcher_armv7m.S
          crash_dump_distribute.cpp
          # these two CrashCatcher files need to be here otherwise HardFault_Handler will be handled
          # by Default_Handler (no compiler error)
          crash_dump_handlers.cpp
          dump.cpp
          dump_parse.cpp
  )

# CrashCatcher needs increased stack size, because w25x methods are hungry
target_compile_definitions(firmware PRIVATE CRASH_CATCHER_STACK_WORD_COUNT=160)
