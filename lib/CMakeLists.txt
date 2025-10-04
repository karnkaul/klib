add_library(${package_name})
add_library(${package_name}::${package_name} ALIAS ${package_name})

target_compile_features(${package_name} PUBLIC cxx_std_23)

target_compile_definitions(${package_name} PUBLIC
  $<$<CONFIG:Debug>:KLIB_DEBUG>
  $<$<BOOL:${KLIB_USE_STACKTRACE}>:KLIB_USE_STACKTRACE>
)

set(stacktrace_lib "")

if(CMAKE_HOST_UNIX OR MINGW)
  set(stacktrace_lib "stdc++exp")
endif()

target_link_libraries(${package_name} PRIVATE
  $<$<BOOL:${KLIB_USE_STACKTRACE}>:${stacktrace_lib}>
)

target_include_directories(${package_name} PRIVATE
  src
)

set(version_header include/${package_name}/build_version.hpp)
configure_file(src/build_version.hpp.in ${version_header} @ONLY)

file(GLOB_RECURSE headers LIST_DIRECTORIES false "include/*.hpp")

target_sources(${package_name} PUBLIC FILE_SET HEADERS
  BASE_DIRS
  include
  "${CMAKE_CURRENT_BINARY_DIR}/include"

  FILES
  ${headers}
  "${CMAKE_CURRENT_BINARY_DIR}/${version_header}"
)

target_sources(${package_name} PRIVATE
  src/klib.cpp
)

add_library(${package_name}-test-main)
add_library(${package_name}::${package_name}-test-main ALIAS ${package_name}-test-main)

target_link_libraries(${package_name}-test-main PUBLIC
  ${package_name}::${package_name}
)

target_sources(${package_name}-test-main PRIVATE
  src/klib_test_main.cpp
)
