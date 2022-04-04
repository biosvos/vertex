find_package(doctest REQUIRED)

add_executable(test test.cpp)
target_link_libraries(test ${PROJECT_NAME} doctest::doctest)
target_compile_definitions(test PRIVATE DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN)