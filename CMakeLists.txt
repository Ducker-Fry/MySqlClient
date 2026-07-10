cmake_minimum_required(VERSION 3.8)

if(POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
      "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_TOOLCHAIN_FILE
    "E:/Software/Vcpkg/vcpkg/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING
    "Vcpkg toolchain file")

project(MySqlClient)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(GTest REQUIRED)

if(MSVC)
  add_compile_options(/utf-8)
endif()

set(CORE_SOURCES
    src/query_app.cpp
    src/input/input_console.cpp
    src/input/input_file.cpp
    src/input/inputdata.cpp
    src/input/file_parser.cpp
    src/core/sql_parser.cpp
    src/database/json_driver.cpp
    src/database/sqlite_driver.cpp
    thirdparty/sqlite/sqlite3.c)

set(CORE_HEADERS
    include/MySqlClient.h
    include/query_app.h
    include/input/inputdata.h
    include/input/file_parser.h
    include/input/input_file.h
    include/input/input_console.h
    include/input/input_manager.h
    include/core/sql_parser.h
    include/database/json_driver.h
    include/database/sqlite_driver.h)

add_library(mysqlclient_lib ${CORE_SOURCES} ${CORE_HEADERS})
target_include_directories(
  mysqlclient_lib
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
         ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

add_executable(app src/main.cpp)
target_link_libraries(app PRIVATE mysqlclient_lib)

add_executable(test_console tests/test_console_input.cpp)
target_link_libraries(test_console PRIVATE mysqlclient_lib GTest::gtest GTest::gtest_main)

add_executable(test_parser tests/test_sqlquery_parser.cpp)
target_link_libraries(test_parser PRIVATE mysqlclient_lib GTest::gtest GTest::gtest_main)

add_executable(test_driver tests/test_driver.cpp)
target_link_libraries(test_driver PRIVATE mysqlclient_lib GTest::gtest GTest::gtest_main)

add_executable(test_sqlite_driver tests/test_sqlite_driver.cpp)
target_link_libraries(test_sqlite_driver PRIVATE mysqlclient_lib GTest::gtest GTest::gtest_main)

add_executable(test_app_cli tests/test_app_cli.cpp)
target_link_libraries(test_app_cli PRIVATE mysqlclient_lib GTest::gtest GTest::gtest_main)

enable_testing()
include(GoogleTest)
gtest_discover_tests(test_console)
gtest_discover_tests(test_parser)
gtest_discover_tests(test_driver)
gtest_discover_tests(test_sqlite_driver)
gtest_discover_tests(test_app_cli)
