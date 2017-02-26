cmake_minimum_required (VERSION 2.6)

project(takai)

set(CMAKE_CXX_FLAGS "-std=c++11 -Lc++ -Ofast")

add_executable (takai
	main.cpp
	board.cpp
	hash.cpp
	movegen.cpp
	player_human.cpp
	player_minmax.cpp
)
add_executable (tournament tournament.cpp board.cpp hash.cpp movegen.cpp)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)
