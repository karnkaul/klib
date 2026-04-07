cmake_minimum_required(VERSION 3.23)

set(package_name klib)
project(${package_name} VERSION "0.6.4")

option(KLIB_USE_STACKTRACE "Enable use of std::stacktrace" ON)
option(KLIB_UNIT_TEST "Enable klib unit test API" ${PROJECT_IS_TOP_LEVEL})
option(KLIB_INSTALL "Setup CMake install for klib" ${PROJECT_IS_TOP_LEVEL})

option(KLIB_BUILD_TESTS "Build klib tests" ${PROJECT_IS_TOP_LEVEL})

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_DEBUG_POSTFIX "-d")

function(klib_set_mainCRTStartup TARGET)
  if(WIN32 AND NOT MINGW)
    message(STATUS "adding /ENTRY:mainCRTStartup link option to ${TARGET}")
    target_link_options(${TARGET} PRIVATE
      "LINKER:/ENTRY:mainCRTStartup"
    )
  endif()
endfunction()

if(NOT KLIB_UNIT_TEST AND KLIB_BUILD_TESTS)
  message(WARNING "[klib] cannot build tests without unit test API")
  set(KLIB_BUILD_TESTS OFF)
endif()

if(NOT KLIB_UNIT_TEST AND KLIB_INSTALL)
  message(WARNING "[klib] cannot install without unit test API")
  set(KLIB_INSTALL OFF)
endif()

add_subdirectory(lib)

if(KLIB_INSTALL)
  message(STATUS "[${package_name}] setting up install")
  include(GNUInstallDirs)

  set(install_dst "${CMAKE_INSTALL_LIBDIR}/cmake/${package_name}")

  install(
    TARGETS ${package_name} ${package_name}-test-main
    EXPORT ${package_name}-targets
    FILE_SET HEADERS
  )

  install(
    EXPORT ${package_name}-targets
    DESTINATION "${install_dst}"
    NAMESPACE ${package_name}::
    FILE ${package_name}-config.cmake
  )
endif()

if(KLIB_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
