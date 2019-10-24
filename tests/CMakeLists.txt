project(stdthreadtest)
cmake_minimum_required(VERSION 2.8)

add_definitions(-std=c++11 -Wall -Wextra)
add_executable(${PROJECT_NAME} tests.cpp)

# Statically link libraries if needed
if(CMAKE_BUILD_TYPE MATCHES "Static")
target_link_libraries(${PROJECT_NAME} -static-libgcc -static-libstdc++ -static)
endif()

# Test std-like headers
# Build type: 
# - StdLikeHeaders
# - StdLikeHeadersCompilerWrapped
if(CMAKE_BUILD_TYPE MATCHES "StdLikeHeaders")
  # let test.cpp use std header (for example, <thread> instead of "mingw.thread.h")
  add_definitions(-DUSE_STD_LIKE_HEADERS)

  set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
  add_custom_target(GENERATED_STD_LIKE_HEADERS
    COMMAND "${PROJECT_SOURCE_DIR}/../tools/generate_std_like_headers" 
        -DestinationFolder ${GENERATED_DIR}
        -GenerateCompilerWrapperWithFileName "${GENERATED_DIR}/mingw-std-headers-g++.bat"
  )
  add_dependencies(${PROJECT_NAME} GENERATED_STD_LIKE_HEADERS)
  # Add include directories
  add_definitions("-I${GENERATED_DIR}")
endif()