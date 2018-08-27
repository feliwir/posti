// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Export]
#define B2D_EXPORTS

// [Dependencies]
#include "../core/math_integrate.h"

B2D_BEGIN_NAMESPACE

// ============================================================================
// [b2d::Math - Integrate]
// ============================================================================

Error Math::integrate(double& dst, EvaluateFunc func, void* funcData, double tMin, double tMax, int nSteps) noexcept {
  if (B2D_UNLIKELY(tMax < tMin || nSteps == 0))
    return DebugUtils::errored(kErrorInvalidArgument);

  static constexpr double x[] = {
    0.1488743389816312108848260,
    0.4333953941292471907992659,
    0.6794095682990244062343274,
    0.8650633666889845107320967,
    0.9739065285171717200779640
  };

  static constexpr double w[] = {
    0.2955242247147528701738930,
    0.2692667193099963550912269,
    0.2190863625159820439955349,
    0.1494513491505805931457763,
    0.0666713443086881375935688
  };

  double tEnd = tMax;
  double step = (tMax - tMin) / double(nSteps);
  double result = 0.0;

  tMax = tMin + step;
  nSteps--;

  for (int step = 0; step <= nSteps; step++) {
    // There is an error accumulated when advancing 'b'. If this is the last
    // step it's good to use overwrite 'b' based on the end of the given interval.
    if (step == nSteps)
      tMax = tEnd;

    double xm = 0.5 * (tMax + tMin);
    double xr = 0.5 * (tMax - tMin);

    double r[10];
    double t[10];

    double d;
    double sum;

    d = x[0] * xr; t[0] = xm + d; t[1] = xm - d;
    d = x[1] * xr; t[2] = xm + d; t[3] = xm - d;
    d = x[2] * xr; t[4] = xm + d; t[5] = xm - d;
    d = x[3] * xr; t[6] = xm + d; t[7] = xm - d;
    d = x[4] * xr; t[8] = xm + d; t[9] = xm - d;

    B2D_PROPAGATE(func(funcData, r, t, 10));

    sum  = w[0] * (r[0] + r[1]);
    sum += w[1] * (r[2] + r[3]);
    sum += w[2] * (r[4] + r[5]);
    sum += w[3] * (r[6] + r[7]);
    sum += w[4] * (r[8] + r[9]);

    tMin = tMax;
    tMax += step;
    result += sum * xr;
  }

  dst = result;
  return kErrorOk;
}

B2D_END_NAMESPACE
