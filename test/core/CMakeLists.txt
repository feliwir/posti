add_executable(core_test vm.cpp)
target_link_libraries(core_test pscore gtest_main)
include(GoogleTest)
gtest_discover_tests(core_test)