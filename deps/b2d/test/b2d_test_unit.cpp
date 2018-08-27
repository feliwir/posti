// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Dependencies]
#include "b2d/b2d.h"

// ============================================================================
// [Main]
// ============================================================================

int main(int argc, const char* argv[]) {
  const b2d::Runtime::BuildInfo& buildInfo = b2d::Runtime::buildInfo();

  INFO("Blend2D Unit-Test (v%u.%u.%u - %s)\n\n",
    buildInfo.majorVersion(),
    buildInfo.minorVersion(),
    buildInfo.patchVersion(),
    buildInfo.isReleaseBuild() ? "release" : "debug");

  return BrokenAPI::run(argc, argv);
}
