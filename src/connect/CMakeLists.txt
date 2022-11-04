target_sources(
  firmware
  PRIVATE connect.cpp
          command.cpp
          render.cpp
          planner.cpp
          printer.cpp
          marlin_printer.cpp
          run.cpp
          hostname.cpp
  )

add_subdirectory(tls)

target_compile_definitions(firmware PRIVATE BUDDY_ENABLE_CONNECT)
