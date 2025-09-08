# CMakeList.txt: MySqlClient 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

#引入 vcpkg 工具链文件
set(CMAKE_TOOLCHAIN_FILE "E:/Software/Vcpkg/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING  "Vcpkg toolchain file")

project ("MySqlClient")
set(CMAKE_CXX_STANDARD 20)

# 查找 vcpkg 安装的 GTest，明确指定组件
find_package(GTest REQUIRED)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/input")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/core")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/database")

# 包含第三方库头文件目录
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/thirdparty")

# 收集主程序需要的源文件。
file(GLOB MAIN_SOURCES "src/*.cpp" "src/input/*.cpp" "src/core/*.cpp" "src/database/*.cpp")
file(GLOB  MAIN_HEADERS "include/*.h" "include/*/*.h")

# 收集测试需要的源文件。
file(GLOB TEST_CONSOLE_SOURCE "tests/test_console_input.cpp"  "src/input/input_console.cpp")
file(GLOB TEST_SQLQUERY_PARSER_SOURCE "tests/test_sqlquery_parser.cpp"  "src/core/sql_parser.cpp")
file(GLOB TEST_DRIVER_SOURCE "tests/test_driver.cpp" "src/database/json_driver.cpp" )
# 设置输出目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

# 将源代码添加到此项目的可执行文件。
add_executable(app ${MAIN_SOURCES} ${MAIN_HEADERS})

# 生成测试可执行文件。
add_executable(test_console ${TEST_CONSOLE_SOURCE})
target_link_libraries(
    test_console
    PRIVATE GTest::gtest_main
    PRIVATE GTest::gtest
)
add_executable(test_parser ${TEST_SQLQUERY_PARSER_SOURCE})
target_link_libraries(
    test_parser
    PRIVATE GTest::gtest_main
    PRIVATE GTest::gtest
)

add_executable(test_driver ${TEST_DRIVER_SOURCE})
target_link_libraries(
    test_driver
    PRIVATE GTest::gtest_main
    PRIVATE GTest::gtest
)


if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET app PROPERTY CXX_STANDARD 20)
endif()

# TODO: 如有需要，请添加测试并安装目标。
# 启用测试
enable_testing()
# 引入 GoogleTest 提供的测试发现相关功能
include(GoogleTest)
# 注册测试用例，让 CMake 能发现测试
gtest_discover_tests(test_console test_parser test_driver)
