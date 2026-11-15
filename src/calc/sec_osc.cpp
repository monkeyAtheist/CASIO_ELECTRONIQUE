// Section : oscillateurs et temporisations (NE555, multivibrateur, Wien, LC).
#include "CalcUtil.hpp"

using namespace cu;

static const double LN2 = 0.69314718055994530942;
static const double LN3 = 1.09861228866810969140;

static void valText(Sch &g, int x, int y, double v, uint8_t unit, int align = 0, bool bold = false)
{
    char buf[32];
    fmtValue(buf, sizeof buf, v, unit, 3);
    int f = bold ? BOLD_12 : SANS_12;
    int w = font::width(f, buf);
    if (align == 1) x -= w / 2;
    else if (align == 2) x -= w;
    if (x + w > 177) x = 177 - w;
    font::drawText(*g.cv, f, g.ox + x, g.oy + y, buf, g.volt);
}

// Boitier NE555 (x, y : coin haut gauche, 54 x 76) : broches 7, 6, 2 a gauche,
// 8 et 4 en haut, 1 en bas, 3 a droite.
static void ne555(Sch &g, int x, int y, int tag)
{
    g.chip(x, y, 54, 76, nullptr, tag);
    tex::draw(*g.cv, "#555", g.ox + x + 30, g.oy + y + 42, g.on(tag) ? g.accent : g.ink, tex::CENTER, 1);
    g.texSmall(x + 3, y + 26, "#7", tex::LEFT);
    g.texSmall(x + 3, y + 52, "#6", tex::LEFT);
    g.texSmall(x + 3, y + 70, "#2", tex::LEFT);
    g.texSmall(x + 12, y + 12, "#8", tex::CENTER);
    g.texSmall(x + 42, y + 12, "#4", tex::CENTER);
    g.texSmall(x + 27, y + 72, "#1", tex::CENTER);
    g.texSmall(x + 51, y + 30, "#3", tex::RIGHT);
}

// Petit graphe temporel : deux traces (tension du condensateur et sortie).
struct Wave {
    int X0, X1, Y0, Y1;
    double tmax, lo, hi;
    int px(double t) const { return X0 + fm::iround(fm::clampc((t / tmax) * (X1 - X0))); }
    double py(double v) const { return fm::clampc(Y1 - (v - lo) / (hi - lo) * (Y1 - Y0)); }
};
static void waveAxes(Canvas &cv, const Wave &w, const char *label)
{
    uint16_t axc = rgb(150, 156, 170);
    cv.fillRect(w.X0, w.Y0, 1, w.Y1 - w.Y0 + 1, axc);
    int y0 = (int)w.py(0);
    if (y0 >= w.Y0 && y0 <= w.Y1) cv.fillRect(w.X0, y0, w.X1 - w.X0, 1, axc);
    tex::draw(cv, label, w.X0 + 5, w.Y0 + 10, col::inkSoft, tex::LEFT, 2);
}
static void level(Canvas &cv, const Wave &w, double v, uint16_t c)
{
    int y = (int)w.py(v);
    for (int x = w.X0; x < w.X1; x += 3) cv.fillRect(x, y, 1, 1, rgb(170, 175, 186));
    char buf[24];
    fmtValue(buf, sizeof buf, v, U_V, 3);
    font::drawText(cv, SANS_12, w.X0 - 4 - font::width(SANS_12, buf), y + 4, buf, c);
}

// ==========================================================================
// NE555 astable
// ==========================================================================
namespace ast {
enum { RA, RB, C, VCC, OTH, OTL, OT, OF, OAL };
enum { CDIODE };
enum { T_RA = 1, T_RB, T_C, T_D, T_U };
static void def(CalcState &s) { s.v[RA] = 1e3; s.v[RB] = 10e3; s.v[C] = 100e-9; s.v[VCC] = 9; }
static double tauC(const CalcState &s) { return (s.c[CDIODE] ? s.v[RA] : s.v[RA] + s.v[RB]) * s.v[C]; }
static double tauD(const CalcState &s) { return s.v[RB] * s.v[C]; }
static void rows(const CalcState &, RowList &r)
{
    r.in("R_A", RA, U_OHM, ST_E24, T_RA, "Entre +V_CC et la broche 7 (≥ 1 kΩ)");
    r.in("R_B", RB, U_OHM, ST_E24, T_RB, "Entre les broches 7 et 6");
    r.in("C", C, U_F, ST_E12, T_C, "Condensateur de temporisation");
    r.choice("#Diode", CDIODE, "non|sur R_B", T_D, "Avec diode : charge par R_A seule (α < 50 % possible)");
    r.in("V_{CC}", VCC, U_V, ST_LIN1, T_U, "Alimentation (4,5 à 16 V)");
    r.head("Résultats");
    r.out("f", OF, U_HZ, "Fréquence 1/T", true);
    r.out("T", OT, U_S, "Période t_H + t_L");
    r.out("t_H", OTH, U_S, "Sortie haute : ln 2 (R_A + R_B) C", false, T_RA);
    r.out("t_L", OTL, U_S, "Sortie basse : ln 2 R_B C", false, T_RB);
    r.out("\\alpha", OAL, U_PCT, "Rapport cyclique t_H / T", true);
    r.msg(0);
}
static void calc(CalcState &s)
{
    s.v[OTH] = LN2 * tauC(s);
    s.v[OTL] = LN2 * tauD(s);
    s.v[OT] = s.v[OTH] + s.v[OTL];
    s.v[OF] = 1 / s.v[OT];
    s.v[OAL] = s.v[OTH] / s.v[OT] * 100;
    setMsg(s, 0, "Graphe : u_C entre V_CC/3 et 2V_CC/3", ML_INFO);
}
static void sch(Sch &g, const CalcState &s)
{
    const int cx = 96, cy = 46, Y = 156, xr = 60;
    g.wire(xr, 22, cx + 42, 22);
    g.tex(xr + 4, 16, "V_{CC}", tex::LEFT, T_U);
    g.wire(cx + 12, 22, cx + 12, cy);
    g.wire(cx + 42, 22, cx + 42, cy);
    g.dot(cx + 12, 22);
    ne555(g, cx, cy, 0);
    g.res(xr, 22, xr, 68, T_RA);
    g.tex(xr - 8, 48, "R_A", tex::RIGHT, T_RA);
    g.dot(xr, 68);
    g.wire(xr, 68, cx, 68);
    g.res(xr, 68, xr, 104, T_RB);
    g.tex(xr - 8, 92, "R_B", tex::RIGHT, T_RB);
    if (s.c[CDIODE]) {
        g.wireHV(xr, 68, 42, 72, T_D);
        g.diode(42, 72, 42, 100, 0, T_D);
        g.wireVH(42, 100, xr, 104, T_D);
    }
    g.dot(xr, 104);
    g.wire(xr, 104, 84, 104);
    g.wire(84, 94, 84, 112);
    g.wire(84, 94, cx, 94);
    g.wire(84, 112, cx, 112);
    g.dot(84, 104);
    g.cap(xr, 104, xr, Y, T_C);
    g.tex(xr - 8, 134, "C", tex::RIGHT, T_C);
    g.wire(cx + 27, cy + 76, cx + 27, Y);
    g.wire(xr, Y, cx + 27, Y);
    g.gnd(cx + 27, Y);
    g.wire(cx + 54, cy + 26, 170, cy + 26);
    g.term(170, cy + 26);
    g.tex(172, cy + 20, "v_s", tex::RIGHT);
    valText(g, 176, 168, s.v[OF], U_HZ, 2, true);
}
static void graph(Canvas &cv, const CalcState &s, const GraphArea &a, uint16_t accent)
{
    double vcc = s.v[VCC], tc = tauC(s), td = tauD(s), T = s.v[OT];
    Wave w{a.x + 52, a.x + a.w - 10, a.y + 18, a.y + 104, 2.5 * T, 0, vcc};
    Wave o{a.x + 52, a.x + a.w - 10, a.y + 124, a.y + 170, 2.5 * T, 0, vcc};
    waveAxes(cv, w, "u_C");
    waveAxes(cv, o, "#sortie");
    level(cv, w, vcc / 3, col::inkSoft);
    level(cv, w, 2 * vcc / 3, col::inkSoft);
    level(cv, o, vcc, col::inkSoft);
    int px = 0, py = 0, opy = 0;
    const int N = 400;
    for (int i = 0; i <= N; i++) {
        double t = w.tmax * i / N;
        double tt = t - fm::floor(t / T) * T;
        bool high = tt < s.v[OTH];
        double v = high ? vcc - (2 * vcc / 3) * fm::exp(-tt / tc) : (2 * vcc / 3) * fm::exp(-(tt - s.v[OTH]) / td);
        int x = (int)((w.X0 + (w.X1 - w.X0) * (double)i / N) * 16);
        int y = (int)(w.py(v) * 16), oy2 = (int)(o.py(high ? vcc : 0) * 16);
        if (!(T > 0)) break;
        if (i) {
            cv.lineSub(px, py, x, y, accent);
            cv.lineSub(px, py + 10, x, y + 10, accent);
            cv.lineSub(px, opy, x, oy2, mix16(accent, col::ink, 6));
            cv.lineSub(px, opy + 10, x, oy2 + 10, mix16(accent, col::ink, 6));
        }
        px = x;
        py = y;
        opy = oy2;
    }
    char buf[48];
    int k = scopy(buf, sizeof buf, "f = ");
    k += fmtValue(buf + k, (int)sizeof(buf) - k, s.v[OF], U_HZ);
    k += scopy(buf + k, (int)sizeof(buf) - k, "   \xCE\xB1 = ");
    fmtValue(buf + k, (int)sizeof(buf) - k, s.v[OAL], U_PCT);
    font::drawText(cv, BOLD_12, a.x + a.w - 10 - font::width(BOLD_12, buf), a.y + 14, buf, accent);
}
}  // namespace ast

// ==========================================================================
// NE555 monostable
// ==========================================================================
namespace mono {
enum { R, C, T };
enum { CMODE };
enum { T_R = 1, T_C };
static void def(CalcState &s) { s.v[R] = 100e3; s.v[C] = 10e-6; s.v[T] = 1; }
static void rows(const CalcState &s, RowList &r)
{
    int m = s.c[CMODE];
    r.choice("#Calculer", CMODE, "durée T|R|C", 0, "Grandeur à calculer");
    if (m != 1) r.in("R", R, U_OHM, ST_E24, T_R, "Entre +V_CC et les broches 6-7");
    if (m != 2) r.in("C", C, U_F, ST_E12, T_C, "Condensateur de temporisation");
    if (m != 0) r.in("T", T, U_S, ST_125, 0, "Durée d'impulsion voulue");
    r.head("Résultats");
    if (m == 0) r.out("T", T, U_S, "T = ln 3 · R C ≈ 1,1 R C", true);
    if (m == 1) r.out("R", R, U_OHM, "R = T / (1,1 C)", true, T_R);
    if (m == 2) r.out("C", C, U_F, "C = T / (1,1 R)", true, T_C);
    r.msg(0);
}
static void calc(CalcState &s)
{
    switch (s.c[CMODE]) {
    case 0: s.v[T] = LN3 * s.v[R] * s.v[C]; break;
    case 1: s.v[R] = s.v[T] / (LN3 * s.v[C]); break;
    default: s.v[C] = s.v[T] / (LN3 * s.v[R]); break;
    }
    setMsg(s, 0, "Déclenchement : front descendant sur la broche 2", ML_INFO);
}
static void sch(Sch &g, const CalcState &s)
{
    const int cx = 96, cy = 46, Y = 156, xr = 40;
    g.wire(xr, 22, cx + 42, 22);
    g.tex(xr + 4, 16, "V_{CC}", tex::LEFT);
    g.wire(cx + 12, 22, cx + 12, cy);
    g.wire(cx + 42, 22, cx + 42, cy);
    g.dot(cx + 12, 22);
    ne555(g, cx, cy, 0);
    g.res(xr, 22, xr, 84, T_R);
    g.tex(xr - 8, 56, "R", tex::RIGHT, T_R);
    g.dot(xr, 84);
    g.wire(xr, 84, 84, 84);
    g.wire(84, 72, 84, 98);
    g.wire(84, 72, cx, 72);
    g.wire(84, 98, cx, 98);
    g.dot(84, 84);
    g.cap(xr, 84, xr, Y, T_C);
    g.tex(xr - 8, 124, "C", tex::RIGHT, T_C);
    g.wireHV(cx, 116, 66, 140);
    g.term(66, 140);
    g.tex(72, 144, "v_e", tex::LEFT);
    g.wire(cx + 27, cy + 76, cx + 27, Y);
    g.wire(xr, Y, cx + 27, Y);
    g.gnd(cx + 27, Y);
    g.wire(cx + 54, cy + 26, 170, cy + 26);
    g.term(170, cy + 26);
    g.tex(172, cy + 20, "v_s", tex::RIGHT);
    valText(g, 176, 168, s.v[T], U_S, 2, true);
}
}  // namespace mono

// ==========================================================================
// Multivibrateur astable a AOP
// ==========================================================================
namespace mv {
enum { R, C, R1, R2, VSAT, OBETA, OT, OF, OVT };
enum { T_R = 1, T_C, T_R1, T_R2 };
static void def(CalcState &s) { s.v[R] = 10e3; s.v[C] = 100e-9; s.v[R1] = 10e3; s.v[R2] = 10e3; s.v[VSAT] = 12; }
static void rows(const CalcState &, RowList &r)
{
    r.in("R", R, U_OHM, ST_E24, T_R, "Résistance de charge du condensateur (sortie → entrée −)");
    r.in("C", C, U_F, ST_E12, T_C, "Condensateur (entrée − → masse)");
    r.in("R_1", R1, U_OHM, ST_E24, T_R1, "Entrée + → masse");
    r.in("R_2", R2, U_OHM, ST_E24, T_R2, "Entrée + → sortie (réaction positive)");
    r.in("V_{sat}", VSAT, U_V, ST_LIN1, 0, "Niveaux de sortie ±V_sat");
    r.head("Résultats");
    r.out("\\beta", OBETA, U_NONE, "β = R₁/(R₁ + R₂)");
    r.out("\\pm V_T", OVT, U_V, "Seuils de basculement ±β V_sat");
    r.out("T", OT, U_S, "T = 2RC ln((1+β)/(1−β)) = 2RC ln(1 + 2R₁/R₂)", true);
    r.out("f", OF, U_HZ, "Fréquence 1/T", true);
    r.msg(0);
}
static void calc(CalcState &s)
{
    double b = s.v[R1] / (s.v[R1] + s.v[R2]);
    s.v[OBETA] = b;
    s.v[OVT] = b * s.v[VSAT];
    s.v[OT] = 2 * s.v[R] * s.v[C] * fm::ln((1 + b) / (1 - b));
    s.v[OF] = 1 / s.v[OT];
    setMsg(s, 0, "Rapport cyclique 50 % (niveaux symétriques)", ML_INFO);
}
static void sch(Sch &g, const CalcState &s)
{
    const int Y = 152, ax = 96, ay = 70, w = 36, d = Sch::inDy(w);
    g.opamp(ax, ay, w, true);
    g.wire(70, ay - d, ax, ay - d);
    g.dot(70, ay - d);
    g.wire(70, ay - d, 70, 30, T_R);
    g.res(70, 30, 144, 30, T_R);
    g.tex(107, 22, "R", tex::CENTER, T_R);
    g.wire(144, 30, 144, ay, T_R);
    g.cap(70, ay - d, 70, Y, T_C);
    g.tex(60, 116, "C", tex::RIGHT, T_C);
    g.wireHV(ax, ay + d, 86, 112);
    g.dot(86, 112);
    g.res(86, 112, 144, 112, T_R2);
    g.tex(115, 105, "R_2", tex::CENTER, T_R2);
    g.wire(144, 112, 144, ay, T_R2);
    g.res(86, 112, 86, Y, T_R1);
    g.tex(94, 136, "R_1", tex::LEFT, T_R1);
    g.wire(ax + w, ay, 166, ay);
    g.dot(144, ay);
    g.wire(70, Y, 166, Y);
    g.dot(86, Y);
    g.gnd(110, Y);
    g.term(166, Y);
    termOut(g, 166, ay, Y, "v_s");
    valText(g, 32, 30, s.v[OF], U_HZ, 1, true);
}
static void graph(Canvas &cv, const CalcState &s, const GraphArea &a, uint16_t accent)
{
    double vs = s.v[VSAT], b = s.v[OBETA], tau = s.v[R] * s.v[C], T = s.v[OT];
    Wave w{a.x + 52, a.x + a.w - 10, a.y + 20, a.y + a.h - 12, 2.5 * T, -vs * 1.1, vs * 1.1};
    waveAxes(cv, w, "u_C\\ \\text{et}\\ v_s");
    level(cv, w, vs, col::inkSoft);
    level(cv, w, -vs, col::inkSoft);
    level(cv, w, b * vs, accent);
    level(cv, w, -b * vs, accent);
    const int N = 400;
    int px = 0, py = 0, opy = 0;
    for (int i = 0; i <= N; i++) {
        double t = w.tmax * i / N;
        double tt = t - fm::floor(t / T) * T;
        bool high = tt < T / 2;
        double t2 = high ? tt : tt - T / 2;
        // charge vers +Vsat depuis -bVsat, puis vers -Vsat depuis +bVsat
        double v = high ? vs - (vs + b * vs) * fm::exp(-t2 / tau) : -vs + (vs + b * vs) * fm::exp(-t2 / tau);
        int x = (int)((w.X0 + (w.X1 - w.X0) * (double)i / N) * 16);
        int y = (int)(w.py(v) * 16), oy2 = (int)(w.py(high ? vs : -vs) * 16);
        if (i) {
            cv.lineSub(px, py, x, y, accent);
            cv.lineSub(px, py + 10, x, y + 10, accent);
            cv.lineSub(px, opy, x, oy2, mix16(accent, col::ink, 8));
        }
        px = x;
        py = y;
        opy = oy2;
    }
    char buf[40];
    int k = scopy(buf, sizeof buf, "T = ");
    fmtValue(buf + k, (int)sizeof(buf) - k, T, U_S);
    font::drawText(cv, BOLD_12, a.x + a.w - 10 - font::width(BOLD_12, buf), a.y + 14, buf, accent);
}
}  // namespace mv

// ==========================================================================
// Oscillateur a pont de Wien
// ==========================================================================
namespace wien {
enum { R, C, RF, RG, OF0, OA };
enum { T_R = 1, T_C, T_RF, T_RG };
static void def(CalcState &s) { s.v[R] = 10e3; s.v[C] = 10e-9; s.v[RF] = 22e3; s.v[RG] = 10e3; }
static void rows(const CalcState &, RowList &r)
{
    r.in("R", R, U_OHM, ST_E24, T_R, "Les deux résistances du pont (identiques)");
    r.in("C", C, U_F, ST_E12, T_C, "Les deux condensateurs du pont (identiques)");
    r.in("R_f", RF, U_OHM, ST_E24, T_RF, "Contre-réaction (sortie → entrée −)");
    r.in("R_g", RG, U_OHM, ST_E24, T_RG, "Entrée − → masse");
    r.head("Résultats");
    r.out("f_0", OF0, U_HZ, "f₀ = 1/(2π RC) : le pont transmet 1/3 sans déphasage", true);
    r.out("A", OA, U_NONE, "Gain de l'amplificateur 1 + R_f/R_g (3 nécessaire)", true);
    r.msg(0);
}
static void calc(CalcState &s)
{
    s.v[OF0] = 1 / (fm::TWO_PI * s.v[R] * s.v[C]);
    s.v[OA] = 1 + s.v[RF] / s.v[RG];
    double a = s.v[OA];
    if (a < 2.999) setMsg(s, 0, "A < 3 : pas d'oscillation", ML_BAD);
    else if (a <= 3.001) setMsg(s, 0, "A = 3 : oscillation entretenue (limite)", ML_OK);
    else if (a < 3.3) setMsg(s, 0, "A > 3 : démarre, peu de distorsion", ML_OK);
    else setMsg(s, 0, "A ≫ 3 : sinus écrêté", ML_WARN);
}
static bool tf(const CalcState &s, double f, Cx &h)
{
    double x = w2pi(f) * s.v[R] * s.v[C];
    h = 1.0 / cx(3, x - 1 / x);
    return true;
}
static double fc(const CalcState &s) { return 1 / (fm::TWO_PI * s.v[R] * s.v[C]); }
static void sch(Sch &g, const CalcState &)
{
    const int Y = 156, ax = 98, ay = 96, w = 34, d = Sch::inDy(w);
    g.opamp(ax, ay, w, false);
    g.wire(ax + w, ay, 170, ay);
    g.term(170, ay);
    g.dot(156, ay);
    // reseau serie (sortie -> +)
    g.wire(156, ay, 156, 26);
    g.res(156, 26, 110, 26, T_R);
    g.cap(110, 26, 60, 26, T_C);
    g.tex(133, 18, "R", tex::CENTER, T_R);
    g.tex(85, 16, "C", tex::CENTER, T_C);
    g.wire(60, 26, 60, ay - d);
    g.dot(60, ay - d);
    g.wire(60, ay - d, ax, ay - d);
    // reseau parallele (+ -> masse)
    g.wire(36, ay - d, 60, ay - d);
    g.res(36, ay - d, 36, Y, T_R);
    g.cap(60, ay - d, 60, Y, T_C);
    g.tex(28, 128, "R", tex::RIGHT, T_R);
    g.tex(68, 128, "C", tex::LEFT, T_C);
    g.dot(36, ay - d);
    // gain
    g.wireHV(ax, ay + d, 88, 128);
    g.dot(88, 128);
    g.res(88, 128, 156, 128, T_RF);
    g.texSmall(122, 122, "R_f", tex::CENTER, T_RF);
    g.wire(156, 128, 156, ay);
    g.res(88, 128, 88, Y, T_RG);
    g.texSmall(94, 148, "R_g", tex::LEFT, T_RG);
    g.wire(36, Y, 88, Y);
    g.dot(60, Y);
    g.gnd(60, Y);
}
}  // namespace wien

// ==========================================================================
// Oscillateurs LC : Colpitts et Hartley
// ==========================================================================
namespace lco {
enum { L, C1, C2, L1, L2, M, C, OEQ, OF0, OW0 };
enum { CTYPE };
enum { T_L = 1, T_C1, T_C2, T_L1, T_L2, T_C };
static void def(CalcState &s)
{
    s.v[L] = 100e-6; s.v[C1] = 1e-9; s.v[C2] = 10e-9; s.v[L1] = 10e-6; s.v[L2] = 100e-6; s.v[M] = 0; s.v[C] = 1e-9;
}
static void rows(const CalcState &s, RowList &r)
{
    bool col = s.c[CTYPE] == 0;
    r.choice("#Type", CTYPE, "Colpitts|Hartley", 0, "Colpitts : diviseur capacitif ; Hartley : diviseur inductif");
    if (col) {
        r.in("L", L, U_H, ST_E12, T_L);
        r.in("C_1", C1, U_F, ST_E12, T_C1);
        r.in("C_2", C2, U_F, ST_E12, T_C2);
    } else {
        r.in("L_1", L1, U_H, ST_E12, T_L1);
        r.in("L_2", L2, U_H, ST_E12, T_L2);
        r.in("M", M, U_H, ST_E12, T_L1, "Inductance mutuelle (0 si bobines séparées)", RF_NONNEG);
        r.in("C", C, U_F, ST_E12, T_C);
    }
    r.head("Résultats");
    if (col) r.out("C_{\\text{éq}}", OEQ, U_F, "C₁C₂/(C₁ + C₂) : condensateurs en série");
    else r.out("L_{\\text{éq}}", OEQ, U_H, "L₁ + L₂ + 2M");
    r.out("f_0", OF0, U_HZ, "f₀ = 1/(2π √(L C)) avec les valeurs équivalentes", true);
    r.out("\\omega_0", OW0, U_RADS);
    r.msg(0);
}
static void calc(CalcState &s)
{
    bool col = s.c[CTYPE] == 0;
    double l, c;
    if (col) {
        c = s.v[C1] * s.v[C2] / (s.v[C1] + s.v[C2]);
        l = s.v[L];
        s.v[OEQ] = c;
        setMsg(s, 0, "Entretien : gain de l'amplificateur ≥ C₂/C₁", ML_INFO);
    } else {
        l = s.v[L1] + s.v[L2] + 2 * s.v[M];
        c = s.v[C];
        s.v[OEQ] = l;
        setMsg(s, 0, "Entretien : gain ≥ (L₂ + M)/(L₁ + M)", ML_INFO);
    }
    s.v[OW0] = 1 / fm::sqrt(l * c);
    s.v[OF0] = s.v[OW0] / fm::TWO_PI;
}
static void sch(Sch &g, const CalcState &s)
{
    bool col = s.c[CTYPE] == 0;
    const int xa = 50, xb = 130, yt = 50, yb = 120;
    if (col) {
        g.ind(xa, yt, xb, yt, T_L);
        g.cap(xa, yt, xa, yb, T_C1);
        g.cap(xb, yt, xb, yb, T_C2);
        g.tex(90, yt - 10, "L", tex::CENTER, T_L);
        g.tex(xa - 10, 90, "C_1", tex::RIGHT, T_C1);
        g.tex(xb + 10, 90, "C_2", tex::LEFT, T_C2);
    } else {
        g.cap(xa, yt, xb, yt, T_C);
        g.ind(xa, yt, xa, yb, T_L1);
        g.ind(xb, yt, xb, yb, T_L2);
        g.tex(90, yt - 10, "C", tex::CENTER, T_C);
        g.tex(xa - 8, 90, "L_1", tex::RIGHT, T_L1);
        g.tex(xb + 10, 90, "L_2", tex::LEFT, T_L2);
    }
    g.wire(xa, yb, xb, yb);
    g.dot(90, yb);
    g.gnd(90, yb);
    g.dot(xa, yt);
    g.dot(xb, yt);
    valText(g, 90, 160, s.v[OF0], U_HZ, 1, true);
}
}  // namespace lco

extern const CalcDef g_calcOsc[] = {
    {"NE555 astable", "f=1/(\\ln 2\\,(R_A+2R_B)\\,C)", "ne555 555 astable oscillateur horloge clignotant", 9, G_CUSTOM, 0,
     ast::def, ast::rows, ast::calc, ast::sch, nullptr, nullptr, ast::graph, "Signaux", 0},
    {"NE555 monostable", "T=1{,}1\\,R\\,C", "ne555 555 monostable temporisation impulsion", 9, G_NONE, 0,
     mono::def, mono::rows, mono::calc, mono::sch, nullptr, nullptr, nullptr, nullptr, 0},
    {"Multivibrateur astable à AOP", "T=2RC\\ln(1+2R_1/R_2)", "multivibrateur astable aop relaxation carre", 9, G_CUSTOM, 0,
     mv::def, mv::rows, mv::calc, mv::sch, nullptr, nullptr, mv::graph, "Signaux", 0},
    {"Oscillateur à pont de Wien", "f_0=1/(2\\pi RC),\\ A=3", "wien oscillateur sinus pont", 9, G_BODE, 0,
     wien::def, wien::rows, wien::calc, wien::sch, wien::tf, wien::fc, nullptr, "β(f)", 0},
    {"Oscillateurs Colpitts et Hartley", "f_0=1/(2\\pi\\sqrt{L\\,C_{eq}})", "colpitts hartley oscillateur lc hf", 9, G_NONE, 0,
     lco::def, lco::rows, lco::calc, lco::sch, nullptr, nullptr, nullptr, nullptr, 0},
};
extern const int g_calcOscN = sizeof(g_calcOsc) / sizeof(g_calcOsc[0]);
