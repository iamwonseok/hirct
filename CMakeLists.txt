cmake_minimum_required(VERSION 3.20)
project(hirct LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(HIRCT_ENABLE_UNITTESTS "Build unit tests (requires googletest)" OFF)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(lib)
add_subdirectory(tools)

# lit
find_program(LIT_COMMAND lit REQUIRED)
find_program(FILECHECK_COMMAND FileCheck REQUIRED)
add_subdirectory(test)

# gtest (optional — requires network or local googletest source)
if(HIRCT_ENABLE_UNITTESTS)
  include(FetchContent)
  FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0)
  FetchContent_MakeAvailable(googletest)
  enable_testing()
  add_subdirectory(unittests)
endif()
