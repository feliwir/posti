// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_TEXT_FONTDIAGNOSTICSINFO_H
#define _B2D_TEXT_FONTDIAGNOSTICSINFO_H

// [Dependencies]
#include "../core/any.h"
#include "../core/array.h"
#include "../core/support.h"
#include "../text/fonttag.h"
#include "../text/textglobals.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_text
//! \{

// ============================================================================
// [b2d::FontDiagnosticsInfo]
// ============================================================================

//! Font diagnostics info contains some information about problematic parts of
//! fonts. When Blend2D loads a particular font it would fill this information
//! for further diagnostics and/or statistics.
//!
//! You can use this data to make sure that the font you are using has not some
//! hidden defects that Blend2D cannot workaround. In many cases Blend2D can
//! use such fonts just fine and would fix small problems (for example unsorted
//! kerning pairs, invalid 'name' table entries, etc), but there are of course
//! cases where Blend2D would refuse to use such font.
struct FontDiagnosticsInfo {
  enum Flags : uint32_t {
    kFlagInvalidNameData        = 0x00000001u, //!< Invalid data in 'name' table (invalid strings).
    kFlagRedundantSubfamilyName = 0x00000002u, //!< Redundant subfamily name has been removed.

    kFlagInvalidCMapData        = 0x00000004u, //!< Invalid data in 'cmap' table.
    kFlagInvalidCMapFormat      = 0x00000008u, //!< Invalid format of 'cmap' (sub)table.

    kFlagInvalidKernData        = 0x00000010u, //!< Invalid data in 'kern' table.
    kFlagInvalidKernFormat      = 0x00000020u, //!< Kerning group is in an unknown format.
    kFlagTruncatedKernData      = 0x00000040u, //!< Kerning pairs in 'kern' table are truncated.
    kFlagUnsortedKernPairs      = 0x00000080u, //!< Kerning pairs in 'kern' table aren't sorted.

    kFlagInvalidGDefData        = 0x00000100u, //!< Invalid data in 'GDEF' table.
    kFlagInvalidGPosData        = 0x00000200u, //!< Invalid data in 'GPOS' table.
    kFlagInvalidGSubData        = 0x00000400u  //!< Invalid data in 'GSUB' table.
  };

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  B2D_INLINE void reset() noexcept {
    _flags = 0;
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE uint32_t flags() const noexcept { return _flags; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _flags;
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_TEXT_FONTDIAGNOSTICSINFO_H
