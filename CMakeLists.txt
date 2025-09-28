cmake_minimum_required(VERSION 3.20)
project(euclid_engine VERSION 0.1.0 LANGUAGES CXX)


# Default to Release if nothing is specified (for single-config generators)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
message(STATUS "No build type selected, defaulting to Release")
set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


add_library(euclid_engine STATIC
src/board.cpp
src/zobrist.cpp
src/fen.cpp
src/movegen.cpp
src/perft.cpp
)


target_include_directories(euclid_engine PUBLIC include)


# Warnings
if(MSVC)
target_compile_options(euclid_engine PRIVATE /W4 /permissive-)
else()
target_compile_options(euclid_engine PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()


add_executable(euclid_cli src/main.cpp)
target_link_libraries(euclid_cli PRIVATE euclid_engine)


# Tests
enable_testing()


add_executable(smoketest tests/smoke.cpp)
target_link_libraries(smoketest PRIVATE euclid_engine)
add_test(NAME smoke COMMAND $<TARGET_FILE:smoketest>)


add_executable(fen_test tests/fen.cpp)
target_link_libraries(fen_test PRIVATE euclid_engine)
add_test(NAME fen COMMAND $<TARGET_FILE:fen_test>)


add_executable(perft_smoke tests/perft_smoke.cpp)
target_link_libraries(perft_smoke PRIVATE euclid_engine)
add_test(NAME perft_smoke COMMAND $<TARGET_FILE:perft_smoke>)

add_executable(king_smoke tests/king_smoke.cpp)
target_link_libraries(king_smoke PRIVATE euclid_engine)
add_test(NAME king_smoke COMMAND $<TARGET_FILE:king_smoke>)

add_executable(bishop_smoke tests/bishop_smoke.cpp)
target_link_libraries(bishop_smoke PRIVATE euclid_engine)
add_test(NAME bishop_smoke COMMAND $<TARGET_FILE:bishop_smoke>)

add_executable(rook_smoke tests/rook_smoke.cpp)
target_link_libraries(rook_smoke PRIVATE euclid_engine)
add_test(NAME rook_smoke COMMAND $<TARGET_FILE:rook_smoke>)

add_executable(queen_smoke tests/queen_smoke.cpp)
target_link_libraries(queen_smoke PRIVATE euclid_engine)
add_test(NAME queen_smoke COMMAND $<TARGET_FILE:queen_smoke>)

add_executable(pawn_smoke tests/pawn_smoke.cpp)
target_link_libraries(pawn_smoke PRIVATE euclid_engine)
add_test(NAME pawn_smoke COMMAND $<TARGET_FILE:pawn_smoke>)
