# CMake generated Testfile for 
# Source directory: C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests
# Build directory: C:/pbrt-v3-master/pbrt-cmake/src/ext/ptex/src/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(wtest "C:/pbrt-v3-master/pbrt-cmake/src/ext/ptex/src/tests/Debug/wtest.exe")
  set_tests_properties(wtest PROPERTIES  _BACKTRACE_TRIPLES "C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;32;add_test;C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(wtest "C:/pbrt-v3-master/pbrt-cmake/src/ext/ptex/src/tests/Release/wtest.exe")
  set_tests_properties(wtest PROPERTIES  _BACKTRACE_TRIPLES "C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;32;add_test;C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(wtest "C:/pbrt-v3-master/pbrt-cmake/src/ext/ptex/src/tests/MinSizeRel/wtest.exe")
  set_tests_properties(wtest PROPERTIES  _BACKTRACE_TRIPLES "C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;32;add_test;C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(wtest "C:/pbrt-v3-master/pbrt-cmake/src/ext/ptex/src/tests/RelWithDebInfo/wtest.exe")
  set_tests_properties(wtest PROPERTIES  _BACKTRACE_TRIPLES "C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;32;add_test;C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;0;")
else()
  add_test(wtest NOT_AVAILABLE)
endif()
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(rtest "C:/Program Files/CMake/bin/cmake.exe" "-DOUT=C:/pbrt-v3-master/pbrt-cmake/src/ext/ptex/src/tests/rtest.out" "-DDATA=C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/rtestok.dat" "-DCMD=C:/pbrt-v3-master/pbrt-cmake/src/ext/ptex/src/tests/Debug/rtest.exe" "-P" "C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/compare_test.cmake")
  set_tests_properties(rtest PROPERTIES  _BACKTRACE_TRIPLES "C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;23;add_test;C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;33;add_compare_test;C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(rtest "C:/Program Files/CMake/bin/cmake.exe" "-DOUT=C:/pbrt-v3-master/pbrt-cmake/src/ext/ptex/src/tests/rtest.out" "-DDATA=C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/rtestok.dat" "-DCMD=C:/pbrt-v3-master/pbrt-cmake/src/ext/ptex/src/tests/Release/rtest.exe" "-P" "C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/compare_test.cmake")
  set_tests_properties(rtest PROPERTIES  _BACKTRACE_TRIPLES "C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;23;add_test;C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;33;add_compare_test;C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(rtest "C:/Program Files/CMake/bin/cmake.exe" "-DOUT=C:/pbrt-v3-master/pbrt-cmake/src/ext/ptex/src/tests/rtest.out" "-DDATA=C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/rtestok.dat" "-DCMD=C:/pbrt-v3-master/pbrt-cmake/src/ext/ptex/src/tests/MinSizeRel/rtest.exe" "-P" "C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/compare_test.cmake")
  set_tests_properties(rtest PROPERTIES  _BACKTRACE_TRIPLES "C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;23;add_test;C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;33;add_compare_test;C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(rtest "C:/Program Files/CMake/bin/cmake.exe" "-DOUT=C:/pbrt-v3-master/pbrt-cmake/src/ext/ptex/src/tests/rtest.out" "-DDATA=C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/rtestok.dat" "-DCMD=C:/pbrt-v3-master/pbrt-cmake/src/ext/ptex/src/tests/RelWithDebInfo/rtest.exe" "-P" "C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/compare_test.cmake")
  set_tests_properties(rtest PROPERTIES  _BACKTRACE_TRIPLES "C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;23;add_test;C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;33;add_compare_test;C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;0;")
else()
  add_test(rtest NOT_AVAILABLE)
endif()
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(ftest "C:/Program Files/CMake/bin/cmake.exe" "-DOUT=C:/pbrt-v3-master/pbrt-cmake/src/ext/ptex/src/tests/ftest.out" "-DDATA=C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/ftestok.dat" "-DCMD=C:/pbrt-v3-master/pbrt-cmake/src/ext/ptex/src/tests/Debug/ftest.exe" "-P" "C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/compare_test.cmake")
  set_tests_properties(ftest PROPERTIES  _BACKTRACE_TRIPLES "C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;23;add_test;C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;34;add_compare_test;C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(ftest "C:/Program Files/CMake/bin/cmake.exe" "-DOUT=C:/pbrt-v3-master/pbrt-cmake/src/ext/ptex/src/tests/ftest.out" "-DDATA=C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/ftestok.dat" "-DCMD=C:/pbrt-v3-master/pbrt-cmake/src/ext/ptex/src/tests/Release/ftest.exe" "-P" "C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/compare_test.cmake")
  set_tests_properties(ftest PROPERTIES  _BACKTRACE_TRIPLES "C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;23;add_test;C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;34;add_compare_test;C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(ftest "C:/Program Files/CMake/bin/cmake.exe" "-DOUT=C:/pbrt-v3-master/pbrt-cmake/src/ext/ptex/src/tests/ftest.out" "-DDATA=C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/ftestok.dat" "-DCMD=C:/pbrt-v3-master/pbrt-cmake/src/ext/ptex/src/tests/MinSizeRel/ftest.exe" "-P" "C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/compare_test.cmake")
  set_tests_properties(ftest PROPERTIES  _BACKTRACE_TRIPLES "C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;23;add_test;C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;34;add_compare_test;C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(ftest "C:/Program Files/CMake/bin/cmake.exe" "-DOUT=C:/pbrt-v3-master/pbrt-cmake/src/ext/ptex/src/tests/ftest.out" "-DDATA=C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/ftestok.dat" "-DCMD=C:/pbrt-v3-master/pbrt-cmake/src/ext/ptex/src/tests/RelWithDebInfo/ftest.exe" "-P" "C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/compare_test.cmake")
  set_tests_properties(ftest PROPERTIES  _BACKTRACE_TRIPLES "C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;23;add_test;C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;34;add_compare_test;C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;0;")
else()
  add_test(ftest NOT_AVAILABLE)
endif()
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(halftest "C:/pbrt-v3-master/pbrt-cmake/src/ext/ptex/src/tests/Debug/halftest.exe")
  set_tests_properties(halftest PROPERTIES  _BACKTRACE_TRIPLES "C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;35;add_test;C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(halftest "C:/pbrt-v3-master/pbrt-cmake/src/ext/ptex/src/tests/Release/halftest.exe")
  set_tests_properties(halftest PROPERTIES  _BACKTRACE_TRIPLES "C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;35;add_test;C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(halftest "C:/pbrt-v3-master/pbrt-cmake/src/ext/ptex/src/tests/MinSizeRel/halftest.exe")
  set_tests_properties(halftest PROPERTIES  _BACKTRACE_TRIPLES "C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;35;add_test;C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(halftest "C:/pbrt-v3-master/pbrt-cmake/src/ext/ptex/src/tests/RelWithDebInfo/halftest.exe")
  set_tests_properties(halftest PROPERTIES  _BACKTRACE_TRIPLES "C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;35;add_test;C:/pbrt-v3-master/pbrt-v3/src/ext/ptex/src/tests/CMakeLists.txt;0;")
else()
  add_test(halftest NOT_AVAILABLE)
endif()
