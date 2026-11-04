cmake_minimum_required(VERSION 3.16)
project(matrix_mul LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Threads REQUIRED)

function(strict_floating_point target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /fp:strict /W4 /utf-8)
    else()
        target_compile_options(${target} PRIVATE -fno-fast-math -ffp-contract=off -Wall -Wextra)
    endif()
endfunction()

add_library(matrix generate.cpp multiply.cpp)
target_link_libraries(matrix PUBLIC Threads::Threads)
strict_floating_point(matrix)

add_executable(matrix_app main.cpp)
target_link_libraries(matrix_app PRIVATE matrix)

enable_testing()
add_executable(matrix_tests tests.cpp)
target_link_libraries(matrix_tests PRIVATE matrix)
target_compile_definitions(matrix_tests PRIVATE
    TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/testdata/2048")
strict_floating_point(matrix_tests)
add_test(NAME matrix_tests COMMAND matrix_tests)

# Optional maintenance tool; tests only read the committed binary fixtures.
add_executable(generate_2048_fixture EXCLUDE_FROM_ALL tools/generate_2048.cpp)
strict_floating_point(generate_2048_fixture)
