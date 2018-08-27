// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_FILL_H
#define _B2D_CORE_FILL_H

// [Dependencies]
#include "../core/globals.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::FillRule]
// ============================================================================

namespace FillRule {
  //! Fill rule.
  enum Type : uint32_t {
    kNonZero          = 0,               //!< Fill using non-zero rule.
    kEvenOdd          = 1,               //!< Fill using even-odd rule.

    kCount            = 2,               //!< Count of fill rules, used to catch invalid arguments.
    kDefault          = kNonZero,        //!< Default fill rule (the actual value should be zero).
    _kPickAny         = kEvenOdd         //!< Synonym to `kRuleEvenOdd`, used in places where fill-rule doesn't matter.
  };
}

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_FILL_H
