target_sources(
  firmware
  PRIVATE httpc.cpp
          os_porting.cpp
          post_file_request.cpp
          post_lambda_request.cpp
          resp_parser.cpp
          socket.cpp
          socket_connection_factory.cpp
          url_decode.cpp
  )
