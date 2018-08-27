// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CORE_GLOBALS_H
#define _B2D_CORE_GLOBALS_H

// [Dependencies]
#include "../core/build.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::Globals]
// ============================================================================

namespace Globals {

// ============================================================================
// [b2d::Globals::<global>]
// ============================================================================

//! Host memory allocator overhead.
constexpr uint32_t kMemAllocOverhead = uint32_t(sizeof(void*)) * 4;

//! Host memory allocator alignment.
//!
//! Specifies minimum alignment of memory allocated by global memory allocator.
constexpr uint32_t kMemAllocAlignment = 8;

// TODO: Document this
constexpr uint32_t kMemGranularity = 64;

//! how much memory to reserve for stack based temporary objects (per object).
constexpr size_t kMemTmpSize = 1024;

//! Convenience zero, used when flags are expected, but not flags are given.
constexpr uint32_t kNoFlags = 0;

//! Invalid index, equals to `(size_t)-1`.
constexpr size_t kInvalidIndex = std::numeric_limits<size_t>::max();

//! The size of the string is not known, but the string is null terminated.
constexpr size_t kNullTerminated = std::numeric_limits<size_t>::max();

// ============================================================================
// [b2d::Globals::ArchId]
// ============================================================================

//! CPU architecture ID.
enum ArchId : uint32_t {
  kArchNone = 0,   //!< None/Unknown CPU architecture.
  kArchX86  = 1,   //!< X86 architecture.
  kArchX64  = 2,   //!< X64 architecture, also called AMD64.
  kArchArm  = 4,   //!< Arm architecture.

  kArchHost = (B2D_ARCH_X86 == 32) ? kArchX86 :
              (B2D_ARCH_X86 == 64) ? kArchX64 :
              (B2D_ARCH_ARM == 32) ? kArchArm : kArchNone
};

// ============================================================================
// [b2d::Globals::ByteOrder]
// ============================================================================

//! Byte order.
enum ByteOrder : uint32_t {
  kByteOrderLE      = 0,
  kByteOrderBE      = 1,
  kByteOrderNative  = B2D_ARCH_LE ? kByteOrderLE : kByteOrderBE,
  kByteOrderSwapped = B2D_ARCH_LE ? kByteOrderBE : kByteOrderLE
};

// ============================================================================
// [b2d::Globals::ResetPolicy]
// ============================================================================

//! Reset policy used by most `reset()` functions.
enum ResetPolicy : uint32_t {
  //! Soft reset, doesn't deallocate memory (default).
  kResetSoft = 0,
  //! Hard reset, releases all memory used, if any.
  kResetHard = 1
};

// ============================================================================
// [b2d::Globals::Link]
// ============================================================================

enum Link : uint32_t {
  kLinkLeft  = 0,
  kLinkRight = 1,

  kLinkPrev  = 0,
  kLinkNext  = 1,

  kLinkFirst = 0,
  kLinkLast  = 1,

  kLinkCount = 2
};

} // Globals namespace

// ============================================================================
// [b2d::Error]
// ============================================================================

//! Error type (32-bit unsigned integer).
typedef uint32_t Error;

//! Error code.
enum ErrorCode : uint32_t {
  kErrorOk = 0,

  // --------------------------------------------------------------------------
  // [Common]
  // --------------------------------------------------------------------------

  //! \internal
  //! \{
  _kErrorCommonStart = 0x00010000,
  _kErrorCommonEnd = 0x0001FFFF,
  //! \}

  kErrorNoMemory = _kErrorCommonStart,
  kErrorInvalidArgument,                 //!< Invalid argument (EINVAL).
  kErrorInvalidState,
  kErrorInvalidHandle,

  kErrorUnsupportedOperation,
  kErrorUnsupportedErrorCode,

  kErrorNotFound,
  kErrorNotImplemented,

  kErrorResourceBusy,
  kErrorResourceMismatch,

  kErrorStringNoSpaceLeft,
  kErrorStringTooLarge,
  kErrorInvalidString,
  kErrorTruncatedString,

  kErrorIOGeneric,                       //!< Generic IO error (EIO, possibly more errors).
  kErrorIOPipe,                          //!< Pipe error.
  kErrorIOOpenFailure,                   //!< File open failure (Windows).
  kErrorIOTooManyFiles,
  kErrorIOTooManyLinks,                  //!< Too many symbolic links on file-system.
  kErrorIOSymlinkLoop,                   //!< Too many levels of symbolic links (ELOOP).
  kErrorIOFileTooLarge,                  //!< File is too large (EFBIG).
  kErrorIOAlreadyExists,                 //!< File or directory already exists (EEXIST).
  kErrorIOAccessDenied,                  //!< Access denied (EACCES).
  kErrorIOInterrupted,                   //!< Interrupted IO error (EINTR).
  kErrorIOInvalidSeek,                   //!< File is not seekable (ESPIPE).
  kErrorIOFileEmpty,                     //!< File is empty.
  kErrorIOEndOfFile,                     //!< End of file (ENODATA).
  kErrorIONameInvalid,                   //!< File name, directory name, or path name is invalid.
  kErrorIONameTooLong,                   //!< File name, directory name, or path name too long (ENAMETOOLONG).
  kErrorIOMediaChanged,                  //!< Media changed (Windows).
  kErrorIOReadOnly,                      //!< The file or file-system is read-only (EROFS)
  kErrorIORWFailure,                     //!< Read/write failure mostly because of bad media/disk.
  kErrorIONoDevice,                      //!< Device doesn't exist (ENXIO).
  kErrorIONoEntry,                       //!< No such file or directory (no-entry) (ENOENT).
  kErrorIONoMedia,                       //!< No media in drive/device (ENOMEDIUM).
  kErrorIONoMoreFiles,                   //!< No more files (ENMFILE).
  kErrorIONoSpaceLeft,                   //!< No space left on device (ENOSPC).
  kErrorIONotEmpty,                      //!< Directory is not empty (ENOTEMPTY).
  kErrorIONotFile,                       //!< Not a file (EISDIR).
  kErrorIONotDirectory,                  //!< Not a directory (ENOTDIR).
  kErrorIONotRootDirectory,              //!< Not a root directory (Windows).
  kErrorIONotSameDevice,                 //!< Not same device (EXDEV).
  kErrorIONotBlockDevice,                //!< Not a block device (ENOTBLK).
  kErrorIONotPermitted,                  //!< Operation not permitted (EPERM).
  kErrorIONotAvailable,                  //!< Data not available (EAGAIN).
  kErrorIOOutOfRange,                    //!< Out of range error (EOVERFLOW, EINVAL).
  kErrorIOOutOfHandles,                  //!< Out of file handles available (ENFILE, EMFILE).

  // --------------------------------------------------------------------------
  // [Blend2D]
  // --------------------------------------------------------------------------

  //! Start of namespace used by Blend.
  _kErrorB2DStart = 0x00020000,
  //! End of namespace used by Blend.
  _kErrorB2DEnd = 0x000200FF,

  //! Invalid header.
  kErrorDeflateInvalidHeader,
  //! Invalid Huffman table specified in a DEFLATE stream.
  kErrorDeflateInvalidTable,
  //! Invalid data encountered in a DEFLATE stream.
  kErrorDeflateInvalidData,

  //! No geometry.
  //!
  //! This may happen when some method is called by passing an empty object. If
  //! this error is returned the output of the function called shouldn't be used
  //! nor considered as correct. For example the bounding-box of shape which is
  //! invalid is defined to be [0, 0, 0, 0], but if this bounding box is used to
  //! extend an existing one the result will be incorrect.
  kErrorNoGeometry = _kErrorB2DStart,

  //! Invalid geometry.
  //!
  //! This may happen if `Path2D` contains invalid data (although this might
  //! be considered as run-time error) or if some basic shape is incorrectly
  //! given (for example if `Rect` width or height is negative).
  kErrorInvalidGeometry,

  //! Can't stroke the path or shape.
  kErrorGeometryUnstrokeable,

  //! Invalid (degenerate) transformation matrix.
  //!
  //! The degenerated transform can't be used in geometry, because the result
  //! is simply nothing - for example rectangle might degenerate to rectangle
  //! with zero width or height which can't be rendered.
  //!
  //! NOTE: This error is always related directly or indirectly to `Matrix2D`.
  kErrorInvalidMatrix,

  //! It is required that a previous path command is a valid point (i.e. not
  //! a close command).
  //!
  //! To add `Path2D::kCmdLineTo`, `Path2D::kCmdQuadTo`, `Path2D::kCmdCubicTo`
  //! or `Path2D::kCmdClose` the previous command must be a valid data point.
  //! The only command that do not require such condition is `Path2D::kCmdMoveTo`.
  kErrorPathNoVertex,

  //! The relative command can't be added, because the previous command
  //! is not a vertex.
  kErrorPathNoRelative,

  //! Pixel format is invalid.
  kErrorInvalidPixelFormat,

  //! Image type is invalid.
  kErrorInvalidImageType,
  //! Image size is invalid.
  kErrorInvalidImageSize,

  //! Font, FontFace, orFontLoader is not initialized (it points to a built-in none instance).
  kErrorFontNotInitialized,
  //! Invalid font signature (broken font or not TrueType/OpenType font at all).
  kErrorFontInvalidSignature,
  //! Invalid font data - happens when `FontData` failed to validate a font data.
  kErrorFontInvalidData,
  //! Invalid font face - happens when `FontFace` failed to validate required tables.
  kErrorFontInvalidFace,
  //! Invalid font face index - `faceIndex` is greater than the number of faces provided by TrueType/OpenType collection.
  kErrorFontInvalidFaceIndex,
  //! Invalid GlyphId.
  kErrorFontInvalidGlyphId,
  //! Font has an invalid CMAP table that prevents Blend2D to use it.
  kErrorFontInvalidCMap,
  //! Font has a missing CMAP and character to glyph mapping is not possible (embedded fonts without CMAP exist).
  kErrorFontMissingCMap,
  //! Font has a CMAP table, but Blend2D doesn't provide any implementation for its platformId, encodingId, or format.
  kErrorFontUnsupportedCMap,
  //! Font has an invalid CFF data.
  kErrorFontInvalidCFFData,
  //! Font program (CFF-CharString) was terminated because the execution reached instruction limit.
  kErrorFontProgramTerminated,

  //! Codec is not yet initializer of it doesn't have a buffer.
  kErrorCodecNotInitialized,
  //! Codec for a required format doesn't exist.
  kErrorCodecNotFound,
  //! Codec doesn't support decoding.
  kErrorCodecNoDecoder,
  //! Codec doesn't support encoding.
  kErrorCodecNoEncoder,
  //! Invalid signature.
  kErrorCodecInvalidSignature,
  //! Invalid or unrecognized format.
  kErrorCodecInvalidFormat,
  //! Invalid size - zero or negative width and/or height.
  kErrorCodecInvalidSize,
  //! The size of the image is too large.
  kErrorCodecUnsupportedSize,
  //! Invalid RLE.
  kErrorCodecInvalidRLE,
  //! Invalid Huffman stream.
  kErrorCodecInvalidHuffman,
  //! The input buffer is incomplete.
  kErrorCodecIncompleteData,
  //! Decode called multiple times or the animation ended.
  kErrorCodecNoMoreFrames,

  //! Invalid data.
  //!
  //! NOTE: This is a generic error that is reported in case that the codec
  //! doesn't have a specific error code for the failure.
  kErrorCodecInvalidData,

  //! Invalid image offset (BMP).
  kErrorBmpInvalidImageOffset,
  //! Invalid image size (BMP).
  kErrorBmpInvalidImageSize,

  //! Multiple IHDR chunks are not allowed (PNG).
  kErrorPngDuplicatedIHDR,
  //! Invalid IDAT chunk (PNG).
  kErrorPngInvalidIDAT,
  //! Invalid IEND chunk (PNG).
  kErrorPngInvalidIEND,
  //! Invalid PLTE chunk (PNG).
  kErrorPngInvalidPLTE,
  //! Invalid tRNS chunk (PNG).
  kErrorPngInvalidTRNS,
  //! Invalid filter type (PNG).
  //!
  //! Can happen during a decode phase. Filter type is specified in a PNG stream
  //! for each scanline. The number of filter types is limited to 5, so any value
  //! greater than 4 causes this error to be returned and decoding terminated.
  kErrorPngInvalidFilter,

  //! Unsupported feature (JPEG).
  kErrorJpegUnsupportedFeature,
  //! Invalid SOF marker (JPEG).
  kErrorJpegInvalidSOFLength,
  //! Multiple SOF markers found (JPEG).
  kErrorJpegDuplicatedSOFMarker,
  //! Unsupported SOF marker (JPEG).
  kErrorJpegUnsupportedSOFMarker,
  //! Invalid component stored in the SOS marker (JPEG).
  kErrorJpegInvalidSOSComponent,
  //! Dumplicated component stored in the SOS marker (JPEG).
  kErrorJpegDuplicatedSOSComponent,
  //! Invalid number of component stored in the SOS marker (JPEG).
  kErrorJpegInvalidSOSComponentCount,
  //! Invalid AC selector stored in SOS marker (JPEG).
  kErrorJpegInvalidACSelector,
  //! Invalid DC selector stored in SOS marker (JPEG).
  kErrorJpegInvalidDCSelector,

  //! JPEG/JFIF marker already processed/defined.
  kErrorJFIFAlreadyDefined,
  //! JPEG/JFIF marker has an invalid size.
  kErrorJFIFInvalidSize,

  //! JPEG/JFXX marker already processed/defined.
  kErrorJFXXAlreadyDefined,
  //! JPEG/JFXX marker has an invalid size.
  kErrorJFXXInvalidSize,
  //! JPEG/JFXX marker has an invalid (thumbnail) format.
  kErrorJFXXInvalidThumbnailFormat
};

// ============================================================================
// [B2D_PROPAGATE]
// ============================================================================

#define B2D_PROPAGATE(...)                                                    \
  B2D_MACRO_BEGIN                                                             \
    ::b2d::Error errorToPropagate = (__VA_ARGS__);                            \
    if (B2D_UNLIKELY(errorToPropagate))                                       \
      return errorToPropagate;                                                \
  B2D_MACRO_END

// ============================================================================
// [b2d::DebugUtils]
// ============================================================================

namespace DebugUtils {

//! Returns the error `err` passed.
//!
//! Provided for debugging purposes. Putting a breakpoint inside `errored` can
//! help with tracing the origin of any error reported / returned by Blend2D.
static inline Error errored(Error err) noexcept { return err; }

//! Prints a message to either standard stream or debug output.
B2D_API void debugOutput(const char* msg) noexcept;

//! Similar to `debugOutput`, designed to behave like `printf()`.
B2D_API void debugFormat(const char* fmt, ...) noexcept;

//! Called upon assertion failure.
B2D_API void B2D_NORETURN assertionFailed(const char* file, int line, const char* msg) noexcept;

} // DebugUtils namespace

// ============================================================================
// [B2D_ASSERT]
// ============================================================================

//! \def B2D_ASSERT(EXP)
//!
//! Run-time assertion used when debugging.

//! \def B2D_NOT_REACHED()
//!
//! Run-time assertion used in code that should be never reached.

#ifdef B2D_BUILD_DEBUG
  #define B2D_ASSERT(EXP)                                                     \
    B2D_MACRO_BEGIN                                                           \
      if (!(EXP))                                                             \
        ::b2d::DebugUtils::assertionFailed(__FILE__, __LINE__, #EXP);         \
    B2D_MACRO_END
  #define B2D_NOT_REACHED()                                                   \
    B2D_MACRO_BEGIN                                                           \
      ::b2d::DebugUtils::assertionFailed(__FILE__, __LINE__, "NOT REACHED");  \
    B2D_MACRO_END
#else
  #define B2D_ASSERT(EXP) ((void)0)
  #define B2D_NOT_REACHED() B2D_ASSUME(0)
#endif

//! \}

// ============================================================================
// [b2d::Init / NoInit]
// ============================================================================

struct NoInit_ {};
constexpr NoInit_ NoInit {};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CORE_GLOBALS_H
