cmake_minimum_required(VERSION 3.14)
project(mini_json LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# json.hpp 是 header-only，用 INTERFACE 目标表达「只需要头文件搜索路径」
add_library(mini_json INTERFACE)
target_include_directories(mini_json INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# 示例程序：不依赖任何第三方库
add_executable(json_demo main.cpp)
target_link_libraries(json_demo PRIVATE mini_json)

# 单元测试：需要 GoogleTest
find_package(GTest QUIET)
if(GTest_FOUND OR GTEST_FOUND)
    enable_testing()
    add_executable(json_tests tests/test_json.cpp)
    target_link_libraries(json_tests PRIVATE mini_json GTest::gtest GTest::gtest_main)
    add_test(NAME json_tests COMMAND json_tests)
    message(STATUS "GoogleTest 已找到，单元测试目标 json_tests 已启用")
else()
    message(STATUS "未找到 GoogleTest，跳过 json_tests（json_demo 仍可编译）")
    message(STATUS "MSYS2 安装方式: pacman -S mingw-w64-ucrt-x86_64-gtest")
endif()
