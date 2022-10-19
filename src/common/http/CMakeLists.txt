target_sources(
  firmware
  PRIVATE connection.cpp
          httpc.cpp
          os_porting.cpp
          post_file_request.cpp
          resp_parser.cpp
          socket.cpp
          socket_connection_factory.cpp
          url_decode.cpp
  )
