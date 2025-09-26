add_library(${PROJECT_NAME})
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

target_compile_definitions(${PROJECT_NAME} PUBLIC
  $<$<CONFIG:Debug>:KLIB_DEBUG>
  $<$<BOOL:${KLIB_USE_STACKTRACE}>:KLIB_USE_STACKTRACE>
)

set(stacktrace_lib "")

if(CMAKE_HOST_UNIX OR MINGW)
  set(stacktrace_lib "stdc++exp")
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
  $<$<BOOL:${KLIB_USE_STACKTRACE}>:${stacktrace_lib}>
)

target_include_directories(${PROJECT_NAME} PRIVATE
  src
)

set(version_header "${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME}/build_version.hpp")
configure_file(src/build_version.hpp.in "{version_header}")

file(GLOB_RECURSE headers LIST_DIRECTORIES false "include/*.hpp")

target_sources(${PROJECT_NAME} PUBLIC FILE_SET HEADERS
  BASE_DIRS
  include
  "${CMAKE_CURRENT_BINARY_DIR}/include"

  FILES
  ${headers}
  "${version_header}"
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

if(KLIB_INSTALL)
  message(STATUS "[klib] setting up install")
  include(GNUInstallDirs)

  install(
    TARGETS klib
    EXPORT klib-targets
    FILE_SET HEADERS
  )

  install(
    EXPORT klib-targets
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/klib"
    NAMESPACE klib::
    FILE klib-config.cmake
  )
endif()
