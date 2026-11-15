// Outils partages par les fichiers de calculateurs (calc/sec_*.cpp).
#pragma once
#include "Calc.hpp"
#include "../sch/Sch.hpp"
#include "../gfx/Canvas.hpp"
#include "../gfx/Color.hpp"
#include "../gfx/Tex.hpp"
#include "../font/Font.hpp"

namespace cu {

inline double w2pi(double f) { return fm::TWO_PI * f; }
inline double db(double m) { return 20.0 * fm::log10(m); }
inline double par2(double a, double b) { return (a > 0 && b > 0) ? a * b / (a + b) : 0.0; }
inline double clamp(double x, double a, double b) { return x < a ? a : x > b ? b : x; }

// Gain complexe -> |H|, dB, phase (degres) dans trois cases.
inline void putH(CalcState &s, Cx H, int iMag, int iDb, int iPhi)
{
    double m = cabs(H);
    s.v[iMag] = m;
    s.v[iDb] = m > 0 ? db(m) : -fm::inf();
    s.v[iPhi] = fm::deg(carg(H));
}

// Gain reel si la partie imaginaire est negligeable, sinon NaN.
inline double realGain(Cx H)
{
    double m = cabs(H);
    if (!(m > 0)) return 0;
    return fm::fabs(H.im) <= 1e-9 * m ? H.re : fm::nan();
}

// Borne d'entree ou de sortie avec fleche de tension depuis la masse (yg).
inline void termIn(Sch &g, int x, int y, int yg, const char *lab, int tag = 0)
{
    g.term(x, y);
    g.varrow(x, yg - 3, y + 4, lab, 1, tag);
}
inline void termOut(Sch &g, int x, int y, int yg, const char *lab, int tag = 0)
{
    g.term(x, y);
    g.varrow(x, yg - 3, y + 4, lab, -1, tag);
}
// Ligne de masse entre deux bornes, symbole de masse en xg.
inline void groundRail(Sch &g, int x0, int x1, int y, int xg)
{
    g.wire(x0, y, x1, y);
    g.term(x0, y);
    g.term(x1, y);
    g.dot(xg, y);
    g.gnd(xg, y);
}

// Ajoute un message selon la saturation de la sortie d'un AOP.
inline void satMsg(CalcState &s, int k, double vs, double vsat)
{
    if (!fm::isfinite(vs)) setMsg(s, k, "Sortie non définie", ML_WARN);
    else if (fm::fabs(vs) > vsat) setMsg(s, k, "Sortie saturée : |Vs| > Vsat", ML_BAD);
    else setMsg(s, k, "Régime linéaire : |Vs| < Vsat", ML_OK);
}

// Petit entier -> texte (renvoie la longueur).
inline int itoaSimple(int v, char *out)
{
    char tmp[12];
    int n = 0, k = 0;
    if (v < 0) { out[k++] = '-'; v = -v; }
    do { tmp[n++] = (char)('0' + v % 10); v /= 10; } while (v && n < 11);
    while (n) out[k++] = tmp[--n];
    out[k] = 0;
    return k;
}

}  // namespace cu
