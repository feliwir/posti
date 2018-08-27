// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_TEXT_OTXMTX_P_H
#define _B2D_TEXT_OTXMTX_P_H

// [Dependencies]
#include "../text/otglobals_p.h"

B2D_BEGIN_SUB_NAMESPACE(OT)

//! \addtogroup b2d_text_ot
//! \{

// ============================================================================
// [b2d::OT::XHeaTable]
// ============================================================================

#pragma pack(push, 1)
//! \internal
//!
//! Base table used by `HHeaTable` and `VHeaTable`
struct XHeaTable {
  enum : uint32_t {
    kMinSize = 36
  };

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  OTFixed version;                       //!< Version number.
  OTInt16 ascender;                      //!< Typographic ascent.
  OTInt16 descender;                     //!< Typographic descent.
  OTInt16 lineGap;                       //!< Typographic line-gap.
  OTUInt16 maxAdvance;                   //!< Maximum advance width/height value in 'htmx|vmtx' table.
  OTInt16 minLeadingBearing;             //!< Minimum left/top sidebearing value in 'htmx|vmtx' table.
  OTInt16 minTrailingBearing;            //!< Minimum right/bottom sidebearing value in 'htmx|vmtx' table.
  OTInt16 maxExtent;                     //!< Maximum x/y extent `max(lsb + ([x/y]Max - [x/y]Min))`.
  OTInt16 caretSlopeRise;                //!< Used to calculate the slope of the cursor (rise).
  OTInt16 caretSlopeRun;                 //!< Used to calculate the slope of the cursor (run).
  OTInt16 caretOffset;                   //!< Shift of slanted highlight on a glyph to produce the best appearance (0 for non-slanted fonts).
  OTInt16 reserved[4];                   //!< Reserved, should be zero.
  OTUInt16 longMetricFormat;             //!< Metric data format (should be zero).
  OTUInt16 longMetricCount;              //!< Number of LongMetric entries in a corresponding ('hmtx' or 'vmtx') table.
};
#pragma pack(pop)

// ============================================================================
// [b2d::OT::HHeaTable]
// ============================================================================

#pragma pack(push, 1)
//! \internal
//!
//! Horizontal Header Table 'hhea'.
//!
//! External Resources:
//!   - https://docs.microsoft.com/en-us/typography/opentype/spec/hhea
//!   - https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6hhea.html
struct HHeaTable : public XHeaTable {
  enum : uint32_t {
    kTag = FontTag::hhea
  };
};
#pragma pack(pop)

// ============================================================================
// [b2d::OT::VHeaTable]
// ============================================================================

#pragma pack(push, 1)
//! \internal
//!
//! Vertical Header Table 'vhea'.
//!
//! External Resources:
//!   - https://docs.microsoft.com/en-us/typography/opentype/spec/vhea
//!   - https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6vhea.html
struct VHeaTable : public XHeaTable {
  enum : uint32_t {
    kTag = FontTag::vhea
  };
};
#pragma pack(pop)

// ============================================================================
// [b2d::OT::XMtxTable]
// ============================================================================

#pragma pack(push, 1)
//! \internal
//!
//! Base table used by `HMtxTable` and `VMtxTable`
struct XMtxTable {
  enum : uint32_t {
    kMinSize = 4 // At least one LongMetric.
  };

  struct LongMetric {
    OTUInt16 advance;                    //!< Advance width/height, in font design units.
    OTInt16 lsb;                         //!< Leading side bearing, in font design units.
  };

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Paired advance width and left side bearing values, indexed by glyph ID.
  B2D_INLINE const LongMetric* lmArray() const noexcept { return fieldAt<LongMetric>(this, 0); }
  //! Leading side bearings for glyph IDs greater than or equal to `metricCount`.
  B2D_INLINE const OTInt16* lsbArray(size_t metricCount) const noexcept { return fieldAt<OTInt16>(this, metricCount * sizeof(LongMetric)); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  /*
  LongMetric lmArray[];
  OTInt16 lsbArray[];
  */
};
#pragma pack(pop)

// ============================================================================
// [b2d::OT::HMtxTable]
// ============================================================================

#pragma pack(push, 1)
//! \internal
//!
//! Horizontal Metrics Table 'hmtx'.
//!
//! External Resources:
//!   - https://docs.microsoft.com/en-us/typography/opentype/spec/hmtx
//!   - https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6hmtx.html
struct HMtxTable : public XMtxTable {
  enum : uint32_t {
    kTag = FontTag::hmtx
  };
};
#pragma pack(pop)

// ============================================================================
// [b2d::OT::VMtxTable]
// ============================================================================

#pragma pack(push, 1)
//! \internal
//!
//! Vertical Metrics Table 'vmtx'.
//!
//! External Resources:
//!   - https://docs.microsoft.com/en-us/typography/opentype/spec/vmtx
//!   - https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6vmtx.html
struct VMtxTable : public XMtxTable {
  enum : uint32_t {
    kTag = FontTag::vmtx
  };
};
#pragma pack(pop)

// ============================================================================
// [b2d::OT::MetricsData]
// ============================================================================

//! \internal
//!
//! Data stored in OTFace that is related to font/glyph metrics (horizontal and vertical).
struct MetricsData {
  uint16_t _longMetricCount[2];          //!< Count of 'LongMetric' entries in "hmtx" and "vmtx" tables.
  uint16_t _lsbArraySize[2];             //!< Count of entries of `lsbArray` in "hmtx" and "vmtx" table (after LongMetric data).
};

// ============================================================================
// [b2d::OT::MetricsImpl]
// ============================================================================

namespace MetricsImpl {
  Error initMetrics(OTFaceImpl* faceI, FontData& fontData) noexcept;
}

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_TEXT_OTXMTX_P_H
