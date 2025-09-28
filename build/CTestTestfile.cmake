# CMake generated Testfile for 
# Source directory: /Users/danielding/euclid-engine
# Build directory: /Users/danielding/euclid-engine/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[smoke]=] "/Users/danielding/euclid-engine/build/smoketest")
set_tests_properties([=[smoke]=] PROPERTIES  _BACKTRACE_TRIPLES "/Users/danielding/euclid-engine/CMakeLists.txt;47;add_test;/Users/danielding/euclid-engine/CMakeLists.txt;0;")
add_test([=[fen]=] "/Users/danielding/euclid-engine/build/fen_test")
set_tests_properties([=[fen]=] PROPERTIES  _BACKTRACE_TRIPLES "/Users/danielding/euclid-engine/CMakeLists.txt;52;add_test;/Users/danielding/euclid-engine/CMakeLists.txt;0;")
add_test([=[perft_smoke]=] "/Users/danielding/euclid-engine/build/perft_smoke")
set_tests_properties([=[perft_smoke]=] PROPERTIES  _BACKTRACE_TRIPLES "/Users/danielding/euclid-engine/CMakeLists.txt;57;add_test;/Users/danielding/euclid-engine/CMakeLists.txt;0;")
add_test([=[king_smoke]=] "/Users/danielding/euclid-engine/build/king_smoke")
set_tests_properties([=[king_smoke]=] PROPERTIES  _BACKTRACE_TRIPLES "/Users/danielding/euclid-engine/CMakeLists.txt;61;add_test;/Users/danielding/euclid-engine/CMakeLists.txt;0;")
add_test([=[bishop_smoke]=] "/Users/danielding/euclid-engine/build/bishop_smoke")
set_tests_properties([=[bishop_smoke]=] PROPERTIES  _BACKTRACE_TRIPLES "/Users/danielding/euclid-engine/CMakeLists.txt;65;add_test;/Users/danielding/euclid-engine/CMakeLists.txt;0;")
add_test([=[rook_smoke]=] "/Users/danielding/euclid-engine/build/rook_smoke")
set_tests_properties([=[rook_smoke]=] PROPERTIES  _BACKTRACE_TRIPLES "/Users/danielding/euclid-engine/CMakeLists.txt;69;add_test;/Users/danielding/euclid-engine/CMakeLists.txt;0;")
add_test([=[queen_smoke]=] "/Users/danielding/euclid-engine/build/queen_smoke")
set_tests_properties([=[queen_smoke]=] PROPERTIES  _BACKTRACE_TRIPLES "/Users/danielding/euclid-engine/CMakeLists.txt;73;add_test;/Users/danielding/euclid-engine/CMakeLists.txt;0;")
