// [Blend-Test]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

#include <cmath>
#include <cstdio>
#include "b2d/b2d.h"

int main(int argc, char* argv[]) {
  b2d::Image img(800, 500, b2d::PixelFormat::kPRGB32);
  b2d::Context2D ctx(img);

  ctx.setCompOp(b2d::CompOp::kSrc);
  ctx.setFillStyle(b2d::Argb32(0xFF000000));
  ctx.fillAll();

  b2d::FontFace face;
  face.createFromFile("/home/petr/workspace/fonts/otfeat.ttf");

  b2d::Font font;
  font.createFromFace(face, 50.0f);

  b2d::TextBuffer buf;
  b2d::GlyphEngine ge;
  ge.createFromFont(font);

  const char str[] = "abcdefghijklm";
  buf.setUtf8Text(str);
  buf.shape(ge);

  b2d::TextMetrics tm;
  buf.measureText(ge, tm);

  printf("Advance: %g\n", tm.advanceWidth());
  printf("LSB RSB: [%g %g]\n", tm.boundingBox().x0(), tm.boundingBox().x1());

  ctx.setFillStyle(b2d::Argb32(0xFFFFFFFF));
  ctx.fillGlyphRun(b2d::Point(100, 100), font, buf.glyphRun());

  const b2d::FontDesignMetrics& dm = face.designMetrics();
  printf("Ascent: %d\n", dm.ascent());
  printf("Descent: %d\n", dm.descent());
  printf("LineGap: %d\n", dm.lineGap());

  /*
  b2d::FontFace face;
  face.createFromFile("/home/petr/workspace/fonts/segoeui.ttf");

  b2d::Font font;
  font.createFromFace(face, 50.0f);

  b2d::TextBuffer buf;
  b2d::GlyphEngine ge;
  ge.createFromFont(font);

  const char str[] = "Hello Blend2D";
  buf.setUtf8Text(str);
  buf.shape(ge);

  b2d::TextMetrics tm;
  buf.measureText(ge, tm);

  printf("Advance: %g\n", tm.advanceWidth());
  printf("LSB RSB: [%g %g]\n", tm.boundingBox().x0(), tm.boundingBox().x1());

  ctx.setFillStyle(b2d::Argb32(0xFFFFFFFF));
  ctx.fillGlyphRun(b2d::Point(100, 100), font, buf.glyphRun());

  const b2d::FontDesignMetrics& dm = face.designMetrics();
  printf("Ascent: %d\n", dm.ascent());
  printf("Descent: %d\n", dm.descent());
  printf("LineGap: %d\n", dm.lineGap());
  */

  ctx.end();
  b2d::ImageUtils::writeImageToFile(
    "sample.bmp", b2d::ImageCodec::codecByName(b2d::ImageCodec::builtinCodecs(), "BMP"), img);

  return 0;
}
