// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_TEXT_OTFACE_P_H
#define _B2D_TEXT_OTFACE_P_H

// [Dependencies]
#include "../text/fontdata.h"
#include "../text/fontface.h"
#include "../text/otglobals_p.h"
#include "../text/otcff_p.h"
#include "../text/otcmap_p.h"
#include "../text/otkern_p.h"
#include "../text/otlayout_p.h"
#include "../text/otmetrics_p.h"
#include "../text/otname_p.h"

B2D_BEGIN_SUB_NAMESPACE(OT)

//! \addtogroup b2d_text_ot
//! \{

// ============================================================================
// [b2d::OT::OTFaceImpl]
// ============================================================================

//! \internal
//!
//! TrueType or OpenType font face.
//!
//! This class provides extra data required by TrueType / OpenType implementation.
//! It's currently the only implementation of `FontFaceImpl` available in Blend2D
//! and there will probably not be any other implementation as OpenType provides
//! a lot of features required to render text in general.
class OTFaceImpl : public FontFaceImpl {
public:
  B2D_NONCOPYABLE(OTFaceImpl)

  OTFaceImpl(const FontLoader& loader, uint32_t faceIndex) noexcept;
  virtual ~OTFaceImpl() noexcept;

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  //! Called by FontFaceImpl after it the face has been created to initialize the
  //! implementation and the content of `OT::Face`. If it fails the `FontFaceImpl`
  //! will be destroyed and error propagated up to the caller.
  Error initFace(FontData& fontData) noexcept override;

  B2D_INLINE Error bailWithDiagnostics(uint32_t diagflags) noexcept {
    _diagnosticsInfo._flags |= diagflags;
    return kErrorOk;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint8_t _locaOffsetSize;               //!< Size of offsets stored in "loca" table, or zero.

  CMapData _cMapData;                    //!< Character to glyph mapping data - 'cmap' table.
  MetricsData _metricsData;              //!< Metrics data - 'hhea', 'vhea', 'hmtx', and 'vmtx' tables.
  LayoutData _layoutData;                //!< OpenType layout data - 'GDEF', 'GSUB', and 'GPOS' tables.
  KernData _kernData[2];                 //!< Legacy kerning data - 'kern' table.
  CFFInfo _cffInfo;                      //!< CFF/CFF2 data (used by CFF implementation of `GlyphOutlineDecoder`).
};

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_TEXT_OTFACE_P_H
