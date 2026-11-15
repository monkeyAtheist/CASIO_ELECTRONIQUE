// Section : amplificateurs operationnels (AOP ideal).
#include "CalcUtil.hpp"

using namespace cu;

// Frequence caracteristique de deux impedances generales.
static double fc2(const CalcState &s, int cz1, int r1, int c1, int l1, int cz2, int r2, int c2, int l2)
{
    double R[2], C[2], L[2];
    int nr = 0, nc = 0, nl = 0;
    int t1 = s.c[cz1], t2 = s.c[cz2];
    if (zHasR(t1)) R[nr++] = s.v[r1];
    if (zHasR(t2)) R[nr++] = s.v[r2];
    if (zHasC(t1)) C[nc++] = s.v[c1];
    if (zHasC(t2)) C[nc++] = s.v[c2];
    if (zHasL(t1)) L[nl++] = s.v[l1];
    if (zHasL(t2)) L[nl++] = s.v[l2];
    return fCorner(R, nr, C, nc, L, nl);
}

static const char *const DESC_F = "Fréquence du signal sinusoïdal";
static const char *const DESC_VE = "Amplitude de l'entrée (crête ou efficace)";
static const char *const DESC_SAT = "Saturation de la sortie (≈ alimentation − 1,5 V)";

// Etiquette d'une impedance pres de son symbole.
static void zLabel(Sch &g, int x, int y, const char *lab, int type, int tag, bool above)
{
    int dy = (type == ZT_RC_P || type == ZT_RL_P) ? 13 : (type == ZT_C ? 8 : 6);
    g.tex(x, above ? y - dy - 3 : y + dy + 13, lab, tex::CENTER, tag);
}

// ==========================================================================
// Amplificateur inverseur
// ==========================================================================
namespace inv {
enum { R1, C1, L1, R2, C2, L2, F, VE, VSAT, OH, ODB, OPHI, OAV, OVS, OZE };
enum { CZ1, CZ2 };
enum { T_Z1 = 1, T_Z2, T_VE, T_VS };

static void def(CalcState &s)
{
    s.v[R1] = 10e3; s.v[C1] = 100e-9; s.v[L1] = 10e-3;
    s.v[R2] = 100e3; s.v[C2] = 1e-9; s.v[L2] = 10e-3;
    s.v[F] = 1e3; s.v[VE] = 0.5; s.v[VSAT] = 13;
    s.c[CZ1] = ZT_R; s.c[CZ2] = ZT_R;
}
static Cx H(const CalcState &s, double f)
{
    double w = w2pi(f);
    return -(zOf(s, CZ2, R2, C2, L2, w) / zOf(s, CZ1, R1, C1, L1, w));
}
static void rows(const CalcState &s, RowList &r)
{
    zRows(r, s, CZ1, R1, C1, L1, "Z_1", "R_1", "C_1", "L_1", T_Z1);
    zRows(r, s, CZ2, R2, C2, L2, "Z_2", "R_2", "C_2", "L_2", T_Z2);
    r.head("Signal d'entrée");
    r.in("f", F, U_HZ, ST_125, 0, DESC_F);
    r.in("V_e", VE, U_V, ST_LIN01, T_VE, DESC_VE, RF_NONNEG);
    r.in("V_{sat}", VSAT, U_V, ST_LIN1, 0, DESC_SAT);
    r.head("Résultats");
    r.out("A_v", OAV, U_NONE, "Gain réel −Z₂/Z₁ (défini si les impédances sont résistives)", true);
    r.out("|H|", OH, U_NONE, "Module de la fonction de transfert H = −Z₂/Z₁");
    r.out("G", ODB, U_DB, "Gain en décibels : G = 20 log |H|");
    r.out("\\varphi", OPHI, U_DEG, "Déphasage de Vs par rapport à Ve");
    r.out("V_s", OVS, U_V, "Amplitude de la sortie : Vs = |H| Ve", true, T_VS);
    r.out("|Z_e|", OZE, U_OHM, "Impédance d'entrée : Ze = Z₁");
    r.msg(0);
}
static void calc(CalcState &s)
{
    Cx h = H(s, s.v[F]);
    putH(s, h, OH, ODB, OPHI);
    s.v[OAV] = realGain(h);
    s.v[OVS] = s.v[OH] * s.v[VE];
    s.v[OZE] = cabs(zOf(s, CZ1, R1, C1, L1, w2pi(s.v[F])));
    satMsg(s, 0, s.v[OVS], s.v[VSAT]);
}
static bool tf(const CalcState &s, double f, Cx &h) { h = H(s, f); return true; }
static double fc(const CalcState &s) { return fc2(s, CZ1, R1, C1, L1, CZ2, R2, C2, L2); }
static void sch(Sch &g, const CalcState &s)
{
    const int Y = 150, ax = 94, ay = 88, w = 36, d = Sch::inDy(w);
    g.zdip(16, ay - d, 70, ay - d, s.c[CZ1], T_Z1);
    zLabel(g, 43, ay - d, "Z_1", s.c[CZ1], T_Z1, true);
    g.wire(70, ay - d, ax, ay - d);
    g.wire(80, ay - d, 80, 42, T_Z2);
    g.zdip(80, 42, 148, 42, s.c[CZ2], T_Z2);
    zLabel(g, 114, 42, "Z_2", s.c[CZ2], T_Z2, true);
    g.wire(148, 42, 148, ay, T_Z2);
    g.dot(80, ay - d);
    g.opamp(ax, ay, w, true);
    g.wire(ax + w, ay, 166, ay);
    g.dot(148, ay);
    g.wireHV(ax, ay + d, 86, Y);
    groundRail(g, 16, 166, Y, 86);
    termIn(g, 16, ay - d, Y, "V_e", T_VE);
    termOut(g, 166, ay, Y, "V_s", T_VS);
}
}  // namespace inv

// ==========================================================================
// Amplificateur non inverseur
// ==========================================================================
namespace ninv {
enum { R1, C1, L1, R2, C2, L2, F, VE, VSAT, OH, ODB, OPHI, OAV, OVS };
enum { CZ1, CZ2 };
enum { T_Z1 = 1, T_Z2, T_VE, T_VS };

static void def(CalcState &s)
{
    s.v[R1] = 10e3; s.v[C1] = 100e-9; s.v[L1] = 10e-3;
    s.v[R2] = 47e3; s.v[C2] = 1e-9; s.v[L2] = 10e-3;
    s.v[F] = 1e3; s.v[VE] = 1; s.v[VSAT] = 13;
}
static Cx H(const CalcState &s, double f)
{
    double w = w2pi(f);
    return cx(1) + zOf(s, CZ2, R2, C2, L2, w) / zOf(s, CZ1, R1, C1, L1, w);
}
static void rows(const CalcState &s, RowList &r)
{
    zRows(r, s, CZ1, R1, C1, L1, "Z_1", "R_1", "C_1", "L_1", T_Z1);
    zRows(r, s, CZ2, R2, C2, L2, "Z_2", "R_2", "C_2", "L_2", T_Z2);
    r.head("Signal d'entrée");
    r.in("f", F, U_HZ, ST_125, 0, DESC_F);
    r.in("V_e", VE, U_V, ST_LIN01, T_VE, DESC_VE, RF_NONNEG);
    r.in("V_{sat}", VSAT, U_V, ST_LIN1, 0, DESC_SAT);
    r.head("Résultats");
    r.out("A_v", OAV, U_NONE, "Gain réel 1 + Z₂/Z₁ (défini si les impédances sont résistives)", true);
    r.out("|H|", OH, U_NONE, "Module de H = 1 + Z₂/Z₁");
    r.out("G", ODB, U_DB, "Gain en décibels : G = 20 log |H|");
    r.out("\\varphi", OPHI, U_DEG, "Déphasage de Vs par rapport à Ve");
    r.out("V_s", OVS, U_V, "Amplitude de la sortie : Vs = |H| Ve", true, T_VS);
    r.msg(0);
}
static void calc(CalcState &s)
{
    Cx h = H(s, s.v[F]);
    putH(s, h, OH, ODB, OPHI);
    s.v[OAV] = realGain(h);
    s.v[OVS] = s.v[OH] * s.v[VE];
    satMsg(s, 0, s.v[OVS], s.v[VSAT]);
}
static bool tf(const CalcState &s, double f, Cx &h) { h = H(s, f); return true; }
static double fc(const CalcState &s) { return fc2(s, CZ1, R1, C1, L1, CZ2, R2, C2, L2); }
static void sch(Sch &g, const CalcState &s)
{
    const int Y = 152, ax = 80, ay = 62, w = 36, d = Sch::inDy(w);
    g.opamp(ax, ay, w, false);
    g.wire(16, ay - d, ax, ay - d);
    g.wireHV(ax, ay + d, 66, 108);
    g.dot(66, 108);
    g.zdip(66, 108, 66, Y, s.c[CZ1], T_Z1);
    g.tex(50, 136, "Z_1", tex::RIGHT, T_Z1);
    g.zdip(66, 108, 134, 108, s.c[CZ2], T_Z2);
    zLabel(g, 100, 108, "Z_2", s.c[CZ2], T_Z2, false);
    g.wire(134, 108, 134, ay, T_Z2);
    g.wire(ax + w, ay, 166, ay);
    g.dot(134, ay);
    groundRail(g, 16, 166, Y, 66);
    termIn(g, 16, ay - d, Y, "V_e", T_VE);
    termOut(g, 166, ay, Y, "V_s", T_VS);
}
}  // namespace ninv

// ==========================================================================
// Suiveur
// ==========================================================================
namespace suiv {
enum { VE, RL, VSAT, OVS, OIS, OPL };
enum { T_VE = 1, T_RL, T_VS };
static void def(CalcState &s) { s.v[VE] = 5; s.v[RL] = 1e3; s.v[VSAT] = 13; }
static void rows(const CalcState &, RowList &r)
{
    r.in("V_e", VE, U_V, ST_LIN01, T_VE, "Tension d'entrée (continue ou instantanée)", 0);
    r.in("R_L", RL, U_OHM, ST_E24, T_RL, "Résistance de charge en sortie");
    r.in("V_{sat}", VSAT, U_V, ST_LIN1, 0, DESC_SAT);
    r.head("Résultats");
    r.out("V_s", OVS, U_V, "Vs = Ve : gain unitaire", true, T_VS);
    r.out("I_s", OIS, U_A, "Courant fourni par l'AOP à la charge");
    r.out("P_L", OPL, U_W, "Puissance dans la charge");
    r.msg(0);
    r.msg(1);
}
static void calc(CalcState &s)
{
    s.v[OVS] = s.v[VE];
    s.v[OIS] = s.v[VE] / s.v[RL];
    s.v[OPL] = s.v[VE] * s.v[VE] / s.v[RL];
    satMsg(s, 0, s.v[OVS], s.v[VSAT]);
    if (fm::fabs(s.v[OIS]) > 20e-3) setMsg(s, 1, "Courant > 20 mA : trop pour un AOP courant", ML_WARN);
    else setMsg(s, 1, "Ze infinie, Zs nulle : adaptateur d'impédance", ML_INFO);
}
static void sch(Sch &g, const CalcState &)
{
    const int Y = 152, ax = 66, ay = 76, w = 36, d = Sch::inDy(w);
    g.opamp(ax, ay, w, false);
    g.wire(16, ay - d, ax, ay - d);
    g.wireHV(ax, ay + d, 56, ay + 30);
    g.wireHV(56, ay + 30, 118, ay);
    g.dot(118, ay);
    g.wire(ax + w, ay, 166, ay);
    g.dot(132, ay);
    g.res(132, ay, 132, Y, T_RL);
    g.tex(124, 132, "R_L", tex::RIGHT, T_RL);
    g.dot(132, Y);
    groundRail(g, 16, 166, Y, 92);
    termIn(g, 16, ay - d, Y, "V_e", T_VE);
    termOut(g, 166, ay, Y, "V_s", T_VS);
}
}  // namespace suiv

// ==========================================================================
// Sommateur inverseur
// ==========================================================================
namespace somme {
enum { V1, V2, V3, R1, R2, R3, RF, VSAT, OVS, OG1, OG2, OG3 };
enum { CN };
enum { T_R1 = 1, T_R2, T_R3, T_RF, T_VS };
static void def(CalcState &s)
{
    s.v[V1] = 1; s.v[V2] = 0.5; s.v[V3] = -0.2;
    s.v[R1] = 10e3; s.v[R2] = 10e3; s.v[R3] = 10e3; s.v[RF] = 22e3; s.v[VSAT] = 13;
    s.c[CN] = 1;
}
static void rows(const CalcState &s, RowList &r)
{
    bool three = s.c[CN] == 1;
    r.choice("#Entrées", CN, "2|3", 0, "Nombre d'entrées du sommateur");
    r.in("V_1", V1, U_V, ST_LIN01, T_R1, "Tension d'entrée 1 (continue ou instantanée)", 0);
    r.in("R_1", R1, U_OHM, ST_E24, T_R1);
    r.in("V_2", V2, U_V, ST_LIN01, T_R2, "Tension d'entrée 2", 0);
    r.in("R_2", R2, U_OHM, ST_E24, T_R2);
    if (three) {
        r.in("V_3", V3, U_V, ST_LIN01, T_R3, "Tension d'entrée 3", 0);
        r.in("R_3", R3, U_OHM, ST_E24, T_R3);
    }
    r.in("R_f", RF, U_OHM, ST_E24, T_RF, "Résistance de contre-réaction");
    r.in("V_{sat}", VSAT, U_V, ST_LIN1, 0, DESC_SAT);
    r.head("Résultats");
    r.out("V_s", OVS, U_V, "Vs = −Rf (V₁/R₁ + V₂/R₂ + V₃/R₃)", true, T_VS);
    r.out("-R_f/R_1", OG1, U_NONE, "Poids de l'entrée 1");
    r.out("-R_f/R_2", OG2, U_NONE, "Poids de l'entrée 2");
    if (three) r.out("-R_f/R_3", OG3, U_NONE, "Poids de l'entrée 3");
    r.msg(0);
}
static void calc(CalcState &s)
{
    bool three = s.c[CN] == 1;
    s.v[OG1] = -s.v[RF] / s.v[R1];
    s.v[OG2] = -s.v[RF] / s.v[R2];
    s.v[OG3] = -s.v[RF] / s.v[R3];
    double vs = s.v[OG1] * s.v[V1] + s.v[OG2] * s.v[V2] + (three ? s.v[OG3] * s.v[V3] : 0);
    s.v[OVS] = vs;
    satMsg(s, 0, vs, s.v[VSAT]);
    if (fm::fabs(vs) > s.v[VSAT]) s.v[OVS] = vs > 0 ? s.v[VSAT] : -s.v[VSAT];
}
static void sch(Sch &g, const CalcState &s)
{
    bool three = s.c[CN] == 1;
    const int Y = 156, ax = 104, ay = 92, w = 32, d = Sch::inDy(w);
    const int ys[3] = {50, 80, 110};
    const char *lab[3] = {"V_1", "V_2", "V_3"};
    const char *rl[3] = {"R_1", "R_2", "R_3"};
    int n = three ? 3 : 2;
    for (int i = 0; i < n; i++) {
        g.term(32, ys[i]);
        g.tex(26, ys[i] + 5, lab[i], tex::RIGHT, T_R1 + i);
        g.res(32, ys[i], 82, ys[i], T_R1 + i);
        g.tex(57, ys[i] - 7, rl[i], tex::CENTER, T_R1 + i);
        if (i) g.dot(82, ys[i]);
    }
    g.wire(82, ys[0], 82, ys[n - 1]);
    g.wire(82, ay - d, ax, ay - d);
    g.dot(82, ay - d);
    g.wire(82, ys[0], 82, 26, T_RF);
    g.res(82, 26, 150, 26, T_RF);
    g.tex(116, 18, "R_f", tex::CENTER, T_RF);
    g.wire(150, 26, 150, ay, T_RF);
    g.opamp(ax, ay, w, true);
    g.wire(ax + w, ay, 166, ay);
    g.dot(150, ay);
    g.wireHV(ax, ay + d, 96, Y);
    g.wire(96, Y, 166, Y);
    g.dot(96, Y);
    g.gnd(96, Y);
    g.term(166, Y);
    termOut(g, 166, ay, Y, "V_s", T_VS);
}
}  // namespace somme

// ==========================================================================
// Soustracteur (amplificateur differentiel)
// ==========================================================================
namespace diff {
enum { V1, V2, R1, R2, R3, R4, VSAT, OVS, OAD, OACM, OCMRR };
enum { T_R1 = 1, T_R2, T_R3, T_R4, T_V1, T_V2, T_VS };
static void def(CalcState &s)
{
    s.v[V1] = 1; s.v[V2] = 1.5;
    s.v[R1] = 10e3; s.v[R2] = 47e3; s.v[R3] = 10e3; s.v[R4] = 47e3; s.v[VSAT] = 13;
}
static void rows(const CalcState &, RowList &r)
{
    r.in("V_1", V1, U_V, ST_LIN01, T_V1, "Tension appliquée sur R₁ (entrée −)", 0);
    r.in("V_2", V2, U_V, ST_LIN01, T_V2, "Tension appliquée sur R₃ (entrée +)", 0);
    r.in("R_1", R1, U_OHM, ST_E24, T_R1);
    r.in("R_2", R2, U_OHM, ST_E24, T_R2, "Résistance de contre-réaction");
    r.in("R_3", R3, U_OHM, ST_E24, T_R3);
    r.in("R_4", R4, U_OHM, ST_E24, T_R4, "Résistance de l'entrée + vers la masse");
    r.in("V_{sat}", VSAT, U_V, ST_LIN1, 0, DESC_SAT);
    r.head("Résultats");
    r.out("V_s", OVS, U_V, "Vs = R₄/(R₃+R₄)·(1+R₂/R₁)·V₂ − R₂/R₁·V₁", true, T_VS);
    r.out("A_d", OAD, U_NONE, "Gain différentiel (sur V₂ − V₁)");
    r.out("A_{mc}", OACM, U_NONE, "Gain de mode commun (nul si R₃/R₄ = R₁/R₂)");
    r.out("#TRMC", OCMRR, U_DB, "Taux de réjection du mode commun 20 log |Ad/Amc|");
    r.msg(0);
}
static void calc(CalcState &s)
{
    double a2 = s.v[R4] / (s.v[R3] + s.v[R4]) * (1 + s.v[R2] / s.v[R1]);
    double a1 = s.v[R2] / s.v[R1];
    double vs = a2 * s.v[V2] - a1 * s.v[V1];
    s.v[OAD] = (a1 + a2) / 2;
    s.v[OACM] = a2 - a1;
    s.v[OCMRR] = fm::fabs(s.v[OACM]) < 1e-12 * a1 ? fm::inf() : db(fm::fabs(s.v[OAD] / s.v[OACM]));
    s.v[OVS] = vs;
    satMsg(s, 0, vs, s.v[VSAT]);
    if (fm::fabs(vs) > s.v[VSAT]) s.v[OVS] = vs > 0 ? s.v[VSAT] : -s.v[VSAT];
}
static void sch(Sch &g, const CalcState &)
{
    const int Y = 156, ax = 98, ay = 80, w = 34, d = Sch::inDy(w);
    g.term(32, ay - d);
    g.tex(26, ay - d + 5, "V_1", tex::RIGHT, T_V1);
    g.res(32, ay - d, 78, ay - d, T_R1);
    g.tex(53, ay - d - 7, "R_1", tex::CENTER, T_R1);
    g.wire(78, ay - d, ax, ay - d);
    g.dot(84, ay - d);
    g.wire(84, ay - d, 84, 30, T_R2);
    g.res(84, 30, 150, 30, T_R2);
    g.tex(117, 22, "R_2", tex::CENTER, T_R2);
    g.wire(150, 30, 150, ay, T_R2);
    g.opamp(ax, ay, w, true);
    g.wire(ax + w, ay, 166, ay);
    g.dot(150, ay);
    g.term(32, 118);
    g.tex(26, 123, "V_2", tex::RIGHT, T_V2);
    g.res(32, 118, 76, 118, T_R3);
    g.tex(52, 111, "R_3", tex::CENTER, T_R3);
    g.wire(76, 118, 88, 118);
    g.wire(88, 118, 88, ay + d);
    g.wire(88, ay + d, ax, ay + d);
    g.dot(88, 118);
    g.res(88, 118, 88, Y, T_R4);
    g.tex(98, 142, "R_4", tex::LEFT, T_R4);
    g.wire(88, Y, 166, Y);
    g.dot(88, Y);
    g.gnd(88, Y);
    g.term(166, Y);
    termOut(g, 166, ay, Y, "V_s", T_VS);
}
}  // namespace diff

// ==========================================================================
// Integrateur
// ==========================================================================
namespace integ {
enum { R1, C, R2, F, VE, VSAT, OTAU, OF0, OG0, OFC, OH, ODB, OPHI, OVS };
enum { CR2 };
enum { T_R1 = 1, T_C, T_R2, T_VE, T_VS };
static void def(CalcState &s)
{
    s.v[R1] = 10e3; s.v[C] = 100e-9; s.v[R2] = 1e6; s.v[F] = 1e3; s.v[VE] = 1; s.v[VSAT] = 13;
    s.c[CR2] = 1;
}
static Cx H(const CalcState &s, double f)
{
    double w = w2pi(f);
    Cx zc = cx(0, -1.0 / (w * s.v[C]));
    Cx z2 = s.c[CR2] ? cpar(cx(s.v[R2]), zc) : zc;
    return -(z2 / cx(s.v[R1]));
}
static void rows(const CalcState &s, RowList &r)
{
    r.in("R_1", R1, U_OHM, ST_E24, T_R1, "Résistance d'entrée");
    r.in("C", C, U_F, ST_E12, T_C, "Condensateur de contre-réaction");
    r.choice("R_2", CR2, "sans|en parallèle sur C", T_R2, "Résistance de fuite : limite le gain en continu");
    if (s.c[CR2]) r.in("R_2", R2, U_OHM, ST_E24, T_R2, "Résistance de fuite en parallèle sur C");
    r.head("Signal d'entrée");
    r.in("f", F, U_HZ, ST_125, 0, DESC_F);
    r.in("V_e", VE, U_V, ST_LIN01, T_VE, DESC_VE, RF_NONNEG);
    r.in("V_{sat}", VSAT, U_V, ST_LIN1, 0, DESC_SAT);
    r.head("Résultats");
    r.out("\\tau", OTAU, U_S, "Constante d'intégration τ = R₁C : vs = −(1/τ)∫ve dt");
    r.out("f_0", OF0, U_HZ, "Fréquence de gain unité : 1/(2π R₁C)");
    if (s.c[CR2]) {
        r.out("A_0", OG0, U_NONE, "Gain en continu : −R₂/R₁");
        r.out("f_c", OFC, U_HZ, "Coupure basse : intégration au-dessus de 1/(2π R₂C)");
    }
    r.out("|H|", OH, U_NONE, "Module du gain à la fréquence f", true);
    r.out("G", ODB, U_DB);
    r.out("\\varphi", OPHI, U_DEG, "Déphasage (−90° pour un intégrateur pur)");
    r.out("V_s", OVS, U_V, "Amplitude de la sortie", true, T_VS);
    r.msg(0);
}
static void calc(CalcState &s)
{
    s.v[OTAU] = s.v[R1] * s.v[C];
    s.v[OF0] = 1.0 / (fm::TWO_PI * s.v[OTAU]);
    s.v[OG0] = -s.v[R2] / s.v[R1];
    s.v[OFC] = 1.0 / (fm::TWO_PI * s.v[R2] * s.v[C]);
    Cx h = H(s, s.v[F]);
    putH(s, h, OH, ODB, OPHI);
    s.v[OVS] = s.v[OH] * s.v[VE];
    satMsg(s, 0, s.v[OVS], s.v[VSAT]);
}
static bool tf(const CalcState &s, double f, Cx &h) { h = H(s, f); return true; }
static double fc(const CalcState &s) { return s.c[CR2] ? fm::sqrt(1.0 / (fm::TWO_PI * s.v[R1] * s.v[C]) / (fm::TWO_PI * s.v[R2] * s.v[C])) : 1.0 / (fm::TWO_PI * s.v[R1] * s.v[C]); }
static void sch(Sch &g, const CalcState &s)
{
    const int Y = 150, ax = 94, ay = 92, w = 36, d = Sch::inDy(w);
    g.res(16, ay - d, 70, ay - d, T_R1);
    g.tex(43, ay - d - 7, "R_1", tex::CENTER, T_R1);
    g.wire(70, ay - d, ax, ay - d);
    g.dot(80, ay - d);
    g.wire(80, ay - d, 80, 56);
    g.cap(80, 56, 148, 56, T_C);
    g.tex(114, 47, "C", tex::CENTER, T_C);
    if (s.c[CR2]) {
        g.dot(80, 56);
        g.dot(148, 56);
        g.wire(80, 56, 80, 26, T_R2);
        g.res(80, 26, 148, 26, T_R2);
        g.wire(148, 26, 148, 56, T_R2);
        g.tex(114, 18, "R_2", tex::CENTER, T_R2);
    }
    g.wire(148, 56, 148, ay);
    g.opamp(ax, ay, w, true);
    g.wire(ax + w, ay, 166, ay);
    g.dot(148, ay);
    g.wireHV(ax, ay + d, 86, Y);
    groundRail(g, 16, 166, Y, 86);
    termIn(g, 16, ay - d, Y, "V_e", T_VE);
    termOut(g, 166, ay, Y, "V_s", T_VS);
}
}  // namespace integ

// ==========================================================================
// Derivateur
// ==========================================================================
namespace deriv {
enum { C1, R1, R2, F, VE, VSAT, OTAU, OF0, OGI, OFC, OH, ODB, OPHI, OVS };
enum { CR1 };
enum { T_C1 = 1, T_R1, T_R2, T_VE, T_VS };
static void def(CalcState &s)
{
    s.v[C1] = 100e-9; s.v[R1] = 1e3; s.v[R2] = 10e3; s.v[F] = 100; s.v[VE] = 1; s.v[VSAT] = 13;
    s.c[CR1] = 1;
}
static Cx H(const CalcState &s, double f)
{
    double w = w2pi(f);
    Cx z1 = cx(0, -1.0 / (w * s.v[C1]));
    if (s.c[CR1]) z1 = z1 + cx(s.v[R1]);
    return -(cx(s.v[R2]) / z1);
}
static void rows(const CalcState &s, RowList &r)
{
    r.in("C_1", C1, U_F, ST_E12, T_C1, "Condensateur d'entrée");
    r.choice("R_1", CR1, "sans|en série avec C₁", T_R1, "Résistance série : limite le gain aux hautes fréquences");
    if (s.c[CR1]) r.in("R_1", R1, U_OHM, ST_E24, T_R1, "Résistance en série avec C₁");
    r.in("R_2", R2, U_OHM, ST_E24, T_R2, "Résistance de contre-réaction");
    r.head("Signal d'entrée");
    r.in("f", F, U_HZ, ST_125, 0, DESC_F);
    r.in("V_e", VE, U_V, ST_LIN01, T_VE, DESC_VE, RF_NONNEG);
    r.in("V_{sat}", VSAT, U_V, ST_LIN1, 0, DESC_SAT);
    r.head("Résultats");
    r.out("\\tau", OTAU, U_S, "τ = R₂C₁ : vs = −τ dve/dt");
    r.out("f_0", OF0, U_HZ, "Fréquence de gain unité : 1/(2π R₂C₁)");
    if (s.c[CR1]) {
        r.out("A_\\infty", OGI, U_NONE, "Gain aux hautes fréquences : −R₂/R₁");
        r.out("f_c", OFC, U_HZ, "Dérivation au-dessous de 1/(2π R₁C₁)");
    }
    r.out("|H|", OH, U_NONE, "Module du gain à la fréquence f", true);
    r.out("G", ODB, U_DB);
    r.out("\\varphi", OPHI, U_DEG, "Déphasage (+90° pour un dérivateur pur)");
    r.out("V_s", OVS, U_V, "Amplitude de la sortie", true, T_VS);
    r.msg(0);
}
static void calc(CalcState &s)
{
    s.v[OTAU] = s.v[R2] * s.v[C1];
    s.v[OF0] = 1.0 / (fm::TWO_PI * s.v[OTAU]);
    s.v[OGI] = -s.v[R2] / s.v[R1];
    s.v[OFC] = 1.0 / (fm::TWO_PI * s.v[R1] * s.v[C1]);
    Cx h = H(s, s.v[F]);
    putH(s, h, OH, ODB, OPHI);
    s.v[OVS] = s.v[OH] * s.v[VE];
    satMsg(s, 0, s.v[OVS], s.v[VSAT]);
}
static bool tf(const CalcState &s, double f, Cx &h) { h = H(s, f); return true; }
static double fc(const CalcState &s) { return 1.0 / (fm::TWO_PI * s.v[R2] * s.v[C1]); }
static void sch(Sch &g, const CalcState &s)
{
    const int Y = 150, ax = 94, ay = 92, w = 36, d = Sch::inDy(w);
    if (s.c[CR1]) {
        g.res(16, ay - d, 44, ay - d, T_R1);
        g.tex(30, ay - d - 7, "R_1", tex::CENTER, T_R1);
        g.cap(44, ay - d, 70, ay - d, T_C1);
        g.tex(57, ay - d - 9, "C_1", tex::CENTER, T_C1);
    } else {
        g.cap(16, ay - d, 70, ay - d, T_C1);
        g.tex(43, ay - d - 9, "C_1", tex::CENTER, T_C1);
    }
    g.wire(70, ay - d, ax, ay - d);
    g.dot(80, ay - d);
    g.wire(80, ay - d, 80, 46, T_R2);
    g.res(80, 46, 148, 46, T_R2);
    g.tex(114, 38, "R_2", tex::CENTER, T_R2);
    g.wire(148, 46, 148, ay, T_R2);
    g.opamp(ax, ay, w, true);
    g.wire(ax + w, ay, 166, ay);
    g.dot(148, ay);
    g.wireHV(ax, ay + d, 86, Y);
    groundRail(g, 16, 166, Y, 86);
    termIn(g, 16, ay - d, Y, "V_e", T_VE);
    termOut(g, 166, ay, Y, "V_s", T_VS);
}
}  // namespace deriv

// ==========================================================================
// Triggers de Schmitt
// ==========================================================================
namespace schmitt {
enum { R1, R2, VREF, VH, VL, OTH, OTL, ODV, OVC, OBETA };
enum { T_R1 = 1, T_R2, T_VREF, T_VE, T_VS };

static void def(CalcState &s)
{
    s.v[R1] = 10e3; s.v[R2] = 47e3; s.v[VREF] = 0; s.v[VH] = 13; s.v[VL] = -13;
}
static void rowsCommon(RowList &r, bool inv)
{
    r.in("R_1", R1, U_OHM, ST_E24, T_R1, inv ? "Entre l'entrée + et Vref" : "Entre l'entrée Ve et l'entrée +");
    r.in("R_2", R2, U_OHM, ST_E24, T_R2, "Réaction positive : entre l'entrée + et la sortie");
    r.in("V_{ref}", VREF, U_V, ST_LIN01, T_VREF, inv ? "Tension de référence (bas de R₁)" : "Tension de l'entrée −", 0);
    r.in("V_{sat}^{+}", VH, U_V, ST_LIN1, T_VS, "Niveau haut de la sortie", 0);
    r.in("V_{sat}^{-}", VL, U_V, ST_LIN1, T_VS, "Niveau bas de la sortie (négatif)", 0);
    r.head("Résultats");
    r.out("V_{T+}", OTH, U_V, "Seuil haut : basculement quand Ve monte", true);
    r.out("V_{T-}", OTL, U_V, "Seuil bas : basculement quand Ve descend", true);
    r.out("\\Delta V", ODV, U_V, "Largeur du cycle d'hystérésis VT+ − VT−");
    r.out("V_{centre}", OVC, U_V, "Milieu du cycle (VT+ + VT−)/2");
    r.out("#Réaction", OBETA, U_NONE, inv ? "β = R₁/(R₁+R₂)" : "R₁/R₂");
    r.msg(0);
}
static void checkLevels(CalcState &s)
{
    if (s.v[VH] <= s.v[VL]) setMsg(s, 0, "Il faut Vsat+ > Vsat−", ML_BAD);
    else setMsg(s, 0, "Graphe : cycle d'hystérésis Vs(Ve)", ML_INFO);
}
// Inverseur : Ve sur l'entree -, R1 entre + et Vref, R2 entre + et la sortie.
static void rowsInv(const CalcState &, RowList &r) { rowsCommon(r, true); }
static void calcInv(CalcState &s)
{
    double r1 = s.v[R1], r2 = s.v[R2];
    s.v[OTH] = (s.v[VREF] * r2 + s.v[VH] * r1) / (r1 + r2);
    s.v[OTL] = (s.v[VREF] * r2 + s.v[VL] * r1) / (r1 + r2);
    s.v[ODV] = s.v[OTH] - s.v[OTL];
    s.v[OVC] = (s.v[OTH] + s.v[OTL]) / 2;
    s.v[OBETA] = r1 / (r1 + r2);
    checkLevels(s);
}
// Non inverseur : Ve par R1 sur l'entree +, R2 entre + et la sortie, Vref sur -.
static void rowsNinv(const CalcState &, RowList &r) { rowsCommon(r, false); }
static void calcNinv(CalcState &s)
{
    double r1 = s.v[R1], r2 = s.v[R2];
    s.v[OTH] = (s.v[VREF] * (r1 + r2) - s.v[VL] * r1) / r2;
    s.v[OTL] = (s.v[VREF] * (r1 + r2) - s.v[VH] * r1) / r2;
    s.v[ODV] = s.v[OTH] - s.v[OTL];
    s.v[OVC] = (s.v[OTH] + s.v[OTL]) / 2;
    s.v[OBETA] = r1 / r2;
    checkLevels(s);
}
static void schInv(Sch &g, const CalcState &)
{
    const int Y = 152, ax = 92, ay = 60, w = 36, d = Sch::inDy(w), xn = 84;
    g.wire(16, ay - d, ax, ay - d);
    g.opamp(ax, ay, w, true);
    g.wireHV(ax, ay + d, xn, 98);
    g.dot(xn, 98);
    g.res(xn, 98, 140, 98, T_R2);
    g.tex(112, 91, "R_2", tex::CENTER, T_R2);
    g.wire(140, 98, 140, ay, T_R2);
    g.res(xn, 98, xn, 122, T_R1);
    g.tex(xn + 8, 116, "R_1", tex::LEFT, T_R1);
    g.vsrc(xn, 122, Y, 0, T_VREF);
    g.tex(xn + 12, 143, "V_{ref}", tex::LEFT, T_VREF);
    g.wire(ax + w, ay, 166, ay);
    g.dot(140, ay);
    groundRail(g, 16, 166, Y, xn);
    termIn(g, 16, ay - d, Y, "V_e", T_VE);
    termOut(g, 166, ay, Y, "V_s", T_VS);
}
static void schNinv(Sch &g, const CalcState &)
{
    const int Y = 152, ax = 92, ay = 78, w = 36, d = Sch::inDy(w);
    g.res(16, ay - d, 70, ay - d, T_R1);
    g.tex(43, ay - d - 7, "R_1", tex::CENTER, T_R1);
    g.wire(70, ay - d, ax, ay - d);
    g.dot(78, ay - d);
    g.wire(78, ay - d, 78, 34, T_R2);
    g.res(78, 34, 150, 34, T_R2);
    g.tex(114, 26, "R_2", tex::CENTER, T_R2);
    g.wire(150, 34, 150, ay, T_R2);
    g.opamp(ax, ay, w, false);
    g.wireHV(ax, ay + d, 84, 114);
    g.vsrc(84, 114, Y, 0, T_VREF);
    g.tex(96, 138, "V_{ref}", tex::LEFT, T_VREF);
    g.wire(ax + w, ay, 166, ay);
    g.dot(150, ay);
    groundRail(g, 16, 166, Y, 84);
    termIn(g, 16, ay - d, Y, "V_e", T_VE);
    termOut(g, 166, ay, Y, "V_s", T_VS);
}

// Cycle d'hysteresis Vs(Ve).
static void drawCycle(Canvas &cv, const CalcState &s, const GraphArea &a, uint16_t accent, bool inv)
{
    double th = s.v[OTH], tl = s.v[OTL], vh = s.v[VH], vl = s.v[VL];
    if (!(vh > vl) || !fm::isfinite(th) || !fm::isfinite(tl)) {
        font::drawText(cv, SANS_14, a.x + 10, a.y + 30, "Niveaux de sortie incohérents", col::red);
        return;
    }
    double lo = fm::fmin(tl, th), hi = fm::fmax(tl, th);
    double span = fm::fmax(hi - lo, fm::fmax(fm::fabs(vh), fm::fabs(vl)) * 0.3);
    double xmin = lo - span * 0.8, xmax = hi + span * 0.8;
    if (xmin > -0.1 * span) xmin = -0.1 * span;
    if (xmax < 0.1 * span) xmax = 0.1 * span;
    double ymin = fm::fmin(vl, 0) - (vh - vl) * 0.12, ymax = fm::fmax(vh, 0) + (vh - vl) * 0.12;
    int X0 = a.x + 48, X1 = a.x + a.w - 14, Y0 = a.y + 28, Y1 = a.y + a.h - 22;
    auto px = [&](double v) { return X0 + fm::iround((v - xmin) / (xmax - xmin) * (X1 - X0)); };
    auto py = [&](double v) { return Y1 - fm::iround((v - ymin) / (ymax - ymin) * (Y1 - Y0)); };
    uint16_t axc = rgb(150, 156, 170);
    int x0 = px(0), y0 = py(0);
    cv.fillRect(X0, y0, X1 - X0 + 1, 1, axc);
    cv.fillRect(x0, Y0, 1, Y1 - Y0 + 1, axc);
    cv.arrowHead(X1 + 4, y0, 1, 0, 5, axc);
    cv.arrowHead(x0, Y0 - 4, 0, -1, 5, axc);
    tex::draw(cv, "V_e", X1 - 2, y0 - 6, col::inkSoft, tex::RIGHT, 1);
    tex::draw(cv, "V_s", x0 - 6, Y0 + 8, col::inkSoft, tex::RIGHT, 1);
    int xh = px(th), xl = px(tl), yh = py(vh), yl = py(vl);
    for (int y = Y0; y <= Y1; y += 3) {
        cv.fillRect(xh, y, 1, 1, axc);
        cv.fillRect(xl, y, 1, 1, axc);
    }
    for (int x = X0; x <= X1; x += 3) {
        cv.fillRect(x, yh, 1, 1, axc);
        cv.fillRect(x, yl, 1, 1, axc);
    }
    uint16_t c = accent;
    int lowL = inv ? yh : yl, lowR = inv ? yl : yh;   // niveau a gauche / a droite
    cv.fillRect(X0, lowL - 1, xh - X0 + 1, 3, c);
    cv.fillRect(xl, lowR - 1, X1 - xl + 1, 3, c);
    cv.fillRect(xh - 1, yh, 3, yl - yh, c);
    cv.fillRect(xl - 1, yh, 3, yl - yh, c);
    int dyh = inv ? 1 : -1;
    cv.arrowHead(xh, (yh + yl) / 2 + 6 * dyh, 0, dyh, 7, c);
    cv.arrowHead(xl, (yh + yl) / 2 - 6 * dyh, 0, -dyh, 7, c);
    cv.arrowHead((X0 + xh) / 2 + 4, lowL, 1, 0, 7, c);
    cv.arrowHead((xl + X1) / 2 - 4, lowR, -1, 0, 7, c);
    char buf[64];
    int k = scopy(buf, sizeof buf, "V_T+ = ");
    k += fmtValue(buf + k, (int)sizeof(buf) - k, th, U_V);
    k += scopy(buf + k, (int)sizeof(buf) - k, "    V_T\xE2\x88\x92 = ");
    k += fmtValue(buf + k, (int)sizeof(buf) - k, tl, U_V);
    k += scopy(buf + k, (int)sizeof(buf) - k, "    \xCE\x94V = ");
    fmtValue(buf + k, (int)sizeof(buf) - k, th - tl, U_V);
    font::drawText(cv, BOLD_12, a.x + 8, a.y + 14, buf, c);
    tex::draw(cv, "V_{T+}", xh + 4, Y1 + 14, col::inkSoft, tex::LEFT, 2);
    tex::draw(cv, "V_{T-}", xl - 4, Y1 + 14, col::inkSoft, tex::RIGHT, 2);
    fmtValue(buf, sizeof buf, vh, U_V);
    font::drawText(cv, SANS_12, X0 - 4 - font::width(SANS_12, buf), yh + 4, buf, col::inkSoft);
    fmtValue(buf, sizeof buf, vl, U_V);
    font::drawText(cv, SANS_12, X0 - 4 - font::width(SANS_12, buf), yl + 4, buf, col::inkSoft);
}
static void cycleInv(Canvas &cv, const CalcState &s, const GraphArea &a, uint16_t c) { drawCycle(cv, s, a, c, true); }
static void cycleNinv(Canvas &cv, const CalcState &s, const GraphArea &a, uint16_t c) { drawCycle(cv, s, a, c, false); }
}  // namespace schmitt

// ==========================================================================
// Amplificateur d'instrumentation (3 AOP)
// ==========================================================================
namespace instr {
enum { V1, V2, RG, R1, R2, R3, VSAT, OG1, OG2, OG, ODB, OVD, OVCM, OVA1, OVA2, OVS };
enum { T_RG = 1, T_R1, T_R2, T_R3, T_V1, T_V2, T_VS };
static void def(CalcState &s)
{
    s.v[V1] = 2.00; s.v[V2] = 2.01; s.v[RG] = 1e3; s.v[R1] = 24.5e3; s.v[R2] = 10e3; s.v[R3] = 20e3; s.v[VSAT] = 13;
}
static void rows(const CalcState &, RowList &r)
{
    r.in("V_1", V1, U_V, ST_LIN01, T_V1, "Entrée + de l'AOP du haut", 0);
    r.in("V_2", V2, U_V, ST_LIN01, T_V2, "Entrée + de l'AOP du bas", 0);
    r.in("R_G", RG, U_OHM, ST_E24, T_RG, "Résistance de réglage du gain");
    r.in("R_1", R1, U_OHM, ST_E24, T_R1, "Contre-réaction des deux AOP d'entrée (identiques)");
    r.in("R_2", R2, U_OHM, ST_E24, T_R2, "Résistances d'entrée du soustracteur");
    r.in("R_3", R3, U_OHM, ST_E24, T_R3, "Résistances de sortie du soustracteur");
    r.in("V_{sat}", VSAT, U_V, ST_LIN1, 0, DESC_SAT);
    r.head("Résultats");
    r.out("G", OG, U_NONE, "G = (1 + 2R₁/R_G)·R₃/R₂", true);
    r.out("G_{\\text{dB}}", ODB, U_DB);
    r.out("G_1", OG1, U_NONE, "Gain de l'étage d'entrée : 1 + 2R₁/R_G");
    r.out("G_2", OG2, U_NONE, "Gain du soustracteur : R₃/R₂");
    r.out("V_d", OVD, U_V, "Tension différentielle V₂ − V₁");
    r.out("V_{mc}", OVCM, U_V, "Tension de mode commun (V₁+V₂)/2");
    r.out("V_{A1}", OVA1, U_V, "Sortie de l'AOP du haut");
    r.out("V_{A2}", OVA2, U_V, "Sortie de l'AOP du bas");
    r.out("V_s", OVS, U_V, "Vs = G (V₂ − V₁)", true, T_VS);
    r.msg(0);
}
static void calc(CalcState &s)
{
    double k = s.v[R1] / s.v[RG];
    s.v[OG1] = 1 + 2 * k;
    s.v[OG2] = s.v[R3] / s.v[R2];
    s.v[OG] = s.v[OG1] * s.v[OG2];
    s.v[ODB] = db(fm::fabs(s.v[OG]));
    s.v[OVD] = s.v[V2] - s.v[V1];
    s.v[OVCM] = (s.v[V1] + s.v[V2]) / 2;
    s.v[OVA1] = s.v[V1] - k * s.v[OVD];
    s.v[OVA2] = s.v[V2] + k * s.v[OVD];
    s.v[OVS] = s.v[OG] * s.v[OVD];
    double vs = s.v[VSAT];
    if (fm::fabs(s.v[OVA1]) > vs || fm::fabs(s.v[OVA2]) > vs) setMsg(s, 0, "Étage d'entrée saturé", ML_BAD);
    else satMsg(s, 0, s.v[OVS], vs);
}
static void sch(Sch &g, const CalcState &)
{
    const int w = 24, d = Sch::inDy(w);
    const int ax = 32, ay1 = 32, ay2 = 140, bx = 118, by = 86;
    g.opamp(ax, ay1, w, false);          // + en haut (V1), - en bas
    g.opamp(ax, ay2, w, true);           // - en haut, + en bas (V2)
    g.wire(4, ay1 - d, ax, ay1 - d, T_V1);
    g.texSmall(5, ay1 - d - 5, "V_1", tex::LEFT, T_V1);
    g.wire(4, ay2 + d, ax, ay2 + d, T_V2);
    g.texSmall(5, ay2 + d + 14, "V_2", tex::LEFT, T_V2);
    // reseau R1 - RG - R1
    const int xp = 16, yp = 62, yq = 110, xr = 64;
    g.wireHV(ax, ay1 + d, xp, yp);
    g.wireHV(ax, ay2 - d, xp, yq);
    g.res(xp, yp, xp, yq, T_RG);
    g.texSmall(24, 90, "R_G", tex::LEFT, T_RG);
    g.dot(xp, yp);
    g.dot(xp, yq);
    g.res(xp, yp, xr, yp, T_R1);
    g.res(xp, yq, xr, yq, T_R1);
    g.texSmall(40, yp - 6, "R_1", tex::CENTER, T_R1);
    g.texSmall(40, yq + 14, "R_1", tex::CENTER, T_R1);
    g.wire(xr, yp, xr, ay1);
    g.wire(xr, yq, xr, ay2);
    g.wire(ax + w, ay1, 76, ay1);
    g.wire(ax + w, ay2, 76, ay2);
    g.dot(xr, ay1);
    g.dot(xr, ay2);
    // soustracteur
    g.wire(76, ay1, 76, by - d);
    g.wire(76, ay2, 76, by + d);
    g.res(76, by - d, 108, by - d, T_R2);
    g.res(76, by + d, 108, by + d, T_R2);
    g.wire(108, by - d, bx, by - d);
    g.wire(108, by + d, bx, by + d);
    g.texSmall(92, by - d - 6, "R_2", tex::CENTER, T_R2);
    g.texSmall(92, by + d + 14, "R_2", tex::CENTER, T_R2);
    g.dot(110, by - d);
    g.dot(110, by + d);
    g.wire(110, by - d, 110, 52, T_R3);
    g.res(110, 52, 156, 52, T_R3);
    g.texSmall(133, 46, "R_3", tex::CENTER, T_R3);
    g.wire(156, 52, 156, by, T_R3);
    g.res(110, by + d, 110, 150, T_R3);
    g.texSmall(117, 132, "R_3", tex::LEFT, T_R3);
    g.gnd(110, 150);
    g.opamp(bx, by, 26, true);
    g.wire(bx + 26, by, 170, by);
    g.dot(156, by);
    g.term(170, by);
    g.tex(174, by + 20, "V_s", tex::RIGHT, T_VS);
}
}  // namespace instr

// ==========================================================================
// Convertisseur courant-tension (transimpedance)
// ==========================================================================
namespace iv {
enum { IE, R, C, VSAT, OVS, OFC, OTR };
enum { CC };
enum { T_I = 1, T_R, T_C, T_VS };
static void def(CalcState &s) { s.v[IE] = 100e-6; s.v[R] = 10e3; s.v[C] = 100e-12; s.v[VSAT] = 13; }
static void rows(const CalcState &s, RowList &r)
{
    r.in("I_e", IE, U_A, ST_125, T_I, "Courant d'entrée (photodiode, capteur…)", 0);
    r.in("R", R, U_OHM, ST_E24, T_R, "Résistance de contre-réaction (transimpédance)");
    r.choice("C", CC, "sans|en parallèle sur R", T_C, "Condensateur de stabilité / de filtrage");
    if (s.c[CC]) r.in("C", C, U_F, ST_E12, T_C);
    r.in("V_{sat}", VSAT, U_V, ST_LIN1, 0, DESC_SAT);
    r.head("Résultats");
    r.out("V_s", OVS, U_V, "Vs = −R Ie (en continu)", true, T_VS);
    r.out("R_T", OTR, U_OHM, "Transimpédance Vs/Ie = −R");
    if (s.c[CC]) r.out("f_c", OFC, U_HZ, "Bande passante : 1/(2π RC)");
    r.msg(0);
}
static void calc(CalcState &s)
{
    s.v[OVS] = -s.v[R] * s.v[IE];
    s.v[OTR] = -s.v[R];
    s.v[OFC] = 1.0 / (fm::TWO_PI * s.v[R] * s.v[C]);
    satMsg(s, 0, s.v[OVS], s.v[VSAT]);
}
static void sch(Sch &g, const CalcState &s)
{
    const int Y = 150, ax = 94, ay = 92, w = 36, d = Sch::inDy(w);
    g.vsrc(30, Y, ay - d, 2, T_I);
    g.tex(42, 125, "I_e", tex::LEFT, T_I);
    g.wire(30, ay - d, ax, ay - d);
    g.dot(80, ay - d);
    g.wire(80, ay - d, 80, 56, T_R);
    g.res(80, 56, 148, 56, T_R);
    g.tex(114, 48, "R", tex::CENTER, T_R);
    if (s.c[CC]) {
        g.dot(80, 56);
        g.dot(148, 56);
        g.wire(80, 56, 80, 28, T_C);
        g.cap(80, 28, 148, 28, T_C);
        g.wire(148, 28, 148, 56, T_C);
        g.tex(114, 18, "C", tex::CENTER, T_C);
    }
    g.wire(148, 56, 148, ay);
    g.opamp(ax, ay, w, true);
    g.wire(ax + w, ay, 166, ay);
    g.dot(148, ay);
    g.wireHV(ax, ay + d, 86, Y);
    g.wire(30, Y, 166, Y);
    g.dot(86, Y);
    g.gnd(86, Y);
    g.term(166, Y);
    termOut(g, 166, ay, Y, "V_s", T_VS);
}
}  // namespace iv

// ==========================================================================
extern const CalcDef g_calcAop[] = {
    {"Amplificateur inverseur", "H=-Z_2/Z_1", "inverseur gain negatif impedance", 3, G_BODE, 0,
     inv::def, inv::rows, inv::calc, inv::sch, inv::tf, inv::fc, nullptr, nullptr, inv::F + 1},
    {"Amplificateur non inverseur", "H=1+Z_2/Z_1", "non inverseur gain positif", 3, G_BODE, 0,
     ninv::def, ninv::rows, ninv::calc, ninv::sch, ninv::tf, ninv::fc, nullptr, nullptr, ninv::F + 1},
    {"Suiveur", "#Gain unitaire, adaptation d'impédance", "suiveur tampon buffer", 3, G_NONE, 0,
     suiv::def, suiv::rows, suiv::calc, suiv::sch, nullptr, nullptr, nullptr, nullptr, 0},
    {"Sommateur inverseur", "V_s=-R_f\\sum V_k/R_k", "sommateur additionneur somme", 3, G_NONE, 0,
     somme::def, somme::rows, somme::calc, somme::sch, nullptr, nullptr, nullptr, nullptr, 0},
    {"Soustracteur", "#Amplificateur différentiel", "soustracteur differentiel difference mode commun", 3,
     G_NONE, 0, diff::def, diff::rows, diff::calc, diff::sch, nullptr, nullptr, nullptr, nullptr, 0},
    {"Intégrateur", "V_s=-\\tfrac{1}{RC}\\textstyle\\int V_e\\,dt", "integrateur", 3, G_BODE, 0,
     integ::def, integ::rows, integ::calc, integ::sch, integ::tf, integ::fc, nullptr, nullptr, integ::F + 1},
    {"Dérivateur", "V_s=-RC\\,dV_e/dt", "derivateur", 3, G_BODE, 0,
     deriv::def, deriv::rows, deriv::calc, deriv::sch, deriv::tf, deriv::fc, nullptr, nullptr, deriv::F + 1},
    {"Trigger de Schmitt inverseur", "#Comparateur à hystérésis, Ve sur l'entrée −", "schmitt trigger hysteresis comparateur inverseur", 3, G_CUSTOM, 0,
     schmitt::def, schmitt::rowsInv, schmitt::calcInv, schmitt::schInv, nullptr, nullptr, schmitt::cycleInv, "Cycle", 0},
    {"Trigger de Schmitt non inverseur", "#Comparateur à hystérésis, Ve sur l'entrée +", "schmitt trigger hysteresis comparateur non inverseur", 3, G_CUSTOM, 0,
     schmitt::def, schmitt::rowsNinv, schmitt::calcNinv, schmitt::schNinv, nullptr, nullptr, schmitt::cycleNinv, "Cycle", 0},
    {"Amplificateur d'instrumentation", "G=(1+2R_1/R_G)\\,R_3/R_2", "instrumentation trois aop mesure differentiel", 3,
     G_NONE, 0, instr::def, instr::rows, instr::calc, instr::sch, nullptr, nullptr, nullptr, nullptr, 0},
    {"Convertisseur courant-tension", "V_s=-R\\,I_e", "transimpedance courant tension photodiode", 3, G_NONE, 0,
     iv::def, iv::rows, iv::calc, iv::sch, nullptr, nullptr, nullptr, nullptr, 0},
};
extern const int g_calcAopN = sizeof(g_calcAop) / sizeof(g_calcAop[0]);
