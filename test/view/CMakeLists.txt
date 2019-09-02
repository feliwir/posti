# Test specific files
add_test(NAME path_test COMMAND psview ${CMAKE_CURRENT_SOURCE_DIR}/files/path.ps)
add_test(NAME arithmetic_test COMMAND psview ${CMAKE_CURRENT_SOURCE_DIR}/files/arithmetic.ps)