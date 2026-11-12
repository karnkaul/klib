add_library(${package_name})
add_library(${package_name}::${package_name} ALIAS ${package_name})

target_compile_features(${package_name} PUBLIC cxx_std_23)

target_compile_definitions(${package_name} PUBLIC
  $<$<CONFIG:Debug>:KLIB_DEBUG>
)

if (WIN32)
  target_compile_definitions(${package_name} PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
  )
endif()

set(stacktrace_lib "")

if(CMAKE_HOST_UNIX OR MINGW)
  set(stacktrace_lib "stdc++exp")
endif()

target_link_libraries(${package_name} PRIVATE
  ${stacktrace_lib}
)

target_include_directories(${package_name} PRIVATE
  src
)

set(version_module src/build_version.cppm)
configure_file(src/build_version.cppm.in ${version_module} @ONLY)

target_sources(${package_name} PUBLIC FILE_SET HEADERS
  BASE_DIRS include
  FILES include/${package_name}/unit_test/unit_test.hpp
)

target_sources(${package_name} PUBLIC FILE_SET CXX_MODULES
  FILES
  src/core_string.cppm
  src/core.cppm
  src/log.cppm
  src/cli.cppm
  src/task.cppm
  src/unit_test.cppm
)

target_sources(${package_name} PUBLIC FILE_SET CXX_MODULES
  BASE_DIRS "${CMAKE_CURRENT_BINARY_DIR}"
  FILES "${CMAKE_CURRENT_BINARY_DIR}/${version_module}"
)

add_library(${package_name}-test-main)
add_library(${package_name}::${package_name}-test-main ALIAS ${package_name}-test-main)

target_link_libraries(${package_name}-test-main PUBLIC
  ${package_name}::${package_name}
)

target_sources(${package_name}-test-main PRIVATE
  src/unit_test_main.cpp
)
