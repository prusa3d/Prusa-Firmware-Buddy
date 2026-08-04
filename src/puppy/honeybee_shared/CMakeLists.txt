target_sources(
  firmware PRIVATE cyphal_node.cpp salted_app_hash_command_server.cpp self_hash.cpp watcher.cpp
  )
target_link_libraries(firmware PRIVATE otp)
