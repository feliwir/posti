// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

#ifndef _B2D_B2D_H
#define _B2D_B2D_H

//! \defgroup b2d b2d

//! \defgroup b2d_core b2d/core
//! \ingroup b2d

//! \defgroup b2d_codecs b2d/codecs
//! \ingroup b2d

//! \defgroup b2d_pipe2d b2d/pipe2d
//! \ingroup b2d

//! \defgroup b2d_rasterengine b2d/rasterengine
//! \ingroup b2d

//! \defgroup b2d_core b2d/support
//! \ingroup b2d

//! \defgroup b2d_text b2d/text
//! \ingroup b2d

//! \defgroup b2d_text_ot b2d/text (OpenType)
//! \ingroup b2d

#include "./core/allocator.h"
#include "./core/any.h"
#include "./core/argb.h"
#include "./core/array.h"
#include "./core/build.h"
#include "./core/buffer.h"
#include "./core/carray.h"
#include "./core/container.h"
#include "./core/cookie.h"
#include "./core/cputicks.h"
#include "./core/error.h"
#include "./core/filesystem.h"
#include "./core/globals.h"
#include "./core/lock.h"
#include "./core/math.h"
#include "./core/math_integrate.h"
#include "./core/math_roots.h"
#include "./core/membuffer.h"
#include "./core/random.h"
#include "./core/runtime.h"
#include "./core/simd.h"
#include "./core/string.h"
#include "./core/support.h"
#include "./core/unicode.h"
#include "./core/unicodedata.h"
#include "./core/wrap.h"
#include "./core/zone.h"
#include "./core/zonelist.h"
#include "./core/zonepool.h"
#include "./core/zonetree.h"

#include "./core/compop.h"
#include "./core/context2d.h"
#include "./core/contextdefs.h"
#include "./core/fill.h"
#include "./core/geom2d.h"
#include "./core/geomtypes.h"
#include "./core/gradient.h"
#include "./core/image.h"
#include "./core/imagecodec.h"
#include "./core/imagescaler.h"
#include "./core/imageutils.h"
#include "./core/matrix2d.h"
#include "./core/path2d.h"
#include "./core/pathflatten.h"
#include "./core/pattern.h"
#include "./core/pixelconverter.h"
#include "./core/pixelformat.h"
#include "./core/region.h"
#include "./core/stroke.h"

#include "./text/font.h"
#include "./text/fontcollection.h"
#include "./text/fontdata.h"
#include "./text/fontdiagnosticsinfo.h"
#include "./text/fontenumerator.h"
#include "./text/fontloader.h"
#include "./text/fontmatcher.h"
#include "./text/fontmatrix.h"
#include "./text/fontmetrics.h"
#include "./text/fontpanose.h"
#include "./text/fonttag.h"
#include "./text/fontunicodecoverage.h"
#include "./text/fontutils.h"
#include "./text/glyphengine.h"
#include "./text/glyphitem.h"
#include "./text/glyphoutlinedecoder.h"
#include "./text/glyphrun.h"
#include "./text/textbuffer.h"
#include "./text/textglobals.h"
#include "./text/textmetrics.h"
#include "./text/textrun.h"

#endif // _B2D_B2D_H
