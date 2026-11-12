cmake_minimum_required(VERSION 4.4)

set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "f35a9ac6-8463-4d38-8eec-5d6008153e7d")
set(CMAKE_CXX_MODULE_STD ON)

set(package_name klib)
project(${package_name} VERSION "0.7.1")

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

add_subdirectory(lib)

if(KLIB_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
