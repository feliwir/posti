// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_TEXT_FONTFACE_H
#define _B2D_TEXT_FONTFACE_H

// [Dependencies]
#include "../core/any.h"
#include "../core/matrix2d.h"
#include "../core/membuffer.h"
#include "../core/string.h"
#include "../text/fontdata.h"
#include "../text/fontdiagnosticsinfo.h"
#include "../text/fontloader.h"
#include "../text/fontmatrix.h"
#include "../text/fontmetrics.h"
#include "../text/fontpanose.h"
#include "../text/fontunicodecoverage.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_text
//! \{

// ============================================================================
// [b2d::FontFaceFuncs]
// ============================================================================

//! `FontFace` functions assigned by OpenType implementations.
struct FontFaceFuncs {
  //! Character to glyph mapping implementation.
  typedef Error (B2D_CDECL* MapGlyphItemsFunc)(
    const GlyphEngine* self,
    GlyphItem* glyphItems,
    size_t count,
    GlyphMappingState* state);

  typedef Error (B2D_CDECL* GetGlyphBoundingBoxes)(
    const GlyphEngine* self,
    const GlyphId* glyphIds,
    size_t glyphIdAdvance,
    IntBox* boxes,
    size_t count);

  typedef Error (B2D_CDECL* GetGlyphAdvancesFunc)(
    const GlyphEngine* self,
    const GlyphItem* glyphItems,
    Point* glyphOffsets,
    size_t count);

  typedef Error (B2D_CDECL* PositionGlyphsFunc)(
    const GlyphEngine* self,
    GlyphItem* glyphItems,
    Point* glyphOffsets,
    size_t count);

  //! Glyph-outline decoder implementation.
  typedef Error (B2D_CDECL* DecodeGlyphFunc)(
    GlyphOutlineDecoder* self,
    uint32_t glyphId,
    const Matrix2D& userMatrix,
    Path2D& out,
    MemBuffer& tmpBuffer,
    GlyphOutlineSinkFunc sink, size_t sinkGlyphIndex, void* closure);

  //! Map glyph items implementation
  //!
  //! Depends on CMAP format and unicode variation sequences support.
  MapGlyphItemsFunc mapGlyphItems;

  //! Get bounding boxes of the given glyphs.
  //!
  //! Depends on implementation (TrueType uses 'glyf', OpenType uses 'CFF ').
  GetGlyphBoundingBoxes getGlyphBoundingBoxes;

  //! Get advances of the given glyphs.
  //!
  //! Advances are obtained through 'hmtx' or 'vmtx' tables.
  GetGlyphAdvancesFunc getGlyphAdvances;

  //! Apply pair adjustment.
  //!
  //! This would use either 'kern' or 'GPOS' table.
  PositionGlyphsFunc applyPairAdjustment;

  //! Decode glyph implementation.
  //!
  //! Depends on implementation (TrueType uses 'glyf', OpenType uses 'CFF ').
  DecodeGlyphFunc decodeGlyph;
};

// ============================================================================
// [b2d::FontFace - Impl]
// ============================================================================

//! \internal
class B2D_VIRTAPI FontFaceImpl : public ObjectImpl {
public:
  B2D_NONCOPYABLE(FontFaceImpl)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  B2D_API explicit FontFaceImpl(const FontLoader& loader, uint32_t faceIndex) noexcept;
  B2D_API virtual ~FontFaceImpl() noexcept;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  B2D_INLINE const FontLoader& loader() const noexcept { return _loader; }
  B2D_INLINE uint32_t implType() const noexcept { return _implType; }
  B2D_INLINE uint32_t faceFlags() const noexcept { return _faceFlags; }
  B2D_INLINE uint32_t faceIndex() const noexcept { return _faceIndex; }
  B2D_INLINE uint64_t faceUniqueId() const noexcept { return _faceUniqueId; }

  B2D_INLINE uint32_t glyphCount() const noexcept { return _glyphCount; }
  B2D_INLINE uint32_t unitsPerEm() const noexcept { return _unitsPerEm; }

  B2D_INLINE uint32_t weight() const noexcept { return _weight; }
  B2D_INLINE uint32_t stretch() const noexcept { return _stretch; }
  B2D_INLINE uint32_t style() const noexcept { return _style; }

  B2D_INLINE const char* fullName() const noexcept { return _fullName.data(); }
  B2D_INLINE const char* familyName() const noexcept { return _familyName.data(); }
  B2D_INLINE const char* subfamilyName() const noexcept { return _subfamilyName.data(); }
  B2D_INLINE const char* postScriptName() const noexcept { return _postScriptName.data(); }

  B2D_INLINE size_t fullNameSize() const noexcept { return _fullName.size(); }
  B2D_INLINE size_t familyNameSize() const noexcept { return _familyName.size(); }
  B2D_INLINE size_t subfamilyNameSize() const noexcept { return _subfamilyName.size(); }
  B2D_INLINE size_t postScriptNameSize() const noexcept { return _postScriptName.size(); }

  B2D_INLINE const FontDiagnosticsInfo& diagnosticsInfo() const noexcept { return _diagnosticsInfo; }
  B2D_INLINE const FontPanose& panose() const noexcept { return _panose; }
  B2D_INLINE const FontDesignMetrics& designMetrics() const noexcept { return _designMetrics; }

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  B2D_API virtual Error initFace(FontData& fontData) noexcept;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  FontLoader _loader;                    //!< Font loader of this `FontFaceImpl`.
  uint32_t _faceIndex;                   //!< Face index in TrueType/OpenType collection (or 0).
  uint32_t _faceFlags;                   //!< Face flags, see `FontFace::FaceFlags`.
  uint64_t _faceUniqueId;                //!< Unique face identifier assigned by Blend2D.

  uint8_t _implType;                     //!< Face implementation type, see `FontFace::ImplType`.
  uint8_t _reserved;
  uint16_t _glyphCount;                  //!< Number of glyphs provided by this font-face.
  uint16_t _unitsPerEm;                  //!< Design units per em.

  uint16_t _weight;                      //!< Face width (1..1000).
  uint8_t _stretch;                      //!< Face stretch (1..9)
  uint8_t _style;                        //!< Face style.

  Utf8String _fullName;                  //!< Full name.
  Utf8String _familyName;                //!< Family name.
  Utf8String _subfamilyName;             //!< Subfamily name.
  Utf8String _postScriptName;            //!< PostScript name.

  FontDiagnosticsInfo _diagnosticsInfo;  //!< Diagnostics information.
  FontPanose _panose;                    //!< Panose classification.
  FontUnicodeCoverage _unicodeCoverage;  //!< Unicode coverage (specified in OS/2 header).
  FontDesignMetrics _designMetrics;      //!< Font face metrics in design units.
  FontFaceFuncs _funcs;                  //!< Font face functions assigned by the implementation.
};

// ============================================================================
// [b2d::FontFace]
// ============================================================================

class FontFace : public Object {
public:
  enum : uint32_t {
    kTypeId = Any::kTypeIdFontFace
  };

  enum ImplType : uint32_t {
    kImplTypeNone         = 0,           //!< Face not initialized or unknown.
    kImplTypeTT           = 1,           //!< TrueType face with TT outlines (has 'glyf' and 'loca' tables).
    kImplTypeOT_CFFv1     = 2,           //!< OpenType face with CFF outlines (has 'CFF ' table).
    kImplTypeOT_CFFv2     = 3,           //!< OpenType face with CFF2 outlines (has 'CFF2' table).
    kImplTypeCount        = 4
  };

  enum Index : uint32_t {
    kIndexLimit           = 256,         //!< Maximum faces stored in a single collection Blend2D would use.
    kIndexCollection      = 0xFFFFFFFFu  //!< Index to the TrueType/OpenType collection instead of a face.
  };

  enum Flags : uint32_t {
    kFlagBaselineYEquals0   = 0x00000001u, //!< Baseline for font at `y` equals 0.
    kFlagLSBPointXEquals0   = 0x00000002u, //!< Left sidebearing point at `x` equals 0 (TT only).
    kFlagCharToGlyphMapping = 0x00010000u, //!< Character to glyph mapping is available.
    kFlagHorizontalMetrics  = 0x00020000u, //!< Horizontal layout data is available.
    kFlagVertData           = 0x00040000u, //!< Vertical layout data is available.
    kFlagLegacyKerning      = 0x00080000u, //!< Legacy kerning data is available ("kern" table).
    kFlagTypographicNames   = 0x00100000u, //!< Font uses typographic family and subfamily names.
    kFlagTypographicMetrics = 0x00200000u, //!< Font uses typographic metrics.
    kFlagPanose             = 0x00400000u, //!< Panose classification is available.
    kFlagUnicodeCoverage    = 0x00800000u, //!< Unicode coverage information is available.
    kFlagVariationSequences = 0x10000000u, //!< Unicode variation sequences feature is available.
    kFlagOpenTypeVariations = 0x20000000u, //!< OpenType Font Variations feature is available.
    kFlagSymbolFont         = 0x40000000u, //!< This is a symbol font.
    kFlagLastResortFont     = 0x80000000u  //!< This is a last resort font.
  };

  // --------------------------------------------------------------------------
  // [Contstruction / Destruction]
  // --------------------------------------------------------------------------

  B2D_INLINE FontFace() noexcept
    : Object(none().impl()) {}

  B2D_INLINE FontFace(const FontFace& other) noexcept
    : Object(other.impl()->addRef()) {}

  B2D_INLINE FontFace(FontFace&& other) noexcept
    : Object(other.impl()) { other._impl = none().impl(); }

  B2D_INLINE explicit FontFace(FontFaceImpl* impl) noexcept
    : Object(impl) {}

  static B2D_INLINE const FontFace& none() noexcept { return Any::noneOf<FontFace>(); }

  // --------------------------------------------------------------------------
  // [Create]
  // --------------------------------------------------------------------------

  //! Create a new `FontFace` from file specified by `fileName`.
  //!
  //! This is a utility function that first creates a `FontLoader` and then
  //! calls `createFromLoader(loader, 0)`.
  //!
  //! NOTE: This function offers a simplified creation of FontFace directly from
  //! a file, but doesn't provide as much flexibility as `createFromLoader()` as
  //! it allows to specify a `faceIndex`, which can be used to load multiple font
  //! faces from TrueType/OpenType collections. The use of `createFromLoader()`
  //! is recommended for any serious font handling.
  B2D_API Error createFromFile(const char* fileName) noexcept;

  //! Create a new `FontFace` from `FontLoader`.
  //!
  //! On success the existing `FontFace` is completely replaced by a new one,
  //! on failure an `Error` is returned and the existing `FontFace` is kept as
  //! is. In other words, it either succeeds and replaces a the `FontFaceImpl`
  //! or returns an error without touching the existing one.
  B2D_API Error createFromLoader(const FontLoader& loader, uint32_t faceIndex) noexcept;

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  B2D_INLINE Error reset() noexcept { return AnyInternal::replaceImpl(this, none().impl()); }

  // --------------------------------------------------------------------------
  // [Any Interface]
  // --------------------------------------------------------------------------

  template<typename T = FontFaceImpl>
  B2D_INLINE T* impl() const noexcept { return static_cast<T*>(_impl); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get face type, see `ImplType`.
  B2D_INLINE uint32_t implType() const noexcept { return impl()->implType(); }

  //! Get face flags, see `Flags`.
  B2D_INLINE uint32_t faceFlags() const noexcept { return impl()->faceFlags(); }

  //! Get the face index of this face.
  //!
  //! NOTE: Face index does only make sense if this face is part of a TrueType
  //! or OpenType font collection. In that case the returned value would be
  //! the index of this face in the collection. If the face is not part of a
  //! collection then the returned value would always be zero.
  B2D_INLINE uint32_t faceIndex() const noexcept { return impl()->faceIndex(); }

  //! Get a unique identifier describing this FontFace.
  B2D_INLINE uint64_t faceUniqueId() const noexcept { return impl()->faceUniqueId(); }

  //! Get a number of glyphs the face provides.
  B2D_INLINE uint32_t glyphCount() const noexcept { return impl()->glyphCount(); }

  //! Get design units per em.
  B2D_INLINE uint32_t unitsPerEm() const noexcept { return impl()->unitsPerEm(); }

  //! Get font weight (returns default weight in case this is a variable font).
  B2D_INLINE uint32_t weight() const noexcept { return impl()->weight(); }
  //! Get font stretch (returns default weight in case this is a variable font).
  B2D_INLINE uint32_t stretch() const noexcept { return impl()->stretch(); }
  //! Get font style.
  B2D_INLINE uint32_t style() const noexcept { return impl()->style(); }

  //! Get full name as UTF-8 null-terminated string.
  B2D_INLINE const char* fullName() const noexcept { return impl()->fullName(); }
  //! Get size of string returned by `fullName()`.
  B2D_INLINE size_t fullNameSize() const noexcept { return impl()->fullNameSize(); }

  //! Get family name as UTF-8 null-terminated string.
  B2D_INLINE const char* familyName() const noexcept { return impl()->familyName(); }
  //! Get size of string returned by `familyName()`.
  B2D_INLINE size_t familyNameSize() const noexcept { return impl()->familyNameSize(); }

  //! Get subfamily name as UTF-8 null-terminated string.
  B2D_INLINE const char* subfamilyName() const noexcept { return impl()->subfamilyName(); }
  //! Get size of string returned by `subfamilyName()`.
  B2D_INLINE size_t subfamilyNameSize() const noexcept { return impl()->subfamilyNameSize(); }

  //! Get manufacturer name as UTF-8 null-terminated string.
  B2D_INLINE const char* postScriptName() const noexcept { return impl()->postScriptName(); }
  //! Get size of string returned by `postScriptName()`.
  B2D_INLINE size_t postScriptNameSize() const noexcept { return impl()->postScriptNameSize(); }

  //! Get diagnostics information.
  B2D_INLINE const FontDiagnosticsInfo& diagnosticsInfo() const noexcept { return impl()->diagnosticsInfo(); }
  //! Get panose classification of this `FontFace`.
  B2D_INLINE const FontPanose& panose() const noexcept { return impl()->panose(); }
  //! Get design metrics of this `FontFace`.
  B2D_INLINE const FontDesignMetrics& designMetrics() const noexcept { return impl()->designMetrics(); }

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  //! Get `FontData` from `FontLoader` associated with this `FontFace`.
  B2D_API FontData fontData() const noexcept;

  //! Populate `out` array with all font tags this face contains.
  B2D_API Error listTags(Array<uint32_t>& out) noexcept;

  //! Calculate a new `Matrix2D` and `FontMetrics` for the given font parameters.
  B2D_API Error calcFontProperties(FontMatrix& matrixOut, FontMetrics& metricsOut, float size) const noexcept;

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  B2D_INLINE FontFace& operator=(const FontFace& other) noexcept {
    AnyInternal::assignImpl(this, other.impl());
    return *this;
  }

  B2D_INLINE FontFace& operator=(FontFace&& other) noexcept {
    FontFaceImpl* oldI = impl();
    _impl = other.impl();

    other._impl = none().impl();
    AnyInternal::releaseImpl(oldI);

    return *this;
  }
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_TEXT_FONTFACE_H
