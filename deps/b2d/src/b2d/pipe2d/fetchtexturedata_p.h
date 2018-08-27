// [B2dPipe]
// 2D Pipeline Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_PIPE2D_FETCHTEXTUREDATA_P_H
#define _B2D_PIPE2D_FETCHTEXTUREDATA_P_H

// [Dependencies]
#include "./pipeglobals_p.h"

B2D_BEGIN_SUB_NAMESPACE(Pipe2D)

//! \addtogroup b2d_pipe2d
//! \{

// ============================================================================
// [b2d::Pipe2D::FetchTextureData]
// ============================================================================

//! Texture fetch data.
struct FetchTextureData {
  //! Extend mode specific for texture fetcher, always per direction [X|Y].
  enum Extend {
    kExtendPad     = 0,
    kExtendRepeat  = 1,
    kExtendRoR     = 2
  };

  // --------------------------------------------------------------------------
  // [Init - Any]
  // --------------------------------------------------------------------------

  inline void initSource(const uint8_t* pixelData, intptr_t stride, uint32_t width, uint32_t height) noexcept {
    // It's safe to const_cast<> it here as texture-fetch won't change the texture itself.
    src.pixels = const_cast<uint8_t*>(pixelData);
    src.stride = stride;
    src.width  = width;
    src.height = height;
  }

  // --------------------------------------------------------------------------
  // [Init - BlitAA]
  // --------------------------------------------------------------------------

  inline uint32_t initBlitAA() noexcept {
    simple.tx = 0;
    simple.ty = 0;
    simple.rx = 0;
    simple.ry = 0;
    return kFetchTypeTextureAABlit;
  }

  // --------------------------------------------------------------------------
  // [Init - Translate]
  // --------------------------------------------------------------------------

  inline uint32_t _initTranslateAny(uint32_t fetchBase, uint32_t extend, int tx, int ty, bool isFractional) noexcept {
    uint32_t extendX = Pattern::extendXFromExtend(extend);
    uint32_t extendY = Pattern::extendYFromExtend(extend);
    uint32_t ixIndex = 17;

    int rx = 0;
    int ry = 0;

    // If the texture width/heigh is 1 all extend modes produce the same output.
    // However, it's safer to just set it to PAD as FetchTexturePart requires
    // `width` to be equal or greater than 2 if the extend mode is REPEAT or
    // REFLECT.
    if (src.width  <= 1) extendX = Pattern::kExtendPad;
    if (src.height <= 1) extendY = Pattern::kExtendPad;

    if (extendX >= Pattern::kExtendRepeat) {
      bool isReflecting = extendX == Pattern::kExtendReflect;

      rx = int(src.width) << uint32_t(isReflecting);
      if (unsigned(tx) >= unsigned(rx))
        tx %= rx;
      if (tx < 0)
        tx += rx;

      // In extreme cases, when `rx` is very small, fetch4()/fetch8() functions
      // may overflow `x` if they increment more than they can fix by subtracting
      // `rw` in case of overflow (and overflow happens as it's used to start
      // over). To fix this and simplify the compiled code we simply precalculate
      // these constants so they are always safe.
      ixIndex = Math::min<uint32_t>(uint32_t(rx), 17);

      // Don't specialize `Repeat vs Reflect` when we are not pixel aligned.
      if (isFractional)
        extendX = 1; // TODO: Naming...
    }

    if (extendY >= Pattern::kExtendRepeat) {
      ry = int(src.height) << uint32_t(extendY == Pattern::kExtendReflect);
      if (unsigned(ty) >= unsigned(ry))
        ty %= ry;
      if (ty < 0)
        ty += ry;
    }

    simple.tx = tx;
    simple.ty = ty;
    simple.rx = rx;
    simple.ry = ry;
    simple.ix = gModTable[ixIndex];

    return fetchBase + extendX;
  }

  inline uint32_t initTranslateAA(uint32_t extend, int x, int y) noexcept {
    return _initTranslateAny(kFetchTypeTextureAAPadX, extend, -x, -y, false);
  }

  inline uint32_t initTranslateFxFy(uint32_t extend, uint32_t filter, int64_t tx64, int64_t ty64) noexcept {
    uint32_t fetchBase = kFetchTypeTextureAAPadX;

    uint32_t wx = uint32_t(tx64 & 0xFF);
    uint32_t wy = uint32_t(ty64 & 0xFF);

    int tx = -int(((tx64)) >> 8);
    int ty = -int(((ty64)) >> 8);

    // If one or both `wx` or `why` are non-zero it means that the translation
    // is fractional. In that case we must calculate weights of [x0 y0], [x1 y0],
    // [x0 y1], and [x1 y1] pixels.
    bool isFractional = (wx | wy) != 0;
    if (isFractional) {
      if (filter == Pattern::kFilterNearest) {
        tx -= (wx >= 128);
        ty -= (wy >= 128);
        isFractional = false;
      }
      else {
        simple.wa = ((      wy) * (      wx)      ) >> 8; // [x0 y0]
        simple.wb = ((      wy) * (256 - wx) + 255) >> 8; // [x1 y0]
        simple.wc = ((256 - wy) * (      wx)      ) >> 8; // [x0 y1]
        simple.wd = ((256 - wy) * (256 - wx) + 255) >> 8; // [x1 y1]

        // The FxFy fetcher must work even when one or both `wx` or `wy` are
        // zero, so we always decrement `tx` and `ty` based on the fetch type.
        if (wy == 0) {
          tx--;
          fetchBase = kFetchTypeTextureFxPadX;
        }
        else if (wx == 0) {
          ty--;
          fetchBase = kFetchTypeTextureFyPadX;
        }
        else {
          tx--;
          ty--;
          fetchBase = kFetchTypeTextureFxFyPadX;
        }
      }
    }

    return _initTranslateAny(fetchBase, extend, tx, ty, isFractional);
  }

  // --------------------------------------------------------------------------
  // [Init - Affine]
  // --------------------------------------------------------------------------

  uint32_t initAffine(uint32_t extend, uint32_t filter, const Matrix2D& m, const Matrix2D& mInv) noexcept {
    // Inverted transformation matrix.
    double xx = mInv.m00();
    double xy = mInv.m01();
    double yx = mInv.m10();
    double yy = mInv.m11();

    if (Math::isNearOne(xx) && Math::isNearZero(xy) && Math::isNearZero(yx) && Math::isNearOne(yy))
      return initTranslateFxFy(extend, filter, Math::i64floor(-mInv.m20() * 256.0), Math::i64floor(-mInv.m21() * 256.0));

    uint32_t fetchType = (filter == Pattern::kFilterNearest) ? kFetchTypeTextureAffineNnAny
                                                             : kFetchTypeTextureAffineBiAny;

    // Pattern bounds.
    int tw = int(src.width);
    int th = int(src.height);

    uint32_t opt = Math::max(tw, th) < 32767 && src.stride >= 0 && src.stride <= intptr_t(std::numeric_limits<int16_t>::max());

    // TODO: Remove this constraint.
    if (filter == Pattern::kFilterBilinear)
      opt = 0;

    fetchType += opt;

    // Pattern extends.
    uint32_t extendX = Pattern::extendXFromExtend(extend);
    uint32_t extendY = Pattern::extendYFromExtend(extend);

    // Translation.
    double tx = mInv.m20();
    double ty = mInv.m21();

    tx += 0.5 * (xx + yx);
    ty += 0.5 * (xy + yy);

    // 32x32 fixed point scale as double, equals to `pow(2, 32)`.
    double fpScale = 4294967296.0;

    // Overflow check of X/Y. When this check passes we decrement rx/ry from
    // the overflown values.
    int ox = std::numeric_limits<int32_t>::max();
    int oy = std::numeric_limits<int32_t>::max();

    // Normalization of X/Y. These values are added to the current `px` and `py`
    // when they overflow the repeat|reflect bounds.
    int rx = 0;
    int ry = 0;

    const int32_t kRoRMinXY = std::numeric_limits<int32_t>::min();
    const int32_t kRoRMaxXY = std::numeric_limits<int32_t>::max();

    if (extendX == Pattern::kExtendPad) {
      affine.minX = 0;
      affine.maxX = int32_t(tw - 1);
    }
    else {
      affine.minX = kRoRMinXY;
      affine.maxX = kRoRMaxXY;

      ox = tw;
      if (extendX == Pattern::kExtendReflect) tw *= 2;

      if (xx < 0.0) {
        xx = -xx;
        yx = -yx;
        tx = double(tw) - tx;
        if (extendX == Pattern::kExtendRepeat) ox = 0;
      }

      ox--;
    }

    if (extendY == Pattern::kExtendPad) {
      affine.minY = 0;
      affine.maxY = int32_t(th - 1);
    }
    else {
      affine.minY = kRoRMinXY;
      affine.maxY = kRoRMaxXY;

      oy = th;
      if (extendY == Pattern::kExtendReflect) th *= 2;

      if (xy < 0.0) {
        xy = -xy;
        yy = -yy;
        ty = double(th) - ty;
        if (extendY == Pattern::kExtendRepeat) oy = 0;
      }

      oy--;
    }

    // Keep the center of the pixel at [0.5, 0.5] if the filter is NEAREST so
    // it can properly round to the nearest pixel during the fetch phase.
    // However, if the filter is not NEAREST the `tx` and `ty` have to be
    // translated by -0.5 so the position starts at the beginning of the pixel.
    if (filter != Pattern::kFilterNearest) {
      tx -= 0.5;
      ty -= 0.5;
    }

    // Texture boundaries converted to `double`.
    double tw_d = double(tw);
    double th_d = double(th);

    // Normalize the matrix in a way that it won't overflow the texture more
    // than once per a single iteration. Happens when scaling is greater than
    // one pixel step. Only useful for repeated / reflected cases.
    if (extendX == Pattern::kExtendPad) {
      tw_d = 4294967296.0;
    }
    else {
      tx = std::fmod(tx, tw_d);
      rx = tw;
      if (xx >= tw_d) xx = std::fmod(xx, tw_d);
    }

    if (extendY == Pattern::kExtendPad) {
      th_d = 4294967296.0;
    }
    else {
      ty = std::fmod(ty, th_d);
      ry = th;
      if (xy >= th_d) xy = std::fmod(xy, th_d);
    }

    affine.xx.i64 = Math::i64floor(xx * fpScale);
    affine.xy.i64 = Math::i64floor(xy * fpScale);
    affine.yx.i64 = Math::i64floor(yx * fpScale);
    affine.yy.i64 = Math::i64floor(yy * fpScale);

    affine.tx.i64 = Math::i64floor(tx * fpScale);
    affine.ty.i64 = Math::i64floor(ty * fpScale);
    affine.rx.i64 = Support::shl(int64_t(rx), 32);
    affine.ry.i64 = Support::shl(int64_t(ry), 32);

    affine.ox.i32Hi = ox;
    affine.ox.i32Lo = std::numeric_limits<int32_t>::max();
    affine.oy.i32Hi = oy;
    affine.oy.i32Lo = std::numeric_limits<int32_t>::max();

    affine.tw = tw_d;
    affine.th = th_d;

    affine.xx2.u64 = affine.xx.u64 << 1u;
    affine.xy2.u64 = affine.xy.u64 << 1u;

    if (extendX >= Pattern::kExtendRepeat && affine.xx2.u32Hi >= uint32_t(tw)) affine.xx2.u32Hi %= uint32_t(tw);
    if (extendY >= Pattern::kExtendRepeat && affine.xy2.u32Hi >= uint32_t(th)) affine.xy2.u32Hi %= uint32_t(th);

    // TODO: Hardcoded for 32-bit PRGB/XRGB formats.
    affine.addrMul[0] = 4u;
    affine.addrMul[1] = int16_t(src.stride);

    return fetchType;
  }

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  inline void reset() noexcept { std::memset(this, 0, sizeof(*this)); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Simple texture data (only identity or translation matrix).
  struct alignas(16) Simple {
    int32_t tx, ty;                    //!< Translate by x/y (inverted).
    int32_t rx, ry;                    //!< Repeat/Reflect w/h.
    ModTable ix;                       //!< Safe X increments by 1..16 (fetchN).
    uint32_t wa;                       //!< 9-bit or 17-bit weight at [0, 0] (A).
    uint32_t wb;                       //!< 9-bit or 17-bit weight at [1, 0] (B).
    uint32_t wc;                       //!< 9-bit or 17-bit weight at [0, 1] (C).
    uint32_t wd;                       //!< 9-bit or 17-bit weight at [1, 1] (D).
  };

  //! Affine texture data.
  struct alignas(16) Affine {
    Value64 xx, xy;                    //!< Single X/Y step in X direction.
    Value64 yx, yy;                    //!< Single X/Y step in Y direction.
    Value64 tx, ty;                    //!< Texture offset at [0, 0].
    Value64 ox, oy;                    //!< Texture overflow check.
    Value64 rx, ry;                    //!< Texture overflow correction (repeat/reflect).
    Value64 xx2, xy2;                  //!< Two X/Y steps in X direction, used by `fetch4()`.
    int32_t minX, minY;                //!< Texture padding minimum.
    int32_t maxX, maxY;                //!< Texture padding maximum.
    double tw, th;                     //!< Repeated tile width/height (doubled if reflected).

    int16_t addrMul[2];                //!< 32-bit value to be used by [V]PMADDWD instruction to calculate address from Y/X pairs.
  };

  PixelData src;                       //!< Source data, used by all textures.
  union {
    Simple simple;                     //!< Simple texture data.
    Affine affine;                     //!< Affine texture data.
  };
};

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_PIPE2D_FETCHTEXTUREDATA_P_H
