add_library(log_dev_null STATIC src/log_dev_null.cpp)
target_link_libraries(log_dev_null PUBLIC log_interface)
