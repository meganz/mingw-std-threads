project(mingw_stdthreads)
cmake_minimum_required(VERSION 3.0)

option(MINGW_STDTHREADS_BUILD_TEST "Build tests")
option(MINGW_STDTHREADS_GENERATE_STDHEADERS "Generate std-like headers")

string(CONCAT mingw_stdthreads_dir_docstring 
       "Optional. When generating std-like headers , this variable can be set"
       "to manually specify the path to mingw-stdthreads directory containing"
       "original library headers.")
set(MINGW_STDTHREADS_DIR "${PROJECT_SOURCE_DIR}"
    CACHE PATH ${mingw_stdthreads_dir_docstring})

# mingw-stdthreads is a header-only library, so make it a INTERFACE target
add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE "${PROJECT_SOURCE_DIR}")

if(MINGW_STDTHREADS_GENERATE_STDHEADERS)
    add_subdirectory(cmake_stdheaders_generator)
    target_link_libraries(${PROJECT_NAME} INTERFACE cmake_stdheaders_generator)
endif()

if(MINGW_STDTHREADS_BUILD_TEST)
    add_subdirectory(tests)
endif()