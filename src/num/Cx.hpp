// Nombres complexes (impedances, fonctions de transfert).
#pragma once
#include "FMath.hpp"

struct Cx {
    double re, im;
};

inline Cx cx(double re, double im = 0.0) { return Cx{re, im}; }
inline Cx operator+(Cx a, Cx b) { return Cx{a.re + b.re, a.im + b.im}; }
inline Cx operator-(Cx a, Cx b) { return Cx{a.re - b.re, a.im - b.im}; }
inline Cx operator-(Cx a) { return Cx{-a.re, -a.im}; }
inline Cx operator*(Cx a, Cx b) { return Cx{a.re * b.re - a.im * b.im, a.re * b.im + a.im * b.re}; }
inline Cx operator*(double k, Cx a) { return Cx{k * a.re, k * a.im}; }
inline Cx operator*(Cx a, double k) { return Cx{k * a.re, k * a.im}; }

// Division (methode de Smith : pas de debordement intermediaire).
inline Cx operator/(Cx a, Cx b)
{
    if (fm::fabs(b.re) >= fm::fabs(b.im)) {
        if (b.re == 0) return Cx{fm::nan(), fm::nan()};
        double r = b.im / b.re, d = b.re + b.im * r;
        return Cx{(a.re + a.im * r) / d, (a.im - a.re * r) / d};
    }
    double r = b.re / b.im, d = b.re * r + b.im;
    return Cx{(a.re * r + a.im) / d, (a.im * r - a.re) / d};
}
inline Cx operator/(Cx a, double k) { return Cx{a.re / k, a.im / k}; }
inline Cx operator/(double k, Cx a) { return cx(k) / a; }

inline double cabs(Cx a) { return fm::hypot(a.re, a.im); }
inline double carg(Cx a) { return fm::atan2(a.im, a.re); }
inline bool cbad(Cx a) { return !fm::isfinite(a.re) || !fm::isfinite(a.im); }

// Association en parallele : a b / (a + b) (gere les impedances nulles).
inline Cx cpar(Cx a, Cx b)
{
    if (a.re == 0 && a.im == 0) return a;
    if (b.re == 0 && b.im == 0) return b;
    return 1.0 / (1.0 / a + 1.0 / b);
}
