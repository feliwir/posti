// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_CODEC_DEFLATE_P_H
#define _B2D_CODEC_DEFLATE_P_H

// [Dependencies]
#include "../core/buffer.h"

B2D_BEGIN_NAMESPACE

//! \addtogroup b2d_core
//! \{

// ============================================================================
// [b2d::Deflate]
// ============================================================================

struct Deflate {
  //! Callback that is used to read a chunk of data to be consumed by the
  //! decoder. It was introduced for PNG support, which can divide the data
  //! stream into multiple `"IDAT"` chunks, thus the stream is not continuous.
  //!
  //! The logic has been simplified in a way that `ReadFunc` reads the first
  //! and all consecutive chunks. There is no other way to be consumed by the
  //! decoder.
  typedef bool (B2D_CDECL* ReadFunc)(void* readCtx, const uint8_t** pData, const uint8_t** pEnd);

  //! Deflate data retrieved by `ReadFunc` into `dst` buffer.
  static B2D_API Error deflate(Buffer& dst, void* readCtx, ReadFunc readFunc, bool hasHeader) noexcept;
};

//! \}

B2D_END_NAMESPACE

// [Guard]
#endif // _B2D_CODEC_DEFLATE_P_H
