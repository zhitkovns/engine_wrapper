cmake_minimum_required(VERSION 3.10)
project(CommandPattern)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Исполняемый файл создается в корне проекта
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

# Добавляем исполняемый файл
add_executable(command_test
    main.cpp
    tests/test.cpp
)

# Указываем директории для включения заголовочных файлов
target_include_directories(command_test PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/test_class
    ${CMAKE_SOURCE_DIR}/tests
)

# Для отладки
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(command_test PRIVATE -g -O0)
else()
    target_compile_options(command_test PRIVATE -O2)
endif()

# Установка выходного имени файла
set_target_properties(command_test PROPERTIES OUTPUT_NAME "command_test")