#include "FMath.hpp"

namespace fm {

double frexp(double x, int &e)
{
    uint64_t u = bits(x);
    int ex = (int)((u >> 52) & 0x7FF);
    if (ex == 0) {
        // denormal : on normalise d'abord (x * 2^54)
        if ((u & 0x7FFFFFFFFFFFFFFFull) == 0) { e = 0; return x; }
        x *= 18014398509481984.0;
        u = bits(x);
        ex = (int)((u >> 52) & 0x7FF) - 54;
    }
    if (ex == 0x7FF) { e = 0; return x; }
    e = ex - 1022;
    u = (u & ~(0x7FFull << 52)) | (1022ull << 52);
    return fromBits(u);
}

static inline double pow2(int k)   // -1022 <= k <= 1023
{
    return fromBits((uint64_t)(k + 1023) << 52);
}

double ldexp(double x, int e)
{
    while (e > 1023) { x *= pow2(1023); e -= 1023; if (isinf(x)) return x; }
    while (e < -1022) { x *= pow2(-1022); e += 1022; if (x == 0) return x; }
    return x * pow2(e);
}

double floor(double x)
{
    if (!isfinite(x)) return x;
    if (fabs(x) >= 4503599627370496.0) return x;   // 2^52 : deja entier
    double t = (double)(int64_t)x;
    if (t > x) t -= 1.0;
    return t;
}

double ceil(double x)
{
    return -floor(-x);
}

int iround(double x)
{
    // saturant : NaN -> 0, bornes +/- 1e9 (pas de debordement de int)
    if (!(x == x)) return 0;
    if (x > 1e9) return 1000000000;
    if (x < -1e9) return -1000000000;
    return x >= 0 ? (int)(x + 0.5) : -(int)(-x + 0.5);
}

double sqrt(double x)
{
    if (!(x > 0)) return x == 0 ? 0.0 : nan();
    if (isinf(x)) return x;
    int e;
    double m = frexp(x, e);           // [0,5 ; 1[
    if (e & 1) { m *= 2.0; e -= 1; }  // [0,5 ; 2[ et e pair
    double y = 0.41731 + 0.59016 * m;
    for (int i = 0; i < 5; i++) y = 0.5 * (y + m / y);
    return ldexp(y, e / 2);
}

// Inverses 1/n (evite les divisions, couteuses sans unite flottante).
static const double INV[32] = {
    0, 1.0, 1.0 / 2, 1.0 / 3, 1.0 / 4, 1.0 / 5, 1.0 / 6, 1.0 / 7, 1.0 / 8, 1.0 / 9, 1.0 / 10,
    1.0 / 11, 1.0 / 12, 1.0 / 13, 1.0 / 14, 1.0 / 15, 1.0 / 16, 1.0 / 17, 1.0 / 18, 1.0 / 19,
    1.0 / 20, 1.0 / 21, 1.0 / 22, 1.0 / 23, 1.0 / 24, 1.0 / 25, 1.0 / 26, 1.0 / 27, 1.0 / 28,
    1.0 / 29, 1.0 / 30, 1.0 / 31,
};

// ln 2 en deux parties (fdlibm) : k * LN2_HI est exact.
static const double LN2_HI = 6.93147180369123816490e-01;
static const double LN2_LO = 1.90821492927058770002e-10;

double ln(double x)
{
    if (isnan(x) || x < 0) return nan();
    if (x == 0) return -inf();
    if (isinf(x)) return x;
    int e;
    double m = frexp(x, e);
    if (m < 0.70710678118654752440) { m *= 2.0; e -= 1; }   // [sqrt(1/2) ; sqrt(2)[
    double s = (m - 1.0) / (m + 1.0);
    double s2 = s * s;
    // 2s (1 + s^2/3 + s^4/5 + ...)
    double p = INV[25];
    for (int k = 11; k >= 0; k--) p = p * s2 + INV[2 * k + 1];
    double r = 2.0 * s * p;
    return (e * LN2_HI + r) + e * LN2_LO;
}

double log10(double x)
{
    return ln(x) * 0.43429448190325182765;
}

double exp(double x)
{
    if (isnan(x)) return x;
    if (x > 709.78) return inf();
    if (x < -745.2) return 0.0;
    int k = iround(x / LN2);
    double r = (x - k * LN2_HI) - k * LN2_LO;   // |r| <= 0,35
    double p = 1.0;
    for (int n = 18; n >= 1; n--) p = 1.0 + p * r * INV[n];
    return ldexp(p, k);
}

double pow10(double x)
{
    // partie entiere exacte par la table, reste par exp
    if (x >= -18 && x <= 18) {
        int k = (int)floor(x + 0.5);
        double r = x - k;
        return p10(k) * (r == 0 ? 1.0 : exp(r * LN10));
    }
    return exp(x * LN10);
}

double pow(double x, double y)
{
    if (x == 0) return y > 0 ? 0.0 : inf();
    if (x < 0) return nan();
    return exp(y * ln(x));
}

// pi/2 en trois parties (fdlibm)
static const double PIO2_1 = 1.57079632673412561417e+00;
static const double PIO2_2 = 6.07710050650619224932e-11;
static const double PIO2_3 = 2.02226624879595063154e-21;

// sin r = r (1 - r^2/(2.3) (1 - r^2/(4.5) (1 - ...))) jusqu'a r^19
static double ksin(double r)
{
    double r2 = r * r, p = 1.0;
    for (int n = 18; n >= 2; n -= 2) p = 1.0 - r2 * INV[n] * INV[n + 1] * p;
    return r * p;
}

// cos r = 1 - r^2/(1.2) (1 - r^2/(3.4) (1 - ...)) jusqu'a r^20
static double kcos(double r)
{
    double r2 = r * r, p = 1.0;
    for (int n = 19; n >= 1; n -= 2) p = 1.0 - r2 * INV[n] * INV[n + 1] * p;
    return p;
}

// Reduction : x = k pi/2 + r, |r| <= pi/4. Renvoie k mod 4.
static int reduce(double x, double &r)
{
    double kf = floor(x / (PI / 2) + 0.5);
    r = ((x - kf * PIO2_1) - kf * PIO2_2) - kf * PIO2_3;
    long long k = (long long)kf;
    return (int)(k & 3);
}

double sin(double x)
{
    if (!isfinite(x)) return nan();
    double r;
    switch (reduce(x, r)) {
    case 0: return ksin(r);
    case 1: return kcos(r);
    case 2: return -ksin(r);
    default: return -kcos(r);
    }
}

double cos(double x)
{
    if (!isfinite(x)) return nan();
    double r;
    switch (reduce(x, r)) {
    case 0: return kcos(r);
    case 1: return -ksin(r);
    case 2: return -kcos(r);
    default: return ksin(r);
    }
}

double tan(double x)
{
    return sin(x) / cos(x);
}

double atan(double x)
{
    if (isnan(x)) return x;
    bool neg = x < 0;
    if (neg) x = -x;
    bool inv = x > 1.0;
    if (isinf(x)) return neg ? -PI / 2 : PI / 2;
    if (inv) x = 1.0 / x;
    // deux demi-angles : atan x = 2 atan(x / (1 + sqrt(1 + x^2)))
    x = x / (1.0 + sqrt(1.0 + x * x));
    x = x / (1.0 + sqrt(1.0 + x * x));    // |x| <= 0,199
    double x2 = x * x, p = 0;
    for (int k = 15; k >= 1; k--) p = (INV[2 * k + 1] - p) * x2;
    double r = 4.0 * (x - x * p);
    if (inv) r = PI / 2 - r;
    return neg ? -r : r;
}

double atan2(double y, double x)
{
    if (isnan(x) || isnan(y)) return nan();
    if (x == 0) {
        if (y > 0) return PI / 2;
        if (y < 0) return -PI / 2;
        return 0.0;
    }
    double a = atan(y / x);
    if (x > 0) return a;
    return y >= 0 ? a + PI : a - PI;
}

double hypot(double a, double b)
{
    a = fabs(a);
    b = fabs(b);
    if (isinf(a) || isinf(b)) return inf();
    double m = a > b ? a : b, n = a > b ? b : a;
    if (m == 0) return 0.0;
    double t = n / m;
    return m * sqrt(1.0 + t * t);
}

static const double s_p10[37] = {
    1e-18, 1e-17, 1e-16, 1e-15, 1e-14, 1e-13, 1e-12, 1e-11, 1e-10, 1e-9, 1e-8, 1e-7,
    1e-6, 1e-5, 1e-4, 1e-3, 1e-2, 1e-1, 1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8,
    1e9, 1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18,
};

double p10(int e)
{
    if (e < -18 || e > 18) return exp(e * LN10);
    return s_p10[e + 18];
}

}  // namespace fm
