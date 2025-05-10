add_library(${PROJECT_NAME})
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

target_compile_definitions(${PROJECT_NAME} PUBLIC
  $<$<CONFIG:Debug>:KLIB_DEBUG>
  $<$<BOOL:${KLIB_USE_STACKTRACE}>:KLIB_USE_STACKTRACE>
)

set(stacktrace_lib "")

if(CMAKE_HOST_UNIX)
  set(stacktrace_lib "stdc++exp")
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
  $<$<BOOL:${KLIB_USE_STACKTRACE}>:${stacktrace_lib}>
)

target_include_directories(${PROJECT_NAME} PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
)

target_include_directories(${PROJECT_NAME} PRIVATE
  src
)

file(GLOB_RECURSE headers LIST_DIRECTORIES false "include/*.hpp")
target_sources(${PROJECT_NAME} PUBLIC FILE_SET HEADERS BASE_DIRS include FILES
  ${headers}
)

configure_file(cmake_src/build_version.hpp.in "${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME}/build_version.hpp")
target_sources(${PROJECT_NAME} PUBLIC FILE_SET HEADERS BASE_DIRS "${CMAKE_CURRENT_BINARY_DIR}/include" FILES
  "${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME}/build_version.hpp"
)

target_sources(${PROJECT_NAME} PRIVATE
  src/klib.cpp
)

add_library(${PROJECT_NAME}-test-main)
add_library(${PROJECT_NAME}::${PROJECT_NAME}-test-main ALIAS ${PROJECT_NAME}-test-main)

target_link_libraries(${PROJECT_NAME}-test-main PUBLIC
  ${PROJECT_NAME}::${PROJECT_NAME}
)

target_sources(${PROJECT_NAME}-test-main PRIVATE
  src/klib_test_main.cpp
)
