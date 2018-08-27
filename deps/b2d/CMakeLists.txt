cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

# Don't create a project if it was already created by another CMakeLists.txt.
# This allows one library to embed another library without a project collision.
if(NOT CMAKE_PROJECT_NAME OR "${CMAKE_PROJECT_NAME}" STREQUAL "b2d")
  project(b2d CXX)
endif()

if (NOT DEFINED B2D_BUILD_EMBED)
  set(B2D_BUILD_EMBED FALSE)
endif()

if (NOT DEFINED B2D_BUILD_STATIC)
  set(B2D_BUILD_STATIC ${B2D_BUILD_EMBED})
endif()

# =============================================================================
# [B2D - Configuration]
# =============================================================================

set(B2D_DIR         "${CMAKE_CURRENT_LIST_DIR}" CACHE PATH "Location of 'b2d'")
set(ASMJIT_DIR      "${B2D_DIR}/../asmjit"      CACHE PATH "Location of 'asmjit'")

set(B2D_BUILD_EMBED  ${B2D_BUILD_EMBED}  CACHE BOOLEAN "Embed 'b2d' library (no targets)")
set(B2D_BUILD_STATIC ${B2D_BUILD_STATIC} CACHE BOOLEAN "Build 'b2d' library as static")
set(B2D_BUILD_BENCH  FALSE               CACHE BOOLEAN "Build 'b2d_bench' application")
set(B2D_BUILD_TEST   FALSE               CACHE BOOLEAN "Build 'b2d_test' applications")

if(NOT DEFINED ASMJIT_BUILD_EMBED)
  set(ASMJIT_BUILD_EMBED TRUE CACHE BOOLEAN "")
endif()

# =============================================================================
# [B2D - Project]
# =============================================================================

include("${B2D_DIR}/CxxProject.cmake")

cxx_project(b2d)
cxx_detect_standard(B2D_PRIVATE_CFLAGS)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set(B2D_PRIVATE_LFLAGS "/OPT:REF /OPT:ICF")

  list(APPEND B2D_PRIVATE_CFLAGS /MP)            # [+] Multi-process compilation
  list(APPEND B2D_PRIVATE_CFLAGS /GF)            # [+] Eliminate duplicate strings.
  list(APPEND B2D_PRIVATE_CFLAGS /GR-)           # [-] Runtime type information.

  list(APPEND B2D_PRIVATE_CFLAGS_DBG /GS)        # [+] Buffer security-check.
  list(APPEND B2D_PRIVATE_CFLAGS_REL /GS-)       # [-] Buffer security-check.

  list(APPEND B2D_PRIVATE_CFLAGS_REL /Ox)        # [+] Global optimization.
endif()

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "^(GNU|Clang|AppleClang)$")
  cxx_detect_cflags(B2D_PRIVATE_CFLAGS
    "-fvisibility=hidden"
    "-fno-exceptions"
    "-fno-math-errno"
    "-fno-tree-vectorize")
  cxx_detect_cflags(B2D_PRIVATE_CFLAGS_REL
    "-O2"
    "-fno-keep-static-consts"
    "-fmerge-all-constants")

  if(B2D_BUILD_SANITIZE)
    cxx_detect_cflags(B2D_SANITIZE_FLAGS "-fsanitize=undefined")
    list(APPEND B2D_PRIVATE_CFLAGS ${B2D_SANITIZE_FLAGS})
    list(APPEND B2D_PRIVATE_LFLAGS ${B2D_SANITIZE_FLAGS})
  endif()
endif()

# TODO: Detect ARM when the support is added.
if(1)
  set(B2D_BUILD_SSE2   TRUE)
  set(B2D_BUILD_SSE3   TRUE)
  set(B2D_BUILD_SSSE3  TRUE)

  set(B2D_BUILD_SSE4_1 TRUE)
  set(B2D_BUILD_SSE4_2 TRUE)
  set(B2D_BUILD_AVX    TRUE)
  set(B2D_BUILD_AVX2   TRUE)

  # Use SSE2 by default on X86.
  list(APPEND B2D_PRIVATE_CFLAGS ${CXX_CFLAGS_SSE2})
endif()

# Dependencies.
if(WIN32)
  list(APPEND B2D_DEPS advapi32 gdi32 ole32 user32 uuid shell32)
else()
  list(APPEND B2D_DEPS m pthread)
endif()

include("${ASMJIT_DIR}/CMakeLists.txt")
list(APPEND B2D_DEPS ${ASMJIT_LIBS})
list(APPEND B2D_PRIVATE_CFLAGS ${ASMJIT_CFLAGS})

# A possibility to reduce the resulting binary size by disabling asmjit logging.
# list(APPEND B2D_PRIVATE_CFLAGS -DASMJIT_DISABLE_LOGGING -DASMJIT_DISABLE_TEXT)

# Use Unicode by default on Windows target.
if(WIN32)
  list(APPEND B2D_PRIVATE_CFLAGS "${CXX_DEFINE}_UNICODE")
endif()

if(B2D_BUILD_SSE2)
  list(APPEND B2D_PRIVATE_CFLAGS "${CXX_DEFINE}B2D_BUILD_SSE2")
endif()

if(B2D_BUILD_SSE3)
  list(APPEND B2D_PRIVATE_CFLAGS "${CXX_DEFINE}B2D_BUILD_SSE3")
endif()

if(B2D_BUILD_SSSE3)
  list(APPEND B2D_PRIVATE_CFLAGS "${CXX_DEFINE}B2D_BUILD_SSSE3")
endif()

if(B2D_BUILD_SSE4_1)
  list(APPEND B2D_PRIVATE_CFLAGS "${CXX_DEFINE}B2D_BUILD_SSE4_1")
endif()

if(B2D_BUILD_SSE4_2)
  list(APPEND B2D_PRIVATE_CFLAGS "${CXX_DEFINE}B2D_BUILD_SSE4_2")
endif()

if(B2D_BUILD_AVX)
  list(APPEND B2D_PRIVATE_CFLAGS "${CXX_DEFINE}B2D_BUILD_AVX")
endif()

if(B2D_BUILD_AVX2)
  list(APPEND B2D_PRIVATE_CFLAGS "${CXX_DEFINE}B2D_BUILD_AVX2")
endif()

list(REMOVE_DUPLICATES B2D_DEPS)
list(REMOVE_DUPLICATES B2D_PRIVATE_CFLAGS)

set(B2D_LIBS ${B2D_DEPS})
if(NOT B2D_BUILD_EMBED)
  list(INSERT B2D_LIBS 0 b2d)
endif()

cxx_project_info(b2d)

# =============================================================================
# [B2D - Source]
# =============================================================================

set(B2D_SRC "")

cxx_add_source(b2d B2D_SRC b2d
  b2d.h
)

cxx_add_source(b2d B2D_SRC b2d/codec
  bmpcodec.cpp
  bmpcodec_p.h
  deflate.cpp
  deflate_p.h
  jpegcodec.cpp
  jpegcodec_p.h
  jpegutils.cpp
  jpegutils_sse2.cpp
  jpegutils_p.h
  pngcodec.cpp
  pngcodec_p.h
  pngutils.cpp
  pngutils_sse2.cpp
  pngutils_p.h
)

cxx_add_source(b2d B2D_SRC b2d/core
  allocator.cpp
  allocator.h
  any.cpp
  any.h
  argb.cpp
  argb.h
  array.cpp
  array.h
  build.h
  buffer.cpp
  buffer.h
  carray.h
  compop.h
  container.cpp
  container.h
  context2d.cpp
  context2d.h
  contextdefs.cpp
  contextdefs.h
  cookie.cpp
  cookie.h
  cputicks.cpp
  cputicks.h
  error.cpp
  error.h
  filesystem.cpp
  filesystem.h
  fill.h
  geom2d.cpp
  geom2d.h
  geomtypes.cpp
  geomtypes.h
  globals.cpp
  globals.h
  gradient.cpp
  gradient.h
  gradient_avx2.cpp
  gradient_sse2.cpp
  image.cpp
  image.h
  imagecodec.cpp
  imagecodec.h
  imagescaler.cpp
  imagescaler.h
  imageutils.cpp
  imageutils.h
  lock.cpp
  lock.h
  lookuptable_p.h
  math.cpp
  math.h
  math_integrate.cpp
  math_integrate.h
  math_roots.cpp
  math_roots.h
  matrix2d.cpp
  matrix2d.h
  matrix2d_avx.cpp
  matrix2d_sse2.cpp
  membuffer.cpp
  membuffer.h
  path2d.cpp
  path2d.h
  pathflatten.cpp
  pathflatten.h
  pattern.cpp
  pattern.h
  pixelconverter.cpp
  pixelconverter_avx2.cpp
  pixelconverter_sse2.cpp
  pixelconverter_ssse3.cpp
  pixelconverter.h
  pixelformat.cpp
  pixelformat.h
  pixelutils.cpp
  pixelutils_p.h
  random.cpp
  random.h
  region.cpp
  region.h
  region_sse2.cpp
  regioninternal.cpp
  regioninternal_p.h
  runtime.cpp
  runtime.h
  simd.h
  simd_x86.h
  simdutils_p.h
  string.cpp
  string.h
  stroke.cpp
  stroke.h
  support.cpp
  support.h
  unicode.cpp
  unicode.h
  unicodedata.cpp
  unicodedata.h
  unicodeio.cpp
  unicodeio_p.h
  uniqueidgenerator_p.h
  zeroallocator.cpp
  zeroallocator_p.h
  zerobuffer.cpp
  zerobuffer_p.h
  zone.cpp
  zone.h
  zonelist.cpp
  zonelist.h
  zonepool.h
  zonetree.cpp
  zonetree.h
)

cxx_add_source(b2d B2D_SRC b2d/pipe2d
  compopinfo.cpp
  compopinfo_p.h
  compoppart.cpp
  compoppart_p.h
  fetchdata_p.h
  fetchgradientdata_p.h
  fetchgradientpart.cpp
  fetchgradientpart_p.h
  fetchpart.cpp
  fetchpart_p.h
  fetchpixelptrpart.cpp
  fetchpixelptrpart_p.h
  fetchsoliddata_p.h
  fetchsolidpart.cpp
  fetchsolidpart_p.h
  fetchtexturedata_p.h
  fetchtexturepart.cpp
  fetchtexturepart_p.h
  fetchutils.cpp
  fetchutils_p.h
  filldata_p.h
  fillpart.cpp
  fillpart_p.h
  functioncache.cpp
  functioncache_p.h
  pipeglobals.cpp
  pipeglobals_p.h
  pipecompiler.cpp
  pipecompiler_p.h
  pipedebug_p.h
  pipehelpers_p.h
  pipepart.cpp
  pipepart_p.h
  pipepixel_p.h
  piperegusage_p.h
  piperuntime_p.h
  piperuntime.cpp
  pipesignature_p.h
  tables.cpp
  tables_p.h
)

cxx_add_source(b2d B2D_SRC b2d/rasterengine
  alternatives_p.h
  analyticrasterizer_p.h
  edgebuilder_p.h
  edgeflatten_p.h
  edgesource_p.h
  edgestorage_p.h
  rastercontext2d.cpp
  rastercontext2d_p.h
  rasterfetchdata_p.h
  rasterfiller.cpp
  rasterfiller_p.h
  rasterglobals_p.h
  rasterspan_p.h
  rasterstate_p.h
  rasterutils_p.h
  rasterworker.cpp
  rasterworker_p.h
)

cxx_add_source(b2d B2D_SRC b2d/text
  font.cpp
  font.h
  fontcollection.cpp
  fontcollection.h
  fontdata.cpp
  fontdata.h
  fontdiagnosticsinfo.h
  fontenumerator.cpp
  fontenumerator.h
  fontface.cpp
  fontface.h
  fontloader.cpp
  fontloader.h
  fontmatcher.cpp
  fontmatcher.h
  fontmetrics.h
  fontpanose.h
  fonttag.h
  fontunicodecoverage.h
  fontutils.cpp
  fontutils.h
  glyphengine.cpp
  glyphengine.h
  glyphitem.h
  glyphoutlinedecoder.cpp
  glyphoutlinedecoder.h
  glyphrun.h
  otcff.cpp
  otcff_p.h
  otcmap.cpp
  otcmap_p.h
  otface.cpp
  otface_p.h
  otglobals_p.h
  otglyf.cpp
  otglyf_p.h
  othead_p.h
  otkern.cpp
  otkern_p.h
  otlayout.cpp
  otlayout_p.h
  otmaxp_p.h
  otname.cpp
  otname_p.h
  otos2_p.h
  otplatform_p.h
  otpost_p.h
  otsfnt_p.h
  otmetrics.cpp
  otmetrics_p.h
  textbuffer.cpp
  textbuffer.h
  textglobals.h
  textmetrics.h
  textrun.h
)

# =============================================================================
# [B2D - Targets]
# =============================================================================

if(NOT B2D_BUILD_EMBED)
  cxx_add_library(b2d b2d
    "${B2D_SRC};${ASMJIT_SRC}"
    "${B2D_DEPS}"
    "${B2D_PRIVATE_CFLAGS}"
    "${B2D_PRIVATE_CFLAGS_DBG}"
    "${B2D_PRIVATE_CFLAGS_REL}"
  )

  target_include_directories(b2d BEFORE PRIVATE ${ASMJIT_INCLUDE_DIR})
  target_include_directories(b2d PUBLIC ${B2D_INCLUDE_DIR})

  foreach(i ${B2D_SRC})
    get_filename_component(path ${i} PATH)
    get_filename_component(name ${i} NAME)
    string(REGEX REPLACE "^${B2D_SOURCE_DIR}/" "" targetpath "${path}")
    if(${name} MATCHES ".h$")
      if(NOT "${name}" MATCHES "_p.h$")
        install(FILES ${i} DESTINATION "include/${targetpath}")
      endif()
    endif()
  endforeach()
endif()

# =============================================================================
# [B2D - Tests]
# =============================================================================

# The library is always embedded into the unit-test executable. This way it's
# much easier to test private functions compared to just linking to `b2d`.
if(B2D_BUILD_TEST)
  cxx_detect_standard(CXX_STD)
  cxx_add_source(b2d B2D_TEST_SRC ../test broken.cpp broken.h b2d_test_unit.cpp)
  set(B2D_TEST_CFLAGS ${B2D_PRIVATE_CFLAGS}
                      ${CXX_STD}
                      ${CXX_DEFINE}B2D_BUILD_STATIC
                      ${CXX_DEFINE}B2D_BUILD_TEST)

  add_executable(b2d_test_unit ${B2D_SRC} ${ASMJIT_SRC} ${B2D_TEST_SRC})
  set_target_properties(b2d_test_unit PROPERTIES LINK_FLAGS "${B2D_PRIVATE_LFLAGS}")
  target_link_libraries(b2d_test_unit ${B2D_DEPS})
  target_include_directories(b2d_test_unit BEFORE PRIVATE ${ASMJIT_INCLUDE_DIR} ${B2D_INCLUDE_DIR})

  if(CMAKE_BUILD_TYPE)
    if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
      target_compile_options(b2d_test_unit PUBLIC "${B2D_TEST_CFLAGS}" ${B2D_PRIVATE_CFLAGS_DBG})
    else()
      target_compile_options(b2d_test_unit PUBLIC "${B2D_TEST_CFLAGS}" ${B2D_PRIVATE_CFLAGS_REL})
    endif()
  else()
    target_compile_options(b2d_test_unit PUBLIC ${B2D_TEST_CFLAGS}
      $<$<CONFIG:Debug>:${B2D_PRIVATE_CFLAGS_DBG}>
      $<$<NOT:$<CONFIG:Debug>>:${B2D_PRIVATE_CFLAGS_REL}>)
  endif()

  set(B2D_SAMPLES_SRC b2d_test_sample)
  foreach(file ${B2D_SAMPLES_SRC})
    add_executable(${file} test/${file}.cpp)
    target_link_libraries(${file} b2d ${B2D_DEPS})
    target_include_directories(${file} BEFORE PRIVATE ${B2D_INCLUDE_DIR})
    target_compile_options(${file} PUBLIC "${CXX_STD}")
  endforeach(file)
endif()
