target_sources(
  firmware
  PRIVATE command.cpp
          connect.cpp
          hostname.cpp
          marlin_printer.cpp
          planner.cpp
          printer.cpp
          render.cpp
          run.cpp
  )

add_subdirectory(tls)

target_compile_definitions(firmware PRIVATE BUDDY_ENABLE_CONNECT)
