// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/math.h"
#include "../core/runtime.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::Math - Unit]
// ============================================================================

#ifdef B2D_BUILD_TEST
UNIT(b2d_core_math_base) {
  INFO("b2d::Math::normalize()");
  EXPECT(std::signbit(Math::normalize(-0.0f)) == false);
  EXPECT(std::signbit(Math::normalize(-0.0 )) == false);

  INFO("b2d::Math::bound()");
  EXPECT(Math::bound<int>(-1, 100, 1000) == 100);
  EXPECT(Math::bound<int>(99, 100, 1000) == 100);
  EXPECT(Math::bound<int>(1044, 100, 1000) == 1000);
  EXPECT(Math::bound<double>(-1.0, 100.0, 1000.0) == 100.0);
  EXPECT(Math::bound<double>(99.0, 100.0, 1000.0) == 100.0);
  EXPECT(Math::bound<double>(1044.0, 100.0, 1000.0) == 1000.0);

  INFO("b2d::Math::isUnitInterval()");
  EXPECT(Math::isUnitInterval( 0.0f  ) == true);
  EXPECT(Math::isUnitInterval( 0.0   ) == true);
  EXPECT(Math::isUnitInterval( 0.5f  ) == true);
  EXPECT(Math::isUnitInterval( 0.5   ) == true);
  EXPECT(Math::isUnitInterval( 1.0f  ) == true);
  EXPECT(Math::isUnitInterval( 1.0   ) == true);
  EXPECT(Math::isUnitInterval(-0.0f  ) == true);
  EXPECT(Math::isUnitInterval(-0.0   ) == true);
  EXPECT(Math::isUnitInterval(-1.0f  ) == false);
  EXPECT(Math::isUnitInterval(-1.0   ) == false);
  EXPECT(Math::isUnitInterval( 1.001f) == false);
  EXPECT(Math::isUnitInterval( 1.001 ) == false);

  INFO("b2d::Math::floor()");
  EXPECT(Math::floor(-1.5f) ==-2.0f);
  EXPECT(Math::floor(-1.5 ) ==-2.0 );
  EXPECT(Math::floor(-0.9f) ==-1.0f);
  EXPECT(Math::floor(-0.9 ) ==-1.0 );
  EXPECT(Math::floor(-0.5f) ==-1.0f);
  EXPECT(Math::floor(-0.5 ) ==-1.0 );
  EXPECT(Math::floor(-0.1f) ==-1.0f);
  EXPECT(Math::floor(-0.1 ) ==-1.0 );
  EXPECT(Math::floor( 0.0f) == 0.0f);
  EXPECT(Math::floor( 0.0 ) == 0.0 );
  EXPECT(Math::floor( 0.1f) == 0.0f);
  EXPECT(Math::floor( 0.1 ) == 0.0 );
  EXPECT(Math::floor( 0.5f) == 0.0f);
  EXPECT(Math::floor( 0.5 ) == 0.0 );
  EXPECT(Math::floor( 0.9f) == 0.0f);
  EXPECT(Math::floor( 0.9 ) == 0.0 );
  EXPECT(Math::floor( 1.5f) == 1.0f);
  EXPECT(Math::floor( 1.5 ) == 1.0 );
  EXPECT(Math::floor(-4503599627370496.0) == -4503599627370496.0);
  EXPECT(Math::floor( 4503599627370496.0) ==  4503599627370496.0);

  INFO("b2d::Math::ceil()");
  EXPECT(Math::ceil(-1.5f) ==-1.0f);
  EXPECT(Math::ceil(-1.5 ) ==-1.0 );
  EXPECT(Math::ceil(-0.9f) == 0.0f);
  EXPECT(Math::ceil(-0.9 ) == 0.0 );
  EXPECT(Math::ceil(-0.5f) == 0.0f);
  EXPECT(Math::ceil(-0.5 ) == 0.0 );
  EXPECT(Math::ceil(-0.1f) == 0.0f);
  EXPECT(Math::ceil(-0.1 ) == 0.0 );
  EXPECT(Math::ceil( 0.0f) == 0.0f);
  EXPECT(Math::ceil( 0.0 ) == 0.0 );
  EXPECT(Math::ceil( 0.1f) == 1.0f);
  EXPECT(Math::ceil( 0.1 ) == 1.0 );
  EXPECT(Math::ceil( 0.5f) == 1.0f);
  EXPECT(Math::ceil( 0.5 ) == 1.0 );
  EXPECT(Math::ceil( 0.9f) == 1.0f);
  EXPECT(Math::ceil( 0.9 ) == 1.0 );
  EXPECT(Math::ceil( 1.5f) == 2.0f);
  EXPECT(Math::ceil( 1.5 ) == 2.0 );
  EXPECT(Math::ceil(-4503599627370496.0) == -4503599627370496.0);
  EXPECT(Math::ceil( 4503599627370496.0) ==  4503599627370496.0);

  INFO("b2d::Math::trunc()");
  EXPECT(Math::trunc(-1.5f) ==-1.0f);
  EXPECT(Math::trunc(-1.5 ) ==-1.0 );
  EXPECT(Math::trunc(-0.9f) == 0.0f);
  EXPECT(Math::trunc(-0.9 ) == 0.0 );
  EXPECT(Math::trunc(-0.5f) == 0.0f);
  EXPECT(Math::trunc(-0.5 ) == 0.0 );
  EXPECT(Math::trunc(-0.1f) == 0.0f);
  EXPECT(Math::trunc(-0.1 ) == 0.0 );
  EXPECT(Math::trunc( 0.0f) == 0.0f);
  EXPECT(Math::trunc( 0.0 ) == 0.0 );
  EXPECT(Math::trunc( 0.1f) == 0.0f);
  EXPECT(Math::trunc( 0.1 ) == 0.0 );
  EXPECT(Math::trunc( 0.5f) == 0.0f);
  EXPECT(Math::trunc( 0.5 ) == 0.0 );
  EXPECT(Math::trunc( 0.9f) == 0.0f);
  EXPECT(Math::trunc( 0.9 ) == 0.0 );
  EXPECT(Math::trunc( 1.5f) == 1.0f);
  EXPECT(Math::trunc( 1.5 ) == 1.0 );
  EXPECT(Math::trunc(-4503599627370496.0) == -4503599627370496.0);
  EXPECT(Math::trunc( 4503599627370496.0) ==  4503599627370496.0);

  INFO("b2d::Math::round()");
  EXPECT(Math::round(-1.5f) ==-1.0f);
  EXPECT(Math::round(-1.5 ) ==-1.0 );
  EXPECT(Math::round(-0.9f) ==-1.0f);
  EXPECT(Math::round(-0.9 ) ==-1.0 );
  EXPECT(Math::round(-0.5f) == 0.0f);
  EXPECT(Math::round(-0.5 ) == 0.0 );
  EXPECT(Math::round(-0.1f) == 0.0f);
  EXPECT(Math::round(-0.1 ) == 0.0 );
  EXPECT(Math::round( 0.0f) == 0.0f);
  EXPECT(Math::round( 0.0 ) == 0.0 );
  EXPECT(Math::round( 0.1f) == 0.0f);
  EXPECT(Math::round( 0.1 ) == 0.0 );
  EXPECT(Math::round( 0.5f) == 1.0f);
  EXPECT(Math::round( 0.5 ) == 1.0 );
  EXPECT(Math::round( 0.9f) == 1.0f);
  EXPECT(Math::round( 0.9 ) == 1.0 );
  EXPECT(Math::round( 1.5f) == 2.0f);
  EXPECT(Math::round( 1.5 ) == 2.0 );
  EXPECT(Math::round(-4503599627370496.0) == -4503599627370496.0);
  EXPECT(Math::round( 4503599627370496.0) ==  4503599627370496.0);

  INFO("b2d::Math::roundeven()");
  EXPECT(Math::roundeven(-1.5f) ==-2.0f);
  EXPECT(Math::roundeven(-1.5 ) ==-2.0 );
  EXPECT(Math::roundeven(-0.9f) ==-1.0f);
  EXPECT(Math::roundeven(-0.9 ) ==-1.0 );
  EXPECT(Math::roundeven(-0.5f) == 0.0f);
  EXPECT(Math::roundeven(-0.5 ) == 0.0 );
  EXPECT(Math::roundeven(-0.1f) == 0.0f);
  EXPECT(Math::roundeven(-0.1 ) == 0.0 );
  EXPECT(Math::roundeven( 0.0f) == 0.0f);
  EXPECT(Math::roundeven( 0.0 ) == 0.0 );
  EXPECT(Math::roundeven( 0.1f) == 0.0f);
  EXPECT(Math::roundeven( 0.1 ) == 0.0 );
  EXPECT(Math::roundeven( 0.5f) == 0.0f);
  EXPECT(Math::roundeven( 0.5 ) == 0.0 );
  EXPECT(Math::roundeven( 0.9f) == 1.0f);
  EXPECT(Math::roundeven( 0.9 ) == 1.0 );
  EXPECT(Math::roundeven( 1.5f) == 2.0f);
  EXPECT(Math::roundeven( 1.5 ) == 2.0 );
  EXPECT(Math::roundeven(-4503599627370496.0) == -4503599627370496.0);
  EXPECT(Math::roundeven( 4503599627370496.0) ==  4503599627370496.0);

  INFO("b2d::Math::ifloor()");
  EXPECT(Math::ifloor(-1.5f) ==-2);
  EXPECT(Math::ifloor(-1.5 ) ==-2);
  EXPECT(Math::ifloor(-0.9f) ==-1);
  EXPECT(Math::ifloor(-0.9 ) ==-1);
  EXPECT(Math::ifloor(-0.5f) ==-1);
  EXPECT(Math::ifloor(-0.5 ) ==-1);
  EXPECT(Math::ifloor(-0.1f) ==-1);
  EXPECT(Math::ifloor(-0.1 ) ==-1);
  EXPECT(Math::ifloor( 0.0f) == 0);
  EXPECT(Math::ifloor( 0.0 ) == 0);
  EXPECT(Math::ifloor( 0.1f) == 0);
  EXPECT(Math::ifloor( 0.1 ) == 0);
  EXPECT(Math::ifloor( 0.5f) == 0);
  EXPECT(Math::ifloor( 0.5 ) == 0);
  EXPECT(Math::ifloor( 0.9f) == 0);
  EXPECT(Math::ifloor( 0.9 ) == 0);
  EXPECT(Math::ifloor( 1.5f) == 1);
  EXPECT(Math::ifloor( 1.5 ) == 1);

  INFO("b2d::Math::iceil()");
  EXPECT(Math::iceil(-1.5f) ==-1);
  EXPECT(Math::iceil(-1.5 ) ==-1);
  EXPECT(Math::iceil(-0.9f) == 0);
  EXPECT(Math::iceil(-0.9 ) == 0);
  EXPECT(Math::iceil(-0.5f) == 0);
  EXPECT(Math::iceil(-0.5 ) == 0);
  EXPECT(Math::iceil(-0.1f) == 0);
  EXPECT(Math::iceil(-0.1 ) == 0);
  EXPECT(Math::iceil( 0.0f) == 0);
  EXPECT(Math::iceil( 0.0 ) == 0);
  EXPECT(Math::iceil( 0.1f) == 1);
  EXPECT(Math::iceil( 0.1 ) == 1);
  EXPECT(Math::iceil( 0.5f) == 1);
  EXPECT(Math::iceil( 0.5 ) == 1);
  EXPECT(Math::iceil( 0.9f) == 1);
  EXPECT(Math::iceil( 0.9 ) == 1);
  EXPECT(Math::iceil( 1.5f) == 2);
  EXPECT(Math::iceil( 1.5 ) == 2);

  INFO("b2d::Math::itrunc()");
  EXPECT(Math::itrunc(-1.5f) ==-1);
  EXPECT(Math::itrunc(-1.5 ) ==-1);
  EXPECT(Math::itrunc(-0.9f) == 0);
  EXPECT(Math::itrunc(-0.9 ) == 0);
  EXPECT(Math::itrunc(-0.5f) == 0);
  EXPECT(Math::itrunc(-0.5 ) == 0);
  EXPECT(Math::itrunc(-0.1f) == 0);
  EXPECT(Math::itrunc(-0.1 ) == 0);
  EXPECT(Math::itrunc( 0.0f) == 0);
  EXPECT(Math::itrunc( 0.0 ) == 0);
  EXPECT(Math::itrunc( 0.1f) == 0);
  EXPECT(Math::itrunc( 0.1 ) == 0);
  EXPECT(Math::itrunc( 0.5f) == 0);
  EXPECT(Math::itrunc( 0.5 ) == 0);
  EXPECT(Math::itrunc( 0.9f) == 0);
  EXPECT(Math::itrunc( 0.9 ) == 0);
  EXPECT(Math::itrunc( 1.5f) == 1);
  EXPECT(Math::itrunc( 1.5 ) == 1);

  INFO("b2d::Math::iround()");
  EXPECT(Math::iround(-1.5f) ==-1);
  EXPECT(Math::iround(-1.5 ) ==-1);
  EXPECT(Math::iround(-0.9f) ==-1);
  EXPECT(Math::iround(-0.9 ) ==-1);
  EXPECT(Math::iround(-0.5f) == 0);
  EXPECT(Math::iround(-0.5 ) == 0);
  EXPECT(Math::iround(-0.1f) == 0);
  EXPECT(Math::iround(-0.1 ) == 0);
  EXPECT(Math::iround( 0.0f) == 0);
  EXPECT(Math::iround( 0.0 ) == 0);
  EXPECT(Math::iround( 0.1f) == 0);
  EXPECT(Math::iround( 0.1 ) == 0);
  EXPECT(Math::iround( 0.5f) == 1);
  EXPECT(Math::iround( 0.5 ) == 1);
  EXPECT(Math::iround( 0.9f) == 1);
  EXPECT(Math::iround( 0.9 ) == 1);
  EXPECT(Math::iround( 1.5f) == 2);
  EXPECT(Math::iround( 1.5 ) == 2);

  INFO("b2d::Math::frac()");
  EXPECT(Math::frac( 0.00f) == 0.00f);
  EXPECT(Math::frac( 0.00 ) == 0.00 );
  EXPECT(Math::frac( 1.00f) == 0.00f);
  EXPECT(Math::frac( 1.00 ) == 0.00 );
  EXPECT(Math::frac( 1.25f) == 0.25f);
  EXPECT(Math::frac( 1.25 ) == 0.25 );
  EXPECT(Math::frac( 1.75f) == 0.75f);
  EXPECT(Math::frac( 1.75 ) == 0.75 );
  EXPECT(Math::frac(-1.00f) == 0.00f);
  EXPECT(Math::frac(-1.00 ) == 0.00 );
  EXPECT(Math::frac(-1.25f) == 0.75f);
  EXPECT(Math::frac(-1.25 ) == 0.75 );
  EXPECT(Math::frac(-1.75f) == 0.25f);
  EXPECT(Math::frac(-1.75 ) == 0.25 );

  INFO("b2d::Math::fracWithSign()");
  EXPECT(Math::fracWithSign( 0.00f) ==  0.00f);
  EXPECT(Math::fracWithSign( 0.00 ) ==  0.00 );
  EXPECT(Math::fracWithSign( 1.00f) ==  0.00f);
  EXPECT(Math::fracWithSign( 1.00 ) ==  0.00 );
  EXPECT(Math::fracWithSign( 1.25f) ==  0.25f);
  EXPECT(Math::fracWithSign( 1.25 ) ==  0.25 );
  EXPECT(Math::fracWithSign( 1.75f) ==  0.75f);
  EXPECT(Math::fracWithSign( 1.75 ) ==  0.75 );
  EXPECT(Math::fracWithSign(-1.00f) ==  0.00f);
  EXPECT(Math::fracWithSign(-1.00 ) ==  0.00 );
  EXPECT(Math::fracWithSign(-1.25f) == -0.25f);
  EXPECT(Math::fracWithSign(-1.25 ) == -0.25 );
  EXPECT(Math::fracWithSign(-1.75f) == -0.75f);
  EXPECT(Math::fracWithSign(-1.75 ) == -0.75 );
}
#endif

B2D_END_NAMESPACE
