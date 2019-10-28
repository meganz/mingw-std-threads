project(stdthreadtest)
cmake_minimum_required(VERSION 3.0)

string(CONCAT mingw_stdthreads_tests_compile_options_docstring
       "Compiler flags used to compile mingw-stdthreads's tests. By default "
       "it's -std=c++11 -Wall -Wextra")
set(MINGW_STDTHREADS_TESTS_COMPILE_OPTIONS "-std=c++11;-Wall;-Wextra" 
    CACHE STRING ${mingw_stdthreads_tests_compile_options_docstring})

set(MINGW_STDTHREADS_TESTS_ADDITIONAL_LINKER_FLAGS "" CACHE STRING 
    "Optional linker flags to be passed when linking mingw-stdthreads's tests")

add_executable(${PROJECT_NAME} tests.cpp)
target_compile_options(${PROJECT_NAME} PRIVATE
                       ${MINGW_STDTHREADS_TESTS_COMPILE_OPTIONS})
target_link_libraries(${PROJECT_NAME} PRIVATE mingw_stdthreads)
target_link_libraries(${PROJECT_NAME} PRIVATE 
                      ${MINGW_STDTHREADS_TESTS_ADDITIONAL_LINKER_FLAGS})