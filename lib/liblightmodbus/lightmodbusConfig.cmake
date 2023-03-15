if (TARGET lightmodbus)
	return()
endif()

add_library(lightmodbus::lightmodbus INTERFACE IMPORTED)
target_include_directories(lightmodbus::lightmodbus INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)