add_library(${PROJECT_NAME}-warnings INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME}-warnings ALIAS ${PROJECT_NAME}-warnings)

if(CMAKE_CXX_COMPILER_ID STREQUAL Clang OR CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  target_compile_options(${PROJECT_NAME}-warnings INTERFACE
    -Wall -Wextra -Wpedantic -Wconversion -Werror=return-type
  )
endif()

add_library(${PROJECT_NAME}-ndwerror INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME}-ndwerror ALIAS ${PROJECT_NAME}-ndwerror)

if(CMAKE_CXX_COMPILER_ID STREQUAL Clang OR CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  target_compile_options(${PROJECT_NAME}-ndwerror INTERFACE
    $<$<NOT:$<CONFIG:Debug>>:-Werror>
  )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL MSVC)
  target_compile_options(${PROJECT_NAME}-ndwerror
    $<$<NOT:$<CONFIG:Debug>>:/WX>
  )
endif()
