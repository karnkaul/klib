file(GLOB_RECURSE sources LIST_DIRECTORIES false "test_*.cpp")

if(NOT "${sources}" STREQUAL "")
  add_executable(${PROJECT_NAME}-tests)
  target_link_libraries(${PROJECT_NAME}-tests PRIVATE
    ${PROJECT_NAME}::${PROJECT_NAME}-test-main
    ${PROJECT_NAME}::${PROJECT_NAME}
  )
  target_include_directories(${PROJECT_NAME}-tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../lib/src"
    "${CMAKE_CURRENT_SOURCE_DIR}"
  )

  # target_sources(${PROJECT_NAME}-tests PRIVATE ${sources})
  target_sources(${PROJECT_NAME}-tests PRIVATE
    test_assert.cpp
    test_demangle.cpp
    test_enum_name.cpp
    test_enum_ops.cpp
    test_fixed_string.cpp
    test_from_chars.cpp
    test_lerp_expr.cpp
    test_log.cpp
    test_ptr.cpp
    test_unique.cpp
    test_version.cpp
  )
  add_test(NAME ${PROJECT_NAME}-tests COMMAND ${PROJECT_NAME}-tests)
endif()

# add_executable(${PROJECT_NAME}-xplat-main-test WIN32 xplat_main_test.cpp)
# klib_set_mainCRTStartup(${PROJECT_NAME}-xplat-main-test)
