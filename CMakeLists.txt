cmake_minimum_required(VERSION 3.0)
project(nxtlauncher CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang"
OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wconversion")

	# -pthread required to enable std::thread
	# Other platforms may not need it / require a different flag
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

# -----------

add_executable(nxtlauncher
	src/nxt_config.hpp
	src/nxt_crc32.hpp
	src/nxt_decompress.hpp
	src/nxt_fifo.hpp
	src/nxt_file.hpp
	src/nxt_http.hpp
	src/nxt_ipc.hpp
	src/nxt_log.hpp
	src/nxt_message.hpp

	src/nxt_config.cpp
	src/nxt_crc32.cpp
	src/nxt_decompress.cpp
	src/nxt_fifo.cpp
	src/nxt_file.cpp
	src/nxt_http.cpp
	src/nxt_ipc.cpp
	src/nxt_log.cpp
	src/nxt_message.cpp

	src/main.cpp
)

install(TARGETS nxtlauncher
	RUNTIME DESTINATION bin
)

# -----------

find_package(CURL REQUIRED)
find_package(LibLZMA REQUIRED)
find_package(X11)

target_include_directories(nxtlauncher SYSTEM PRIVATE ${CURL_INCLUDE_DIRS} ${LIBLZMA_INCLUDE_DIRS})
target_link_libraries(nxtlauncher ${CURL_LIBRARIES} ${LIBLZMA_LIBRARIES})

if(X11_FOUND)
	target_include_directories(nxtlauncher SYSTEM PRIVATE ${X11_INCLUDE_DIRS})
	target_link_libraries(nxtlauncher ${X11_LIBRARIES})
	target_compile_definitions(nxtlauncher PRIVATE ENABLE_X11)
else()
	message(WARNING "X11 library not found - window title will not be set")
endif()

# for dlopen, dlsym, dlclose
target_link_libraries(nxtlauncher dl)

# -----------

# Helper script
# configure_file(runescape runescape COPYONLY)
file(COPY runescape DESTINATION .
	FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
	                 GROUP_READ             GROUP_EXECUTE
	                 WORLD_READ             WORLD_EXECUTE
)

# Install nxtlauncher
install(TARGETS nxtlauncher RUNTIME DESTINATION bin)

# Install helper script
install(FILES runescape DESTINATION bin
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
	            GROUP_READ             GROUP_EXECUTE
	            WORLD_READ             WORLD_EXECUTE
)

