add_library(pscore STATIC
    builtins.cpp builtins.hpp
    graphicsstate.hpp
    interpreter.cpp interpreter.hpp
    object.hpp
    objects/integer.hpp
    objects/name.hpp
    objects/operand.hpp
    objects/real.hpp
    objects/string.hpp
    parser.cpp parser.hpp
    renderer.cpp renderer.hpp
    util.hpp)

target_link_libraries(pscore Blend2D::Blend2D)
set(generated_headers "${CMAKE_CURRENT_BINARY_DIR}/generated_headers")
set(pscore_export "${generated_headers}/pscore_export.hpp")
include(GenerateExportHeader)
generate_export_header(pscore EXPORT_FILE_NAME ${pscore_export})

# Global includes. Used by all targets
# Note:
#   * header location in project: Foo/Source/foo/Bar.hpp
#   * header can be included by C++ code `#include <foo/Bar.hpp>`
#   * header location in project: ${CMAKE_CURRENT_BINARY_DIR}/generated_headers/foo/BAR_EXPORT.h
#   * header can be included by: `#include <BAR_EXPORT.h>`
target_include_directories(
    pscore PUBLIC
    "$<BUILD_INTERFACE:${generated_headers}>"
    ${CMAKE_CURRENT_SOURCE_DIR}
)