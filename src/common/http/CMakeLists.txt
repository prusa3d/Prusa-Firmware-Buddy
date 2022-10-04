target_sources(
  firmware
  PRIVATE url_decode.cpp
          httpc.cpp
          socket.cpp
          resp_parser.cpp
          os_porting.cpp
          socket_connection_factory.cpp
          post_file_request.cpp
  )
