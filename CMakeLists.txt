cmake_minimum_required(VERSION 3.20)
project(euclid_engine VERSION 0.1.0 LANGUAGES CXX)

# Default to Release if nothing is specified
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

# Tiny smoke test target (ctest integration is optional for now)
enable_testing()
add_executable(smoketest tests/smoke.cpp)
target_link_libraries(smoketest PRIVATE euclid_engine)
add_test(NAME smoke COMMAND smoketest)
