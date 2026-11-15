// Fonctions mathematiques maison (double precision, sans libm).
//
// La Graph 90+E n'a pas d'unite de calcul flottant : les doubles sont
// emules par libgcc. Ces fonctions evitent toute dependance a une
// bibliotheque mathematique externe et donnent exactement les memes
// resultats sur PC (simulateur, tests) et sur la calculatrice.
// Precision verifiee contre libm : erreur relative < 1e-14 (tools/tests).
#pragma once
#include <stdint.h>

namespace fm {

constexpr double PI = 3.14159265358979323846;
constexpr double TWO_PI = 6.28318530717958647692;
constexpr double LN10 = 2.30258509299404568402;
constexpr double LN2 = 0.69314718055994530942;

inline uint64_t bits(double x)
{
    uint64_t u;
    __builtin_memcpy(&u, &x, 8);
    return u;
}
inline double fromBits(uint64_t u)
{
    double x;
    __builtin_memcpy(&x, &u, 8);
    return x;
}

inline bool isnan(double x) { return x != x; }
inline bool isinf(double x) { return (bits(x) & 0x7FFFFFFFFFFFFFFFull) == 0x7FF0000000000000ull; }
inline bool isfinite(double x) { return (bits(x) & 0x7FF0000000000000ull) != 0x7FF0000000000000ull; }
inline double nan() { return fromBits(0x7FF8000000000000ull); }
inline double inf() { return fromBits(0x7FF0000000000000ull); }
inline double fabs(double x) { return x < 0 ? -x : x; }
inline double fmin(double a, double b) { return a < b ? a : b; }
inline double fmax(double a, double b) { return a > b ? a : b; }
inline double sq(double x) { return x * x; }

// x = m * 2^e avec 0,5 <= |m| < 1 (x fini, non nul).
double frexp(double x, int &e);
double ldexp(double x, int e);
double floor(double x);
double ceil(double x);
// Arrondi a l'entier le plus proche (saturant : NaN -> 0, |resultat| <= 1e9).
int iround(double x);
// Coordonnee d'ecran sure : NaN -> 0, bornee a +/- 1e5.
inline double clampc(double x) { return !(x == x) ? 0.0 : x > 1e5 ? 1e5 : x < -1e5 ? -1e5 : x; }

double sqrt(double x);
double exp(double x);
double ln(double x);
double log10(double x);
double pow10(double x);          // 10^x
double pow(double x, double y);  // x > 0
double sin(double x);
double cos(double x);
double tan(double x);
double atan(double x);
double atan2(double y, double x);
double hypot(double a, double b);

inline double deg(double rad) { return rad * (180.0 / PI); }
inline double rad(double d) { return d * (PI / 180.0); }

// Puissances de 10 exactes de 1e-18 a 1e18.
double p10(int e);

}  // namespace fm
