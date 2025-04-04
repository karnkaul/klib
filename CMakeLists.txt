cmake_minimum_required(VERSION 3.23)

project(klib VERSION "0.4.1")

option(KLIB_USE_STACKTRACE "Enable use of std::stacktrace" ON)

option(KLIB_BUILD_TESTS "Build klib tests" ${PROJECT_IS_TOP_LEVEL})

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_DEBUG_POSTFIX "-d")

include(cmake/interfaces.cmake)
add_subdirectory(lib)

if(KLIB_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
