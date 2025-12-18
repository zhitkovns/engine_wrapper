cmake_minimum_required(VERSION 3.10)
project(CommandPattern)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(test
    main.cpp
    tests/test.cpp
)

target_include_directories(test PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/test_class
    ${CMAKE_SOURCE_DIR}/tests
)

# Для отладки
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(test PRIVATE -g -O0)
else()
    target_compile_options(test PRIVATE -O2)
endif()

set_target_properties(test PROPERTIES OUTPUT_NAME "test")