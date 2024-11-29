add_library(${PROJECT_NAME})
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

target_link_libraries(${PROJECT_NAME} PRIVATE
  ${PROJECT_NAME}::${PROJECT_NAME}-compile-options
)

target_include_directories(${PROJECT_NAME} PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
)

target_include_directories(${PROJECT_NAME} PRIVATE
  .
  src
)

target_sources(${PROJECT_NAME} PUBLIC FILE_SET HEADERS BASE_DIRS include FILES
  include/klib/c_string.hpp
  include/klib/task_fwd.hpp
  include/klib/task_id.hpp
  include/klib/task_queue_create_info.hpp
  include/klib/task_queue_fwd.hpp
  include/klib/task_queue.hpp
  include/klib/task_status.hpp
  include/klib/task.hpp
  include/klib/unit_test.hpp
  include/klib/version.hpp
  include/klib/version_str.hpp
)

target_sources(${PROJECT_NAME} PRIVATE
  src/klib.cpp
)

get_target_property(sources ${PROJECT_NAME} SOURCES)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${sources})
configure_file(src/build_version.hpp.in "${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME}/build_version.hpp")
source_group("include/${PROJECT_NAME}" FILES "${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME}/build_version.hpp")

target_sources(${PROJECT_NAME} PUBLIC FILE_SET HEADERS BASE_DIRS "${CMAKE_CURRENT_BINARY_DIR}/include" FILES
  "${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME}/build_version.hpp"
)

add_library(${PROJECT_NAME}-test-main)
add_library(${PROJECT_NAME}::${PROJECT_NAME}-test-main ALIAS ${PROJECT_NAME}-test-main)

target_link_libraries(${PROJECT_NAME}-test-main PRIVATE
  ${PROJECT_NAME}::${PROJECT_NAME}-compile-options
)

target_link_libraries(${PROJECT_NAME}-test-main PUBLIC
  ${PROJECT_NAME}::${PROJECT_NAME}
)

target_sources(${PROJECT_NAME}-test-main PRIVATE
  src/klib_test_main.cpp
)
