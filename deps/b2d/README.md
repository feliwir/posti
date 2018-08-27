Blend2D
-------

2D Vector Graphics Powered by a JIT Compiler.

  * [Official Repository (asmjit/asmjit)](https://github.com/blend2d/b2d)
  * [Official Blog (asmbits)](https://asmbits.blogspot.com/ncr)
  * [Official Chat (gitter)](https://gitter.im/blend2d/b2d)
  * [Permissive ZLIB license](./LICENSE.md)

Introduction
------------

Blend2D is a next generation 2D vector graphics engine written in C++, see [Blend2D homepage](https://blend2d.com) for more  details and basic use.

Alpha Release
-------------

Please note that this is an alpha release of Blend2D engine. Its purpose is to demonstrate the abilities (and possible future) of the project, but not to be used in production. Possible issues can be handled either privately or publicly (via github issues) and discussion should go to gitter.

Limitations
-----------

Here are some limitations that you should be aware of:

  - Text rendering is very simple and doesn't contain any text breaking abilities at the moment. Proper text layout API is planned in the future.
  - Text rendering doesn't do any caching - it renders text as paths and every time you render a glyph Blend2D decodes such path from font data and passes it to the rasterizer. This means that rendering very small text has currently huge overhead. On the other side thanks to the new rasterizer and optimized glyph decoders it should be much faster than doing the same in other 2D engines.
  - Blend2D only supports 32-bit pixel formats (PRGB32 and XRGB32), please don't try to use A8, which will be implemented together with font glyph caching and will be mostly used to store alpha masks.
  - Rendering context doesn't currently do any clipping. Actually rectangular clipping is working, but the API doesn't modify it. After thinking about this it was decided that Blend2D will support either rectangular clip box or span-based clipping that will use similar technique as the current rasterizer (bit-arrays). The second thing is that the clipping with masks is always problematic and it will not be the preferred way in Blend2D to render things that are clipped to paths. Blend2D would prefer to use layers instead where 2 layers where the first would be content and the second mask. This would allow much richer and faster way of clip-to-path.

TODO
----

  - Features:
    - [ ] Context2D - Restrict() - Make certain operations restricted.
    - [ ] Context2D/Pipe2D - Target - A8
    - [ ] Context2D/Pipe2D - Target - XRGB32 (finalize, test, ...)

  - Improvements:
    - [ ] Pipe2D - Fix affine bilinear texture fetch to work with `fetch4()`.
    - [ ] Pipe2D - Make AA blits faster (PAD/REPEAT/REFLECT).
    - [ ] Pipe2D - FillAnalytic must handle well cases where there is no end line on the right side (clipped out lines).
    - [ ] EdgeBuilder should not emit fully clipped out lines at the right border (optimization) (easy, wait until other tasks are done).
    - [ ] EdgeBuilder - should we calculate also a horizontal bounding-box and apply it to the filler? It seems it would be good.
    - [ ] Context2D/Pipe2D - Add 'conservative' option that would be used to select between conservative vs high-performance pipelines.
    - [ ] Context2D/Pipe2D - Add something like capabilities to FetchPart that can be used to do something very special, like `fetch16ToDst` - this can improve performance of SRC opaque fills like fetching linear gradients.
    - [ ] Context2D - Create SIMD versions of common things used in Context2D as compilers are not able to use SSE2 properly.
    - [ ] FetchData - Add two fetchType fields, one for conservative fetch, and one for high-performance fetch. This can be used to parametrize the pipeline based on complexity of `compOp` even after FetchData was already created.
    - [ ] AnalyticRasterizer - SIMD optimizations. I think it's still possible to make the rasterization faster.
    - [ ] Alpha/Mask should always be 0..255 or 0..1 (Questionable, rasterizers don't work like this) (Maybe masks should be normalized to 0.256 instead, I don't know what's better)
    - [ ] Rasterizer - stroked rect special-case
    - [ ] Image::create() should reuse its buffer if possible.
    - [ ] ImageScaler - Introduce SIMD optimizations.

  - Beta
    - [ ] Context2D - Clipping - Aligned/misaligned box
    - [ ] Context2D - Clipping - Region clipping by postprocessing rasterizer cells or creating spans before going to composition.
    - [ ] Context2D - Clipping - ClipToPath / TransformedBox
    - [ ] PathCrop - Reintroduce again as it will be needed for Text rendering (and text in a clip-box)

  - Research
    - [ ] Explore `/Qfast_transcendentals` option (MSVC)

License
-------

Blend2D can be distributed under [ZLIB License](./LICENSE.md).
