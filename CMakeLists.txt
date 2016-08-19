cmake_minimum_required(VERSION 3.0)
project(nxtlauncher CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang"
OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wconversion")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -fno-rtti")
endif()

# -----------

add_executable(nxtlauncher
	src/nxt_config.hpp
	src/nxt_fifo.hpp
	src/nxt_ipc.hpp
	src/nxt_log.hpp
	src/nxt_message.hpp

	src/nxt_config.cpp
	src/nxt_fifo.cpp
	src/nxt_ipc.cpp
	src/nxt_log.cpp
	src/nxt_message.cpp

	src/main.cpp
)

install(TARGETS nxtlauncher
	RUNTIME DESTINATION bin
)

# -----------

target_link_libraries(nxtlauncher dl)

