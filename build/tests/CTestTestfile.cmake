# CMake generated Testfile for 
# Source directory: D:/Github Repo/Homogeneous/tests
# Build directory: D:/Github Repo/Homogeneous/build/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[all_tests]=] "D:/Github Repo/Homogeneous/build/bin/Debug/homogeneous_tests.exe")
  set_tests_properties([=[all_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "D:/Github Repo/Homogeneous/tests/CMakeLists.txt;18;add_test;D:/Github Repo/Homogeneous/tests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[all_tests]=] "D:/Github Repo/Homogeneous/build/bin/Release/homogeneous_tests.exe")
  set_tests_properties([=[all_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "D:/Github Repo/Homogeneous/tests/CMakeLists.txt;18;add_test;D:/Github Repo/Homogeneous/tests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test([=[all_tests]=] "D:/Github Repo/Homogeneous/build/bin/MinSizeRel/homogeneous_tests.exe")
  set_tests_properties([=[all_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "D:/Github Repo/Homogeneous/tests/CMakeLists.txt;18;add_test;D:/Github Repo/Homogeneous/tests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test([=[all_tests]=] "D:/Github Repo/Homogeneous/build/bin/RelWithDebInfo/homogeneous_tests.exe")
  set_tests_properties([=[all_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "D:/Github Repo/Homogeneous/tests/CMakeLists.txt;18;add_test;D:/Github Repo/Homogeneous/tests/CMakeLists.txt;0;")
else()
  add_test([=[all_tests]=] NOT_AVAILABLE)
endif()
