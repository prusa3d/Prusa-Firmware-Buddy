target_sources(
  firmware
  PRIVATE background.cpp
          command.cpp
          connect.cpp
          hostname.cpp
          marlin_printer.cpp
          planner.cpp
          printer.cpp
          render.cpp
          run.cpp
          sleep.cpp
  )

add_subdirectory(tls)

target_compile_definitions(firmware PRIVATE BUDDY_ENABLE_CONNECT)
