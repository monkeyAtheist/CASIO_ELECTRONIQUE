#include "Calc.hpp"
#include <string.h>

CalcState &calcState(int i)
{
    CalcState &s = g_states[i];
    if (!s.ready) calcReset(i);
    return s;
}

void calcReset(int i)
{
    CalcState &s = g_states[i];
    memset(&s, 0, sizeof(s));
    for (int k = 0; k < NV; k++) s.v[k] = 0;
    if (calcDef(i).defaults) calcDef(i).defaults(s);
    s.ready = 1;
    if (calcDef(i).compute) calcDef(i).compute(s);
}

void setMsg(CalcState &s, int k, const char *m, uint8_t lvl)
{
    if (k < 0 || k >= NMSG) return;
    s.msg[k] = m;
    s.lvl[k] = lvl;
}

// --------------------------------------------------------------------------
// Impedances
// --------------------------------------------------------------------------
const char *const Z_CHOICES = "R|C|L|R+C|R\xE2\x88\xA5" "C|R+L|R\xE2\x88\xA5" "L";

bool zHasR(int t) { return t == ZT_R || t == ZT_RC_S || t == ZT_RC_P || t == ZT_RL_S || t == ZT_RL_P; }
bool zHasC(int t) { return t == ZT_C || t == ZT_RC_S || t == ZT_RC_P; }
bool zHasL(int t) { return t == ZT_L || t == ZT_RL_S || t == ZT_RL_P; }

Cx zEval(int t, double R, double C, double L, double w)
{
    Cx zr = cx(R), zc = w > 0 && C > 0 ? cx(0, -1.0 / (w * C)) : cx(0, -fm::inf());
    Cx zl = cx(0, w * L);
    switch (t) {
    case ZT_R: return zr;
    case ZT_C: return zc;
    case ZT_L: return zl;
    case ZT_RC_S: return zr + zc;
    case ZT_RC_P: return (w > 0 && C > 0) ? cpar(zr, zc) : zr;
    case ZT_RL_S: return zr + zl;
    case ZT_RL_P: return cpar(zr, zl);
    default: return zr;
    }
}

void zRows(RowList &r, const CalcState &s, int cslot, int rslot, int cslotC, int lslot,
           const char *labelZ, const char *labelR, const char *labelC, const char *labelL, int tag)
{
    int t = s.c[cslot];
    r.choice(labelZ, cslot, Z_CHOICES, tag, "Nature de l'imp\xC3\xA9" "dance (\xE2\x86\x90 \xE2\x86\x92 pour changer)");
    if (zHasR(t)) r.in(labelR, rslot, U_OHM, ST_E24, tag);
    if (zHasC(t)) r.in(labelC, cslotC, U_F, ST_E12, tag);
    if (zHasL(t)) r.in(labelL, lslot, U_H, ST_E12, tag);
}

Cx zOf(const CalcState &s, int cslot, int rslot, int cslotC, int lslot, double w)
{
    return zEval(s.c[cslot], s.v[rslot], s.v[cslotC], s.v[lslot], w);
}

double fCorner(const double *R, int nR, const double *C, int nC, const double *L, int nL)
{
    // moyenne geometrique des frequences de coupure possibles
    double sum = 0;
    int n = 0;
    for (int i = 0; i < nR; i++)
        for (int j = 0; j < nC; j++)
            if (R[i] > 0 && C[j] > 0) { sum += fm::log10(1.0 / (fm::TWO_PI * R[i] * C[j])); n++; }
    for (int i = 0; i < nR; i++)
        for (int j = 0; j < nL; j++)
            if (R[i] > 0 && L[j] > 0) { sum += fm::log10(R[i] / (fm::TWO_PI * L[j])); n++; }
    for (int i = 0; i < nL; i++)
        for (int j = 0; j < nC; j++)
            if (L[i] > 0 && C[j] > 0) { sum += fm::log10(1.0 / (fm::TWO_PI * fm::sqrt(L[i] * C[j]))); n++; }
    if (!n) return 0;
    return fm::pow10(sum / n);
}
