// Section : filtres passifs (1er et 2e ordre) et actifs (Sallen-Key).
#include "CalcUtil.hpp"

using namespace cu;

static const char *const DESC_F = "Fréquence du signal sinusoïdal";
static const char *const DESC_VE = "Amplitude de l'entrée (crête ou efficace)";

// Cadre commun : borne d'entree (14, y), sortie (166, y), masse en Y.
static void frame(Sch &g, int y, int Y, int xg, int tVe, int tVs)
{
    groundRail(g, 14, 166, Y, xg);
    termIn(g, 14, y, Y, "V_e", tVe);
    termOut(g, 166, y, Y, "V_s", tVs);
}

// ==========================================================================
// 1er ordre RC et RL
// ==========================================================================
namespace o1 {
enum { R, X, F, VE, OFC, OTAU, OH, ODB, OPHI, OVS };   // X = C ou L
enum { CTYPE };
enum { T_R = 1, T_X, T_VE, T_VS };
static void defRC(CalcState &s) { s.v[R] = 1.5e3; s.v[X] = 100e-9; s.v[F] = 1e3; s.v[VE] = 1; }
static void defRL(CalcState &s) { s.v[R] = 1e3; s.v[X] = 47e-3; s.v[F] = 1e3; s.v[VE] = 1; }
static double fcRC(const CalcState &s) { return 1.0 / (fm::TWO_PI * s.v[R] * s.v[X]); }
static double fcRL(const CalcState &s) { return s.v[R] / (fm::TWO_PI * s.v[X]); }
static Cx H(const CalcState &s, double f, double fc)
{
    Cx d = cx(1, f / fc);
    return s.c[CTYPE] ? cx(0, f / fc) / d : 1.0 / d;
}
static void rowsK(const CalcState &s, RowList &r, bool isC)
{
    r.choice("#Type", CTYPE, "Passe-bas|Passe-haut", 0, "Nature du filtre");
    r.in("R", R, U_OHM, ST_E24, T_R);
    if (isC) r.in("C", X, U_F, ST_E12, T_X);
    else r.in("L", X, U_H, ST_E12, T_X);
    r.head("Signal d'entrée");
    r.in("f", F, U_HZ, ST_125, 0, DESC_F);
    r.in("V_e", VE, U_V, ST_LIN01, T_VE, DESC_VE, RF_NONNEG);
    r.head("Résultats");
    r.out("f_c", OFC, U_HZ, isC ? "Fréquence de coupure à −3 dB : 1/(2π RC)" : "Fréquence de coupure à −3 dB : R/(2π L)", true);
    r.out("\\tau", OTAU, U_S, isC ? "Constante de temps RC" : "Constante de temps L/R");
    r.out("|H|", OH, U_NONE, "Module de la fonction de transfert à f");
    r.out("G", ODB, U_DB, "Gain en décibels à f", true);
    r.out("\\varphi", OPHI, U_DEG, "Déphasage de Vs sur Ve à f");
    r.out("V_s", OVS, U_V, "Amplitude de la sortie", false, T_VS);
    r.msg(0);
    (void)s;
}
static void calcK(CalcState &s, bool isC)
{
    double fc = isC ? fcRC(s) : fcRL(s);
    s.v[OFC] = fc;
    s.v[OTAU] = 1.0 / (fm::TWO_PI * fc);
    Cx h = H(s, s.v[F], fc);
    putH(s, h, OH, ODB, OPHI);
    s.v[OVS] = s.v[OH] * s.v[VE];
    setMsg(s, 0, s.c[CTYPE] ? "Pente +20 dB/décade au-dessous de f_c" : "Pente −20 dB/décade au-dessus de f_c", ML_INFO);
}
static void schK(Sch &g, const CalcState &s, bool isC)
{
    const int y = 60, Y = 150, xn = 112;
    bool ph = s.c[CTYPE] == 1;
    // passe-bas RC : R serie, C parallele ; passe-haut RC : C serie, R parallele
    // passe-bas RL : L serie, R parallele ; passe-haut RL : R serie, L parallele
    bool seriesIsR = isC ? !ph : ph;
    if (seriesIsR) g.res(14, y, xn, y, T_R);
    else if (isC) g.cap(14, y, xn, y, T_X);
    else g.ind(14, y, xn, y, T_X);
    g.tex(63, y - 11, seriesIsR ? "R" : (isC ? "C" : "L"), tex::CENTER, seriesIsR ? T_R : T_X);
    if (!seriesIsR) g.res(xn, y, xn, Y, T_R);
    else if (isC) g.cap(xn, y, xn, Y, T_X);
    else g.ind(xn, y, xn, Y, T_X);
    g.tex(xn - 10, 110, seriesIsR ? (isC ? "C" : "L") : "R", tex::RIGHT, seriesIsR ? T_X : T_R);
    g.wire(xn, y, 166, y);
    g.dot(xn, y);
    frame(g, y, Y, xn, T_VE, T_VS);
}
static void rowsRC(const CalcState &s, RowList &r) { rowsK(s, r, true); }
static void rowsRL(const CalcState &s, RowList &r) { rowsK(s, r, false); }
static void calcRC(CalcState &s) { calcK(s, true); }
static void calcRL(CalcState &s) { calcK(s, false); }
static void schRC(Sch &g, const CalcState &s) { schK(g, s, true); }
static void schRL(Sch &g, const CalcState &s) { schK(g, s, false); }
static bool tfRC(const CalcState &s, double f, Cx &h) { h = H(s, f, fcRC(s)); return true; }
static bool tfRL(const CalcState &s, double f, Cx &h) { h = H(s, f, fcRL(s)); return true; }
}  // namespace o1

// ==========================================================================
// 2e ordre RLC
// ==========================================================================
namespace o2 {
enum { R, L, C, F, VE, OF0, OQ, OXI, OB, OFR, OHMAX, OH, ODB, OPHI, OVS };
enum { CTYPE };
enum { T_R = 1, T_L, T_C, T_VE, T_VS };
enum { PB, PH, PBANDE, CB };
static void def(CalcState &s) { s.v[R] = 220; s.v[L] = 10e-3; s.v[C] = 100e-9; s.v[F] = 5e3; s.v[VE] = 1; }
static double f0(const CalcState &s) { return 1.0 / (fm::TWO_PI * fm::sqrt(s.v[L] * s.v[C])); }
static double qf(const CalcState &s) { return fm::sqrt(s.v[L] / s.v[C]) / s.v[R]; }
static Cx H(const CalcState &s, double f)
{
    double x = f / f0(s), q = qf(s);
    Cx d = cx(1 - x * x, x / q);
    switch (s.c[CTYPE]) {
    case PB: return 1.0 / d;
    case PH: return cx(-x * x) / d;
    case PBANDE: return cx(0, x / q) / d;
    default: return cx(1 - x * x) / d;
    }
}
static void rows(const CalcState &s, RowList &r)
{
    int t = s.c[CTYPE];
    r.choice("#Sortie", CTYPE, "C : passe-bas|L : passe-haut|R : passe-bande|L+C : coupe-bande", 0,
             "Composant aux bornes duquel on prend Vs");
    r.in("R", R, U_OHM, ST_E24, T_R);
    r.in("L", L, U_H, ST_E12, T_L);
    r.in("C", C, U_F, ST_E12, T_C);
    r.head("Signal d'entrée");
    r.in("f", F, U_HZ, ST_125, 0, DESC_F);
    r.in("V_e", VE, U_V, ST_LIN01, T_VE, DESC_VE, RF_NONNEG);
    r.head("Résultats");
    r.out("f_0", OF0, U_HZ, "Fréquence propre 1/(2π √(LC))", true);
    r.out("Q", OQ, U_NONE, "Facteur de qualité (1/R) √(L/C)", true);
    r.out("\\xi", OXI, U_NONE, "Amortissement ξ = 1/(2Q)");
    if (t == PBANDE || t == CB) r.out("B", OB, U_HZ, t == PBANDE ? "Bande passante à −3 dB : f₀/Q" : "Largeur de la bande coupée à −3 dB : f₀/Q");
    if (t == PB || t == PH) {
        r.out("f_r", OFR, U_HZ, "Fréquence du maximum (résonance si Q > 0,707)");
        r.out("G_{\\max}", OHMAX, U_DB, "Gain maximal (surtension)");
    }
    r.out("|H|", OH, U_NONE, "Module de H à f");
    r.out("G", ODB, U_DB, "Gain en décibels à f", true);
    r.out("\\varphi", OPHI, U_DEG, "Déphasage de Vs sur Ve à f");
    r.out("V_s", OVS, U_V, "Amplitude de la sortie", false, T_VS);
    r.msg(0);
}
static void calc(CalcState &s)
{
    double q = qf(s), fo = f0(s);
    s.v[OF0] = fo;
    s.v[OQ] = q;
    s.v[OXI] = 1 / (2 * q);
    s.v[OB] = fo / q;
    int t = s.c[CTYPE];
    if (q > 0.70710678118654752 + 1e-12) {
        double k = fm::sqrt(1 - 1 / (2 * q * q));
        s.v[OFR] = t == PB ? fo * k : fo / k;
        s.v[OHMAX] = db(q / fm::sqrt(1 - 1 / (4 * q * q)));
        setMsg(s, 0, "Résonance : Q > 1/√2", ML_INFO);
    } else {
        s.v[OFR] = NaN();
        s.v[OHMAX] = 0;
        setMsg(s, 0, q > 0.7071 ? "Q = 1/√2 : réponse de Butterworth" : "Pas de résonance (Q ≤ 1/√2)", ML_INFO);
    }
    if (t == PBANDE) setMsg(s, 0, "Gain 0 dB à f₀, pentes ±20 dB/décade", ML_INFO);
    if (t == CB) setMsg(s, 0, "Réjection totale à f₀ (composants idéaux)", ML_INFO);
    Cx h = H(s, s.v[F]);
    putH(s, h, OH, ODB, OPHI);
    s.v[OVS] = s.v[OH] * s.v[VE];
}
static bool tf(const CalcState &s, double f, Cx &h) { h = H(s, f); return true; }
static double fc(const CalcState &s) { return f0(s); }
static void comp(Sch &g, int k, int x0, int y0, int x1, int y1)
{
    if (k == 0) g.res(x0, y0, x1, y1, T_R);
    else if (k == 1) g.ind(x0, y0, x1, y1, T_L);
    else g.cap(x0, y0, x1, y1, T_C);
}
static const char *const NAMES[3] = {"R", "L", "C"};
static void sch(Sch &g, const CalcState &s)
{
    const int y = 56, Y = 150, xn = 118;
    // composants en serie (a, b) et en parallele (c)
    static const int8_t ser[4][2] = {{0, 1}, {0, 2}, {1, 2}, {0, -1}};
    static const int8_t sh[4] = {2, 1, 0, -1};
    int t = s.c[CTYPE];
    if (ser[t][1] >= 0) {
        comp(g, ser[t][0], 14, y, 66, y);
        comp(g, ser[t][1], 66, y, xn, y);
        g.tex(40, y - 11, NAMES[ser[t][0]], tex::CENTER, 1 + ser[t][0]);
        g.tex(92, y - 11, NAMES[ser[t][1]], tex::CENTER, 1 + ser[t][1]);
    } else {
        comp(g, 0, 14, y, xn, y);
        g.tex(66, y - 11, "R", tex::CENTER, T_R);
    }
    if (sh[t] >= 0) {
        comp(g, sh[t], xn, y, xn, Y);
        g.tex(xn - 10, 108, NAMES[sh[t]], tex::RIGHT, 1 + sh[t]);
    } else {
        comp(g, 1, xn, y, xn, 103);
        comp(g, 2, xn, 103, xn, Y);
        g.tex(xn - 10, 84, "L", tex::RIGHT, T_L);
        g.tex(xn - 10, 132, "C", tex::RIGHT, T_C);
    }
    g.wire(xn, y, 166, y);
    g.dot(xn, y);
    frame(g, y, Y, xn, T_VE, T_VS);
}
}  // namespace o2

// ==========================================================================
// Sallen-Key (passe-bas et passe-haut), gain K = 1 + Rb/Ra
// ==========================================================================
namespace sk {
// passe-bas : R1, R2 en serie, C1 contre-reaction, C2 a la masse
// passe-haut : C1, C2 en serie, R1 contre-reaction, R2 a la masse
enum { R1, R2, C1, C2, RA, RB, F, VE, VSAT, OK_, OF0, OQ, OH, ODB, OPHI, OVS };
enum { CGAIN };
enum { T_R1 = 1, T_R2, T_C1, T_C2, T_RA, T_RB, T_VE, T_VS };
static void defPB(CalcState &s)
{
    s.v[R1] = 10e3; s.v[R2] = 10e3; s.v[C1] = 22e-9; s.v[C2] = 10e-9;
    s.v[RA] = 10e3; s.v[RB] = 5.6e3; s.v[F] = 1e3; s.v[VE] = 1; s.v[VSAT] = 13;
}
static void defPH(CalcState &s)
{
    s.v[R1] = 10e3; s.v[R2] = 22e3; s.v[C1] = 10e-9; s.v[C2] = 10e-9;
    s.v[RA] = 10e3; s.v[RB] = 5.6e3; s.v[F] = 1e3; s.v[VE] = 1; s.v[VSAT] = 13;
}
static double K(const CalcState &s) { return s.c[CGAIN] ? 1 + s.v[RB] / s.v[RA] : 1.0; }
static double prodRC(const CalcState &s) { return s.v[R1] * s.v[R2] * s.v[C1] * s.v[C2]; }
// coefficient du terme en s (denominateur 1 + a1 s + a2 s^2)
static double a1(const CalcState &s, bool hp)
{
    double k = K(s);
    if (!hp) return (s.v[R1] + s.v[R2]) * s.v[C2] + (1 - k) * s.v[R1] * s.v[C1];
    return s.v[R1] * (s.v[C1] + s.v[C2]) + (1 - k) * s.v[R2] * s.v[C2];
}
static Cx H(const CalcState &s, double f, bool hp)
{
    double w = w2pi(f), a2 = prodRC(s);
    Cx d = cx(1 - w * w * a2, w * a1(s, hp));
    return hp ? cx(-K(s) * w * w * a2) / d : cx(K(s)) / d;
}
static void rowsK(const CalcState &s, RowList &r, bool hp)
{
    if (!hp) {
        r.in("R_1", R1, U_OHM, ST_E24, T_R1, "Résistance d'entrée");
        r.in("R_2", R2, U_OHM, ST_E24, T_R2, "Résistance vers l'entrée +");
        r.in("C_1", C1, U_F, ST_E12, T_C1, "Condensateur de contre-réaction (vers la sortie)");
        r.in("C_2", C2, U_F, ST_E12, T_C2, "Condensateur de l'entrée + à la masse");
    } else {
        r.in("C_1", C1, U_F, ST_E12, T_C1, "Condensateur d'entrée");
        r.in("C_2", C2, U_F, ST_E12, T_C2, "Condensateur vers l'entrée +");
        r.in("R_1", R1, U_OHM, ST_E24, T_R1, "Résistance de contre-réaction (vers la sortie)");
        r.in("R_2", R2, U_OHM, ST_E24, T_R2, "Résistance de l'entrée + à la masse");
    }
    r.choice("#Gain", CGAIN, "unitaire (suiveur)|K = 1 + Rb/Ra", T_RB, "Amplificateur non inverseur de gain K");
    if (s.c[CGAIN]) {
        r.in("R_a", RA, U_OHM, ST_E24, T_RA);
        r.in("R_b", RB, U_OHM, ST_E24, T_RB);
    }
    r.head("Signal d'entrée");
    r.in("f", F, U_HZ, ST_125, 0, DESC_F);
    r.in("V_e", VE, U_V, ST_LIN01, T_VE, DESC_VE, RF_NONNEG);
    r.in("V_{sat}", VSAT, U_V, ST_LIN1, 0, "Saturation de la sortie de l'AOP");
    r.head("Résultats");
    r.out("f_0", OF0, U_HZ, "f₀ = 1/(2π √(R₁R₂C₁C₂))", true);
    r.out("Q", OQ, U_NONE, "Facteur de qualité (0,707 : Butterworth)", true);
    r.out("K", OK_, U_NONE, hp ? "Gain aux hautes fréquences" : "Gain en continu");
    r.out("|H|", OH, U_NONE, "Module de H à f");
    r.out("G", ODB, U_DB, "Gain en décibels à f");
    r.out("\\varphi", OPHI, U_DEG, "Déphasage de Vs sur Ve à f");
    r.out("V_s", OVS, U_V, "Amplitude de la sortie", false, T_VS);
    r.msg(0);
}
static void calcK(CalcState &s, bool hp)
{
    double a2 = prodRC(s), b = a1(s, hp);
    s.v[OK_] = K(s);
    s.v[OF0] = 1.0 / (fm::TWO_PI * fm::sqrt(a2));
    s.v[OQ] = b > 0 ? fm::sqrt(a2) / b : NaN();
    Cx h = H(s, s.v[F], hp);
    putH(s, h, OH, ODB, OPHI);
    s.v[OVS] = s.v[OH] * s.v[VE];
    if (!(b > 0)) setMsg(s, 0, "Instable : K trop grand (pôles à partie réelle ≥ 0)", ML_BAD);
    else if (fm::fabs(s.v[OVS]) > s.v[VSAT]) setMsg(s, 0, "Sortie saturée : |Vs| > Vsat", ML_BAD);
    else if (fm::fabs(s.v[OQ] - 0.7071) < 0.01) setMsg(s, 0, "Q ≈ 0,707 : Butterworth (gain le plus plat)", ML_OK);
    else if (s.v[OQ] > 0.7072) setMsg(s, 0, "Q > 0,707 : bosse de résonance", ML_INFO);
    else setMsg(s, 0, "Q < 0,707 : coupure plus douce", ML_INFO);
}
static void schK(Sch &g, const CalcState &s, bool hp)
{
    const int Y = 152, ax = 116, ay = 96, w = 30, d = Sch::inDy(w), yi = ay - d;
    // element serie 1 (entree -> A), serie 2 (A -> B), contre-reaction (A -> sortie), masse (B -> masse)
    if (!hp) {
        g.res(8, yi, 48, yi, T_R1);
        g.res(48, yi, 92, yi, T_R2);
        g.cap(48, 42, 150, 42, T_C1);
        g.cap(92, yi, 92, Y, T_C2);
    } else {
        g.cap(8, yi, 48, yi, T_C1);
        g.cap(48, yi, 92, yi, T_C2);
        g.res(48, 42, 150, 42, T_R1);
        g.res(92, yi, 92, Y, T_R2);
    }
    g.tex(28, yi - 10, hp ? "C_1" : "R_1", tex::CENTER, T_R1 + (hp ? 2 : 0));
    g.tex(70, yi - 10, hp ? "C_2" : "R_2", tex::CENTER, T_R2 + (hp ? 2 : 0));
    g.tex(99, 33, hp ? "R_1" : "C_1", tex::CENTER, hp ? T_R1 : T_C1);
    g.tex(86, 128, hp ? "R_2" : "C_2", tex::RIGHT, hp ? T_R2 : T_C2);
    g.wire(48, yi, 48, 42);
    g.dot(48, yi);
    g.wire(92, yi, ax, yi);
    g.dot(92, yi);
    g.wire(150, 42, 150, ay);
    g.opamp(ax, ay, w, false);
    g.wire(ax + w, ay, 172, ay);
    g.dot(150, ay);
    // contre-reaction de l'entree -
    int ym = ay + d;
    g.wireHV(ax, ym, 106, 126);
    if (s.c[CGAIN]) {
        g.res(106, 126, 150, 126, T_RB);
        g.texSmall(128, 117, "R_b", tex::CENTER, T_RB);
        g.res(106, 126, 106, Y, T_RA);
        g.texSmall(111, 146, "R_a", tex::LEFT, T_RA);
        g.dot(106, 126);
        g.dot(106, Y);
    } else {
        g.wire(106, 126, 150, 126);
    }
    g.wire(150, 126, 150, ay);
    groundRail(g, 8, 172, Y, 92);
    g.term(8, yi);
    g.varrow(8, Y - 3, yi + 4, "V_e", 1, T_VE);
    g.term(172, ay);
    g.varrow(172, Y - 3, ay + 4, "V_s", -1, T_VS);
}
static void rowsPB(const CalcState &s, RowList &r) { rowsK(s, r, false); }
static void rowsPH(const CalcState &s, RowList &r) { rowsK(s, r, true); }
static void calcPB(CalcState &s) { calcK(s, false); }
static void calcPH(CalcState &s) { calcK(s, true); }
static void schPB(Sch &g, const CalcState &s) { schK(g, s, false); }
static void schPH(Sch &g, const CalcState &s) { schK(g, s, true); }
static bool tfPB(const CalcState &s, double f, Cx &h) { h = H(s, f, false); return true; }
static bool tfPH(const CalcState &s, double f, Cx &h) { h = H(s, f, true); return true; }
static double fc(const CalcState &s) { return 1.0 / (fm::TWO_PI * fm::sqrt(prodRC(s))); }
}  // namespace sk

extern const CalcDef g_calcFilter[] = {
    {"Filtre RC du 1er ordre", "f_c=1/(2\\pi RC)", "filtre rc passe bas haut premier ordre coupure", 2, G_BODE, 0,
     o1::defRC, o1::rowsRC, o1::calcRC, o1::schRC, o1::tfRC, o1::fcRC, nullptr, nullptr, o1::F + 1},
    {"Filtre RL du 1er ordre", "f_c=R/(2\\pi L)", "filtre rl passe bas haut premier ordre bobine", 2, G_BODE, 0,
     o1::defRL, o1::rowsRL, o1::calcRL, o1::schRL, o1::tfRL, o1::fcRL, nullptr, nullptr, o1::F + 1},
    {"Filtre RLC du 2e ordre", "#Passe-bas, passe-haut, passe-bande, coupe-bande", "filtre rlc second ordre passe bande coupe rejecteur", 2, G_BODE, 0,
     o2::def, o2::rows, o2::calc, o2::sch, o2::tf, o2::fc, nullptr, nullptr, o2::F + 1},
    {"Sallen-Key passe-bas", "#Filtre actif du 2e ordre", "sallen key passe bas actif butterworth second ordre", 2, G_BODE, 0,
     sk::defPB, sk::rowsPB, sk::calcPB, sk::schPB, sk::tfPB, sk::fc, nullptr, nullptr, sk::F + 1},
    {"Sallen-Key passe-haut", "#Filtre actif du 2e ordre", "sallen key passe haut actif butterworth second ordre", 2, G_BODE, 0,
     sk::defPH, sk::rowsPH, sk::calcPH, sk::schPH, sk::tfPH, sk::fc, nullptr, nullptr, sk::F + 1},
};
extern const int g_calcFilterN = sizeof(g_calcFilter) / sizeof(g_calcFilter[0]);
