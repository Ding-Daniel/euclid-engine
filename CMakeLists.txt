cmake_minimum_required(VERSION 3.20)
project(euclid_engine VERSION 0.1.0 LANGUAGES CXX)

# Default to Release if nothing is specified (single-config generators)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "No build type selected, defaulting to Release")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------- Optional ORT support ----------------
# Expect FindONNXRuntime.cmake to live at: <repo>/cmake/FindONNXRuntime.cmake
option(EUCLID_ENABLE_ORT "Enable ONNX Runtime evaluation backend (requires onnxruntime installed)" OFF)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_library(euclid_engine STATIC
  src/board.cpp
  src/zobrist.cpp
  src/fen.cpp
  src/movegen.cpp
  src/perft.cpp
  src/attack.cpp
  src/move_do.cpp
  src/attacks_tbl.cpp
  src/eval.cpp
  src/uci.cpp
  src/search.cpp
  src/tt.cpp
  src/encode.cpp
  src/nn.cpp
  src/nn_eval.cpp
  src/ort_eval.cpp
  src/game.cpp
  src/selfplay.cpp
  src/dataset.cpp
)
target_include_directories(euclid_engine PUBLIC include)

# Warnings
if(MSVC)
  target_compile_options(euclid_engine PRIVATE /W4 /permissive-)
else()
  target_compile_options(euclid_engine PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()

if(EUCLID_ENABLE_ORT)
  find_package(ONNXRuntime REQUIRED)
  target_compile_definitions(euclid_engine PUBLIC EUCLID_USE_ORT=1)
  target_link_libraries(euclid_engine PUBLIC ONNXRuntime::onnxruntime)
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

add_executable(pin_smoke tests/pin_smoke.cpp)
target_link_libraries(pin_smoke PRIVATE euclid_engine)
add_test(NAME pin_smoke COMMAND $<TARGET_FILE:pin_smoke>)

add_executable(incheck_smoke tests/incheck_smoke.cpp)
target_link_libraries(incheck_smoke PRIVATE euclid_engine)
add_test(NAME incheck_smoke COMMAND $<TARGET_FILE:incheck_smoke>)

add_executable(do_undo_smoke tests/do_undo_smoke.cpp)
target_link_libraries(do_undo_smoke PRIVATE euclid_engine)
add_test(NAME do_undo_smoke COMMAND $<TARGET_FILE:do_undo_smoke>)

add_executable(castle_smoke tests/castle_smoke.cpp)
target_link_libraries(castle_smoke PRIVATE euclid_engine)
add_test(NAME castle_smoke COMMAND $<TARGET_FILE:castle_smoke>)

add_executable(perft_refs tests/perft_refs.cpp)
target_link_libraries(perft_refs PRIVATE euclid_engine)
add_test(NAME perft_refs COMMAND $<TARGET_FILE:perft_refs>)

add_executable(eval_smoke tests/eval_smoke.cpp)
target_link_libraries(eval_smoke PRIVATE euclid_engine)
add_test(NAME eval_smoke COMMAND $<TARGET_FILE:eval_smoke>)

add_executable(uci_smoke tests/uci_smoke.cpp)
target_link_libraries(uci_smoke PRIVATE euclid_engine)
add_test(NAME uci_smoke COMMAND $<TARGET_FILE:uci_smoke>)

add_executable(search_smoke tests/search_smoke.cpp)
target_link_libraries(search_smoke PRIVATE euclid_engine)
add_test(NAME search_smoke COMMAND $<TARGET_FILE:search_smoke>)

add_executable(search_limits_smoke tests/search_limits_smoke.cpp)
target_link_libraries(search_limits_smoke PRIVATE euclid_engine)
add_test(NAME search_limits_smoke COMMAND $<TARGET_FILE:search_limits_smoke>)

add_executable(nullmove_state_smoke tests/nullmove_state_smoke.cpp)
target_link_libraries(nullmove_state_smoke PRIVATE euclid_engine)
add_test(NAME nullmove_state_smoke COMMAND $<TARGET_FILE:nullmove_state_smoke>)

add_executable(draw_rules_smoke tests/draw_rules_smoke.cpp)
target_link_libraries(draw_rules_smoke PRIVATE euclid_engine)
add_test(NAME draw_rules_smoke COMMAND $<TARGET_FILE:draw_rules_smoke>)

add_executable(encode_smoke tests/encode_smoke.cpp)
target_link_libraries(encode_smoke PRIVATE euclid_engine)
add_test(NAME encode_smoke COMMAND $<TARGET_FILE:encode_smoke>)

add_executable(nn_smoke tests/nn_smoke.cpp)
target_link_libraries(nn_smoke PRIVATE euclid_engine)
add_test(NAME nn_smoke COMMAND $<TARGET_FILE:nn_smoke>)

add_executable(nn_eval_integration_smoke tests/nn_eval_integration_smoke.cpp)
target_link_libraries(nn_eval_integration_smoke PRIVATE euclid_engine)
add_test(NAME nn_eval_integration_smoke COMMAND $<TARGET_FILE:nn_eval_integration_smoke>)

add_executable(search_nn_pov_smoke tests/search_nn_pov_smoke.cpp)
target_link_libraries(search_nn_pov_smoke PRIVATE euclid_engine)
add_test(NAME search_nn_pov_smoke COMMAND $<TARGET_FILE:search_nn_pov_smoke>)

add_executable(uci_evalmodel_smoke tests/uci_evalmodel_smoke.cpp)
target_link_libraries(uci_evalmodel_smoke PRIVATE euclid_engine)
add_test(NAME uci_evalmodel_smoke COMMAND $<TARGET_FILE:uci_evalmodel_smoke>)

add_executable(nn_make_const_smoke tests/nn_make_const_smoke.cpp)
target_link_libraries(nn_make_const_smoke PRIVATE euclid_engine)
add_dependencies(nn_make_const_smoke euclid_cli)
add_test(NAME nn_make_const_smoke COMMAND $<TARGET_FILE:nn_make_const_smoke>)

add_executable(selfplay_smoke tests/selfplay_smoke.cpp)
target_link_libraries(selfplay_smoke PRIVATE euclid_engine)
add_test(NAME selfplay_smoke COMMAND $<TARGET_FILE:selfplay_smoke>)

add_executable(eval_sanity_smoke tests/eval_sanity_smoke.cpp)
target_link_libraries(eval_sanity_smoke PRIVATE euclid_engine)
add_test(NAME eval_sanity_smoke COMMAND $<TARGET_FILE:eval_sanity_smoke>)

add_executable(dataset_smoke tests/dataset_smoke.cpp)
target_link_libraries(dataset_smoke PRIVATE euclid_engine)
add_test(NAME dataset_smoke COMMAND $<TARGET_FILE:dataset_smoke>)
