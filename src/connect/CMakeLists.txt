target_sources(
  firmware
  PRIVATE background.cpp
          command.cpp
          command_id.cpp
          connect.cpp
          connection_cache.cpp
          hostname.cpp
          json_out.cpp
          marlin_printer.cpp
          planner.cpp
          printer.cpp
          registrator.cpp
          render.cpp
          run.cpp
          sleep.cpp
  )

add_subdirectory(tls)

target_compile_definitions(firmware PRIVATE BUDDY_ENABLE_CONNECT)
