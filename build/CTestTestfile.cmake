# CMake generated Testfile for 
# Source directory: /Users/danielding/euclid-engine
# Build directory: /Users/danielding/euclid-engine/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[smoke]=] "/Users/danielding/euclid-engine/build/smoketest")
set_tests_properties([=[smoke]=] PROPERTIES  _BACKTRACE_TRIPLES "/Users/danielding/euclid-engine/CMakeLists.txt;36;add_test;/Users/danielding/euclid-engine/CMakeLists.txt;0;")
add_test([=[fen]=] "fen_test")
set_tests_properties([=[fen]=] PROPERTIES  _BACKTRACE_TRIPLES "/Users/danielding/euclid-engine/CMakeLists.txt;37;add_test;/Users/danielding/euclid-engine/CMakeLists.txt;0;")
