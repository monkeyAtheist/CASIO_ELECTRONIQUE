// Section : lois de base (Ohm, diviseur, pont, LED, decibels, thermique...).
#include "CalcUtil.hpp"

using namespace cu;

static void valText(Sch &g, int x, int y, double v, uint8_t unit, int align = 0, bool bold = false)
{
    char buf[32];
    fmtValue(buf, sizeof buf, v, unit, 3);
    int f = bold ? BOLD_12 : SANS_12;
    int w = font::width(f, buf);
    if (align == 1) x -= w / 2;
    else if (align == 2) x -= w;
    if (x + w > 177) x = 177 - w;
    if (x < 1) x = 1;
    font::drawText(*g.cv, f, g.ox + x, g.oy + y, buf, g.volt);
}

// ==========================================================================
// Loi d'Ohm et puissance
// ==========================================================================
namespace ohm {
enum { U, I, R, P };
enum { CK };
enum { T_U = 1, T_I, T_R, T_P };
static const uint8_t KNOWN[6][2] = {{U, I}, {U, R}, {I, R}, {P, U}, {P, I}, {P, R}};
static void def(CalcState &s) { s.v[U] = 12; s.v[I] = 0.1; s.v[R] = 120; s.v[P] = 1.2; s.c[CK] = 1; }
static const char *const LAB[4] = {"U", "I", "R", "P"};
static const uint8_t UNIT[4] = {U_V, U_A, U_OHM, U_W};
static const uint8_t STEP[4] = {ST_LIN1, ST_125, ST_E24, ST_125};
static const char *const DESC[4] = {"Tension U = R I", "Courant I = U / R", "Résistance R = U / I", "Puissance P = U I = R I² = U²/R"};
static void rows(const CalcState &s, RowList &r)
{
    r.choice("#Données", CK, "U et I|U et R|I et R|P et U|P et I|P et R", 0, "Les deux grandeurs connues");
    int a = KNOWN[s.c[CK]][0], b = KNOWN[s.c[CK]][1];
    r.in(LAB[a], a, UNIT[a], STEP[a], a + 1, DESC[a]);
    r.in(LAB[b], b, UNIT[b], STEP[b], b + 1, DESC[b]);
    r.head("Résultats");
    for (int k = 0; k < 4; k++)
        if (k != a && k != b) r.out(LAB[k], k, UNIT[k], DESC[k], true, k + 1);
}
static void calc(CalcState &s)
{
    double &u = s.v[U], &i = s.v[I], &rr = s.v[R], &p = s.v[P];
    switch (s.c[CK]) {
    case 0: rr = u / i; p = u * i; break;
    case 1: i = u / rr; p = u * u / rr; break;
    case 2: u = rr * i; p = rr * i * i; break;
    case 3: i = p / u; rr = u * u / p; break;
    case 4: u = p / i; rr = p / (i * i); break;
    default: u = fm::sqrt(p * rr); i = fm::sqrt(p / rr); break;
    }
}
static void sch(Sch &g, const CalcState &s)
{
    g.vsrc(40, 40, 136, 0, T_U);
    g.wire(40, 40, 130, 40);
    g.wire(40, 136, 130, 136);
    g.res(130, 40, 130, 136, T_R);
    g.tex(140, 94, "R", tex::LEFT, T_R);
    g.varrow(18, 122, 54, "U", -1, T_U);
    g.iarrow(96, 40, 1, 0, "I", 90, 32, T_I);
    valText(g, 64, 94, s.v[U], U_V, 0);
    valText(g, 92, 58, s.v[I], U_A, 1);
    valText(g, 89, 160, s.v[P], U_W, 1, true);
}
}  // namespace ohm

// ==========================================================================
// Diviseur de tension
// ==========================================================================
namespace div {
enum { VE, R1, R2, RL, VST, OVS, OI, OP1, OP2, ORTH, ORAT, OR2E, OVSE };
enum { CMODE, CLOAD };
enum { T_R1 = 1, T_R2, T_RL, T_VE, T_VS };
static void def(CalcState &s) { s.v[VE] = 12; s.v[R1] = 10e3; s.v[R2] = 4.7e3; s.v[RL] = 10e3; s.v[VST] = 3.3; }
static void rows(const CalcState &s, RowList &r)
{
    bool solve = s.c[CMODE] == 1;
    r.choice("#Calculer", CMODE, "V_s|R_2 (V_s voulu)", 0, "Tension de sortie, ou R₂ à choisir");
    r.choice("#Charge", CLOAD, "sans|avec R_L", T_RL, "Charge branchée en sortie");
    r.in("V_e", VE, U_V, ST_LIN1, T_VE, "Tension d'entrée", 0);
    r.in("R_1", R1, U_OHM, ST_E24, T_R1, "Résistance du haut");
    if (!solve) r.in("R_2", R2, U_OHM, ST_E24, T_R2, "Résistance du bas");
    else r.in("V_s", VST, U_V, ST_LIN01, T_VS, "Tension de sortie voulue", 0);
    if (s.c[CLOAD]) r.in("R_L", RL, U_OHM, ST_E24, T_RL, "Résistance de charge");
    r.head("Résultats");
    if (solve) {
        r.out("R_2", R2, U_OHM, "R₂ exacte pour obtenir V_s", true, T_R2);
        r.out("#E24 proche", OR2E, U_OHM, "Valeur normalisée la plus proche", false, T_R2);
        r.out("V_s\\ (E24)", OVSE, U_V, "Tension obtenue avec la valeur E24");
    }
    r.out("V_s", OVS, U_V, "V_s = V_e R₂/(R₁ + R₂) (R₂ ∥ R_L avec charge)", !solve, T_VS);
    r.out("V_s/V_e", ORAT, U_NONE, "Rapport de division");
    r.out("I", OI, U_A, "Courant dans R₁");
    r.out("P_1", OP1, U_W, "Puissance dans R₁", false, T_R1);
    r.out("P_2", OP2, U_W, "Puissance dans R₂", false, T_R2);
    r.out("R_{th}", ORTH, U_OHM, "Résistance de sortie (Thévenin) R₁ ∥ R₂");
    r.msg(0);
}
static double vsOf(const CalcState &s, double r2)
{
    double r2e = s.c[CLOAD] ? par2(r2, s.v[RL]) : r2;
    return s.v[VE] * r2e / (s.v[R1] + r2e);
}
static void calc(CalcState &s)
{
    setMsg(s, 0, nullptr, ML_INFO);
    if (s.c[CMODE] == 1) {
        double ve = s.v[VE], vs = s.v[VST];
        bool ok_ = (ve > 0 && vs > 0 && vs < ve) || (ve < 0 && vs < 0 && vs > ve);
        double rp = ok_ ? s.v[R1] * vs / (ve - vs) : NaN();
        double r2 = rp;
        if (ok_ && s.c[CLOAD]) r2 = rp < s.v[RL] ? 1.0 / (1.0 / rp - 1.0 / s.v[RL]) : NaN();
        if (!ok(r2)) {
            setMsg(s, 0, ok_ ? "Impossible : R_L trop faible" : "Il faut 0 < V_s < V_e", ML_BAD);
            s.v[OR2E] = s.v[OVSE] = NaN();
        } else {
            s.v[R2] = r2;
            s.v[OR2E] = eNearest(E24, r2);
            s.v[OVSE] = vsOf(s, s.v[OR2E]);
        }
    }
    double r2 = s.v[R2];
    double r2e = s.c[CLOAD] ? par2(r2, s.v[RL]) : r2;
    s.v[OVS] = vsOf(s, r2);
    s.v[ORAT] = s.v[OVS] / s.v[VE];
    s.v[OI] = s.v[VE] / (s.v[R1] + r2e);
    s.v[OP1] = s.v[R1] * s.v[OI] * s.v[OI];
    s.v[OP2] = s.v[OVS] * s.v[OVS] / r2;
    s.v[ORTH] = par2(s.v[R1], r2);
    if (!s.msg[0] && s.c[CLOAD]) {
        double vs0 = s.v[VE] * r2 / (s.v[R1] + r2);
        if (fm::fabs(vs0 - s.v[OVS]) > 0.05 * fm::fabs(vs0))
            setMsg(s, 0, "La charge fait chuter V_s de plus de 5 %", ML_WARN);
        else setMsg(s, 0, "Charge peu influente (R_L ≫ R_th)", ML_OK);
    }
}
static void sch(Sch &g, const CalcState &s)
{
    const int x = 70, Y = 152, ym = 88;
    g.term(70, 18);
    g.tex(78, 22, "V_e", tex::LEFT, T_VE);
    g.res(x, 18, x, ym, T_R1);
    g.res(x, ym, x, Y, T_R2);
    g.tex(x - 9, 56, "R_1", tex::RIGHT, T_R1);
    g.tex(x - 9, 124, "R_2", tex::RIGHT, T_R2);
    g.dot(x, ym);
    g.wire(x, ym, 160, ym);
    g.term(160, ym);
    g.wire(x, Y, 160, Y);
    g.term(160, Y);
    g.dot(x, Y);
    g.gnd(x, Y);
    if (s.c[CLOAD]) {
        g.res(124, ym, 124, Y, T_RL);
        g.tex(116, 124, "R_L", tex::RIGHT, T_RL);
        g.dot(124, ym);
        g.dot(124, Y);
    }
    g.varrow(160, Y - 3, ym + 4, "V_s", -1, T_VS);
    valText(g, 106, ym - 6, s.v[OVS], U_V, 1);
}
}  // namespace div

// ==========================================================================
// Pont de Wheatstone
// ==========================================================================
namespace wh {
enum { E, R1, R2, R3, R4, OVA, OVB, OVAB, OR4EQ, OI };
enum { T_R1 = 1, T_R2, T_R3, T_R4, T_E };
static void def(CalcState &s) { s.v[E] = 10; s.v[R1] = 1e3; s.v[R2] = 1e3; s.v[R3] = 1e3; s.v[R4] = 1.1e3; }
static void rows(const CalcState &, RowList &r)
{
    r.in("E", E, U_V, ST_LIN1, T_E, "Tension d'alimentation du pont", 0);
    r.in("R_1", R1, U_OHM, ST_E24, T_R1, "Branche gauche, en haut");
    r.in("R_2", R2, U_OHM, ST_E24, T_R2, "Branche gauche, en bas");
    r.in("R_3", R3, U_OHM, ST_E24, T_R3, "Branche droite, en haut");
    r.in("R_4", R4, U_OHM, ST_E24, T_R4, "Branche droite, en bas (souvent la résistance à mesurer)");
    r.head("Résultats");
    r.out("V_A", OVA, U_V, "E R₂/(R₁ + R₂)");
    r.out("V_B", OVB, U_V, "E R₄/(R₃ + R₄)");
    r.out("V_{AB}", OVAB, U_V, "Tension de déséquilibre V_A − V_B", true);
    r.out("R_4\\ (\\text{équil.})", OR4EQ, U_OHM, "Équilibre : R₁ R₄ = R₂ R₃", true, T_R4);
    r.out("I", OI, U_A, "Courant fourni par la source");
    r.msg(0);
}
static void calc(CalcState &s)
{
    s.v[OVA] = s.v[E] * s.v[R2] / (s.v[R1] + s.v[R2]);
    s.v[OVB] = s.v[E] * s.v[R4] / (s.v[R3] + s.v[R4]);
    s.v[OVAB] = s.v[OVA] - s.v[OVB];
    s.v[OR4EQ] = s.v[R2] * s.v[R3] / s.v[R1];
    s.v[OI] = s.v[E] / (s.v[R1] + s.v[R2]) + s.v[E] / (s.v[R3] + s.v[R4]);
    if (fm::fabs(s.v[OVAB]) < 1e-6 * fm::fabs(s.v[E])) setMsg(s, 0, "Pont équilibré : V_AB = 0", ML_OK);
    else setMsg(s, 0, "Pont déséquilibré", ML_INFO);
}
static void sch(Sch &g, const CalcState &s)
{
    const int xl = 74, xr = 140, yt = 24, ym = 88, Y = 152;
    g.vsrc(30, yt, Y, 0, T_E);
    g.tex(17, 94, "E", tex::RIGHT, T_E);
    g.wire(30, yt, xr, yt);
    g.wire(30, Y, xr, Y);
    g.res(xl, yt, xl, ym, T_R1);
    g.res(xl, ym, xl, Y, T_R2);
    g.res(xr, yt, xr, ym, T_R3);
    g.res(xr, ym, xr, Y, T_R4);
    g.tex(xl - 8, 60, "R_1", tex::RIGHT, T_R1);
    g.tex(xl - 8, 124, "R_2", tex::RIGHT, T_R2);
    g.tex(xr + 8, 60, "R_3", tex::LEFT, T_R3);
    g.tex(xr + 8, 124, "R_4", tex::LEFT, T_R4);
    g.dot(xl, ym);
    g.dot(xr, ym);
    g.dot(xl, yt);
    g.dot(xl, Y);
    g.gnd(xl, Y);
    g.tex(xl - 8, ym + 4, "A", tex::RIGHT);
    g.tex(xr + 8, ym + 4, "B", tex::LEFT);
    g.harrow(xr - 6, xl + 6, ym, "V_{AB}", -1);
    valText(g, (xl + xr) / 2, ym + 16, s.v[OVAB], U_V, 1, true);
}
}  // namespace wh

// ==========================================================================
// Resistance serie d'une LED
// ==========================================================================
namespace led {
enum { VCC, VF, IF_, N, OR, ORE, OI, OPR, OPRC, OPL };
enum { CCOL };
enum { T_R = 1, T_LED, T_V };
static const double VFT[6] = {2.0, 2.1, 2.2, 3.1, 3.2, 1.3};
static void def(CalcState &s) { s.v[VCC] = 5; s.v[VF] = 2.0; s.v[IF_] = 20e-3; s.v[N] = 1; }
static void rows(const CalcState &s, RowList &r)
{
    r.choice("#LED", CCOL, "rouge|jaune|verte|bleue|blanche|infrarouge|autre (V_F saisie)", T_LED,
             "Couleur : fixe la tension directe typique V_F");
    r.in("V_{CC}", VCC, U_V, ST_LIN01, T_V, "Tension d'alimentation");
    if (s.c[CCOL] == 6) r.in("V_F", VF, U_V, ST_LIN01, T_LED, "Tension directe d'une LED (notice)");
    r.in("I_F", IF_, U_A, ST_125, T_LED, "Courant voulu (souvent 5 à 20 mA)");
    r.in("n", N, U_NONE, ST_INT, T_LED, "Nombre de LED en série", RF_POS | RF_INT);
    r.head("Résultats");
    if (s.c[CCOL] != 6) r.out("V_F", VF, U_V, "Tension directe typique de la couleur choisie", false, T_LED);
    r.out("R", OR, U_OHM, "R = (V_CC − n V_F) / I_F", false, T_R);
    r.out("#E24 au-dessus", ORE, U_OHM, "Valeur normalisée supérieure (courant un peu plus faible)", true, T_R);
    r.out("I\\ (E24)", OI, U_A, "Courant réel avec la valeur E24");
    r.out("P_R", OPR, U_W, "Puissance dans la résistance");
    r.out("#Puissance R", OPRC, U_W, "Calibre conseillé (au moins le double)");
    r.out("P_{LED}", OPL, U_W, "Puissance par LED");
    r.msg(0);
}
static void calc(CalcState &s)
{
    if (s.c[CCOL] < 6) s.v[VF] = VFT[s.c[CCOL]];
    double n = s.v[N] < 1 ? 1 : fm::floor(s.v[N] + 0.5);
    s.v[N] = n;
    double vr = s.v[VCC] - n * s.v[VF];
    if (!(vr > 0)) {
        s.v[OR] = s.v[ORE] = s.v[OI] = s.v[OPR] = s.v[OPRC] = s.v[OPL] = NaN();
        setMsg(s, 0, "V_CC trop faible : il faut V_CC > n·V_F", ML_BAD);
        return;
    }
    s.v[OR] = vr / s.v[IF_];
    s.v[ORE] = eCeil(E24, s.v[OR]);
    s.v[OI] = vr / s.v[ORE];
    s.v[OPR] = s.v[ORE] * s.v[OI] * s.v[OI];
    static const double pw[7] = {0.125, 0.25, 0.5, 1, 2, 5, 10};
    s.v[OPRC] = NaN();
    for (int i = 0; i < 7; i++)
        if (pw[i] >= 2 * s.v[OPR]) { s.v[OPRC] = pw[i]; break; }
    s.v[OPL] = s.v[VF] * s.v[OI];
    setMsg(s, 0, vr < 1 ? "Peu de marge : le courant variera beaucoup avec V_F" : "Marge suffisante pour un courant stable",
           vr < 1 ? ML_WARN : ML_OK);
}
static void sch(Sch &g, const CalcState &s)
{
    const int x = 110, yt = 24, Y = 152;
    int n = (int)s.v[N];
    if (n < 1) n = 1;
    g.vsrc(30, yt, Y, 0, T_V);
    g.tex(44, 92, "V_{CC}", tex::LEFT, T_V);
    g.wire(30, yt, x, yt);
    g.wire(30, Y, x, Y);
    g.res(x, yt, x, 64, T_R);
    g.tex(x + 9, 48, "R", tex::LEFT, T_R);
    int shown = n > 3 ? 3 : n;
    int y0 = 64, step = (Y - y0) / shown;
    for (int i = 0; i < shown; i++) g.diode(x, y0 + step * i, x, y0 + step * (i + 1), 1, T_LED);
    if (n > 3) g.tex(x + 14, 136, "#…", tex::LEFT);
    g.tex(x - 14, 110, "#LED", tex::RIGHT, T_LED);
    g.iarrow(x, 62, 0, 1, "I_F", x + 8, 66);
}
}  // namespace led

// ==========================================================================
// Decibels (rapports)
// ==========================================================================
namespace dbr {
enum { X, G, OUTX, OUTG, OP };
enum { CTYPE, CDIR };
static void def(CalcState &s) { s.v[X] = 10; s.v[G] = -3; }
static void rows(const CalcState &s, RowList &r)
{
    bool volt = s.c[CTYPE] == 0, toDb = s.c[CDIR] == 0;
    r.choice("#Rapport", CTYPE, "tension (20 log)|puissance (10 log)", 0, "Rapport de tensions (ou de courants) ou de puissances");
    r.choice("#Sens", CDIR, "rapport → dB|dB → rapport", 0, "Conversion à effectuer");
    if (toDb) r.in(volt ? "V_s/V_e" : "P_s/P_e", X, U_NONE, ST_125, 0, "Rapport (gain ou atténuation)");
    else r.in("G", G, U_DB, ST_LIN1, 0, "Gain en décibels", 0);
    r.head("Résultats");
    if (toDb) r.out("G", OUTG, U_DB, volt ? "G = 20 log(V_s/V_e)" : "G = 10 log(P_s/P_e)", true);
    else r.out(volt ? "V_s/V_e" : "P_s/P_e", OUTX, U_NONE, volt ? "10^(G/20)" : "10^(G/10)", true);
    r.out(volt ? "P_s/P_e" : "V_s/V_e", OP, U_NONE, volt ? "Rapport de puissances (même impédance)" : "Rapport de tensions (même impédance)");
    r.msg(0);
}
static void calc(CalcState &s)
{
    bool volt = s.c[CTYPE] == 0, toDb = s.c[CDIR] == 0;
    double k = volt ? 20 : 10;
    double g = toDb ? k * fm::log10(s.v[X]) : s.v[G];
    double x = toDb ? s.v[X] : fm::pow10(g / k);
    s.v[OUTG] = g;
    s.v[OUTX] = x;
    s.v[OP] = volt ? x * x : fm::sqrt(x);
    setMsg(s, 0, volt ? "Repères : ×2 → +6 dB, ×10 → +20 dB, ÷√2 → −3 dB" : "Repères : ×2 → +3 dB, ×10 → +10 dB", ML_INFO);
}
static void sch(Sch &g, const CalcState &s)
{
    bool volt = s.c[CTYPE] == 0;
    g.chip(64, 60, 52, 50, "\\times\\,A");
    g.wire(20, 85, 64, 85);
    g.wire(116, 85, 160, 85);
    g.term(20, 85);
    g.term(160, 85);
    g.tex(20, 76, volt ? "V_e" : "P_e", tex::CENTER);
    g.tex(160, 76, volt ? "V_s" : "P_s", tex::CENTER);
    char buf[32];
    fmtValue(buf, sizeof buf, s.v[OUTG], U_DB);
    font::drawText(*g.cv, BOLD_14, g.ox + 90 - font::width(BOLD_14, buf) / 2, g.oy + 136, buf, g.volt);
}
}  // namespace dbr

// ==========================================================================
// Niveaux absolus : dBm, dBW, dBV, dBu
// ==========================================================================
namespace dbl {
enum { U, R, OP, ODBM, ODBW, ODBV, ODBU, OUC, OUPP };
enum { T_U = 1, T_R };
static void def(CalcState &s) { s.v[U] = 0.775; s.v[R] = 600; }
static void rows(const CalcState &, RowList &r)
{
    r.in("U_{\\text{eff}}", U, U_V, ST_125, T_U, "Tension efficace sur la charge");
    r.in("R", R, U_OHM, ST_E24, T_R, "Charge (50 Ω en HF, 600 Ω en audio pro)");
    r.head("Résultats");
    r.out("P", OP, U_W, "P = U²/R");
    r.out("#En dBm", ODBM, U_DBM, "10 log(P / 1 mW)", true);
    r.out("#En dBW", ODBW, U_DBW, "10 log(P / 1 W)");
    r.out("#En dBV", ODBV, U_DBV, "20 log(U / 1 V)", true);
    r.out("#En dBu", ODBU, U_DBU, "20 log(U / 0,775 V) : 0 dBu = 1 mW dans 600 Ω");
    r.out("U_{\\text{crête}}", OUC, U_V, "Sinusoïde : U√2");
    r.out("U_{\\text{cc}}", OUPP, U_V, "Crête à crête : 2√2 U");
}
static void calc(CalcState &s)
{
    double u = s.v[U], p = u * u / s.v[R];
    s.v[OP] = p;
    s.v[ODBM] = 10 * fm::log10(p / 1e-3);
    s.v[ODBW] = 10 * fm::log10(p);
    s.v[ODBV] = 20 * fm::log10(u);
    s.v[ODBU] = 20 * fm::log10(u / 0.774596669241483);
    s.v[OUC] = u * fm::sqrt(2.0);
    s.v[OUPP] = 2 * s.v[OUC];
}
static void sch(Sch &g, const CalcState &)
{
    g.vsrc(40, 40, 136, 1, 0);
    g.wire(40, 40, 130, 40);
    g.wire(40, 136, 130, 136);
    g.res(130, 40, 130, 136, T_R);
    g.tex(140, 94, "R", tex::LEFT, T_R);
    g.varrow(112, 122, 54, "u", -1, T_U);
}
}  // namespace dbl

// ==========================================================================
// Dissipateur thermique
// ==========================================================================
namespace th {
enum { P, TJM, TA, RJC, RCS, RSA, RJA, ORSAM, OTJ, OTC, OTS, OTJ0 };
enum { T_JC = 1, T_CS, T_SA, T_P };
static void def(CalcState &s)
{
    s.v[P] = 10; s.v[TJM] = 150; s.v[TA] = 40; s.v[RJC] = 1.5; s.v[RCS] = 0.5; s.v[RSA] = 5; s.v[RJA] = 62;
}
static void rows(const CalcState &, RowList &r)
{
    r.in("P", P, U_W, ST_125, T_P, "Puissance dissipée par le composant");
    r.in("T_{j\\max}", TJM, U_DEGC, ST_LIN5, 0, "Température de jonction maximale (notice)", 0);
    r.in("T_a", TA, U_DEGC, ST_LIN5, 0, "Température ambiante (dans le boîtier de l'appareil)", 0);
    r.in("R_{th\\,jc}", RJC, U_CW, ST_LIN01, T_JC, "Jonction → boîtier (notice)");
    r.in("R_{th\\,cs}", RCS, U_CW, ST_LIN01, T_CS, "Boîtier → dissipateur (isolant, graisse)", RF_NONNEG);
    r.in("R_{th\\,sa}", RSA, U_CW, ST_LIN01, T_SA, "Dissipateur choisi → air");
    r.in("R_{th\\,ja}", RJA, U_CW, ST_LIN1, 0, "Jonction → air sans dissipateur (notice)");
    r.head("Résultats");
    r.out("R_{th\\,sa\\,\\max}", ORSAM, U_CW, "(T_jmax − T_a)/P − R_jc − R_cs", true, T_SA);
    r.out("T_j", OTJ, U_DEGC, "Avec le dissipateur choisi : T_a + P (R_jc + R_cs + R_sa)", true);
    r.out("T_c", OTC, U_DEGC, "Température du boîtier");
    r.out("T_s", OTS, U_DEGC, "Température du dissipateur");
    r.out("T_j\\ (\\text{sans})", OTJ0, U_DEGC, "Sans dissipateur : T_a + P R_ja");
    r.msg(0);
}
static void calc(CalcState &s)
{
    double p = s.v[P];
    s.v[ORSAM] = (s.v[TJM] - s.v[TA]) / p - s.v[RJC] - s.v[RCS];
    s.v[OTS] = s.v[TA] + p * s.v[RSA];
    s.v[OTC] = s.v[OTS] + p * s.v[RCS];
    s.v[OTJ] = s.v[OTC] + p * s.v[RJC];
    s.v[OTJ0] = s.v[TA] + p * s.v[RJA];
    if (s.v[ORSAM] <= 0) setMsg(s, 0, "Impossible : même un dissipateur parfait ne suffit pas", ML_BAD);
    else if (s.v[OTJ] > s.v[TJM]) setMsg(s, 0, "Dissipateur choisi insuffisant : T_j > T_jmax", ML_BAD);
    else if (s.v[OTJ0] <= s.v[TJM]) setMsg(s, 0, "Pas besoin de dissipateur (T_j sans < T_jmax)", ML_OK);
    else setMsg(s, 0, "Dissipateur nécessaire et suffisant", ML_OK);
}
static void sch(Sch &g, const CalcState &s)
{
    // analogie electrique : P = courant, temperatures = potentiels
    const int x = 96, y0 = 20, y1 = 62, y2 = 98, y3 = 134, Y = 158;
    g.vsrc(34, y0, Y, 2, T_P);
    g.tex(48, 96, "P", tex::LEFT, T_P);
    g.wire(34, y0, x, y0);
    g.wire(34, Y, x, Y);
    g.res(x, y0, x, y1, T_JC);
    g.res(x, y1, x, y2, T_CS);
    g.res(x, y2, x, y3, T_SA);
    g.wire(x, y3, x, Y);
    g.dot(x, y0);
    g.dot(x, y1);
    g.dot(x, y2);
    g.dot(x, y3);
    g.texSmall(x - 8, 45, "R_{jc}", tex::RIGHT, T_JC);
    g.texSmall(x - 8, 84, "R_{cs}", tex::RIGHT, T_CS);
    g.texSmall(x - 8, 120, "R_{sa}", tex::RIGHT, T_SA);
    const double tv[4] = {s.v[OTJ], s.v[OTC], s.v[OTS], s.v[TA]};
    const char *nm[4] = {"T_j", "T_c", "T_s", "T_a"};
    const int ys[4] = {y0, y1, y2, y3};
    for (int i = 0; i < 4; i++) {
        g.texSmall(x + 8, ys[i] + 4, nm[i], tex::LEFT);
        valText(g, x + 30, ys[i] + 4, tv[i], U_DEGC, 0, i == 0);
    }
}
}  // namespace th

// ==========================================================================
// Energie stockee (condensateur, bobine)
// ==========================================================================
namespace nrj {
enum { C, U, L, I, OWC, OQ, OWL, OPHI };
enum { T_C = 1, T_L };
static void def(CalcState &s) { s.v[C] = 1000e-6; s.v[U] = 25; s.v[L] = 10e-3; s.v[I] = 2; }
static void rows(const CalcState &, RowList &r)
{
    r.head("Condensateur");
    r.in("C", C, U_F, ST_E12, T_C);
    r.in("U", U, U_V, ST_LIN1, T_C, "Tension aux bornes", 0);
    r.out("W_C", OWC, U_J, "Énergie stockée ½ C U²", true, T_C);
    r.out("Q", OQ, U_COUL, "Charge Q = C U", false, T_C);
    r.head("Bobine");
    r.in("L", L, U_H, ST_E12, T_L);
    r.in("I", I, U_A, ST_125, T_L, "Courant traversant la bobine", 0);
    r.out("W_L", OWL, U_J, "Énergie stockée ½ L I²", true, T_L);
    r.out("\\Phi", OPHI, U_WB, "Flux propre L I", false, T_L);
}
static void calc(CalcState &s)
{
    s.v[OWC] = 0.5 * s.v[C] * s.v[U] * s.v[U];
    s.v[OQ] = s.v[C] * s.v[U];
    s.v[OWL] = 0.5 * s.v[L] * s.v[I] * s.v[I];
    s.v[OPHI] = s.v[L] * s.v[I];
}
static void sch(Sch &g, const CalcState &s)
{
    g.cap(50, 40, 50, 120, T_C);
    g.term(50, 40);
    g.term(50, 120);
    g.tex(40, 84, "C", tex::RIGHT, T_C);
    g.varrow(70, 112, 48, "U", 1, T_C);
    g.ind(130, 40, 130, 120, T_L);
    g.term(130, 40);
    g.term(130, 120);
    g.tex(122, 84, "L", tex::RIGHT, T_L);
    g.iarrow(130, 34, 0, 1, "I", 138, 30, T_L);
    valText(g, 50, 150, s.v[OWC], U_J, 1, true);
    valText(g, 130, 150, s.v[OWL], U_J, 1, true);
}
}  // namespace nrj

extern const CalcDef g_calcBase[] = {
    {"Loi d'Ohm et puissance", "U=R\\,I,\\ P=U\\,I", "ohm puissance tension courant resistance watt", 6, G_NONE, 0,
     ohm::def, ohm::rows, ohm::calc, ohm::sch, nullptr, nullptr, nullptr, nullptr, 0},
    {"Diviseur de tension", "V_s=V_e\\,R_2/(R_1+R_2)", "diviseur tension pont resistances charge", 6, G_NONE, 0,
     div::def, div::rows, div::calc, div::sch, nullptr, nullptr, nullptr, nullptr, 0},
    {"Pont de Wheatstone", "#Équilibre : R₁R₄ = R₂R₃", "wheatstone pont mesure equilibre jauge", 6, G_NONE, 0,
     wh::def, wh::rows, wh::calc, wh::sch, nullptr, nullptr, nullptr, nullptr, 0},
    {"Résistance d'une LED", "R=(V_{CC}-n\\,V_F)/I_F", "led diode electroluminescente resistance serie", 6, G_NONE, 0,
     led::def, led::rows, led::calc, led::sch, nullptr, nullptr, nullptr, nullptr, 0},
    {"Décibels", "G=20\\log(V_s/V_e)", "decibel db gain attenuation rapport", 6, G_NONE, 0,
     dbr::def, dbr::rows, dbr::calc, dbr::sch, nullptr, nullptr, nullptr, nullptr, 0},
    {"Niveaux dBm, dBV, dBu", "#Niveaux absolus d'un signal", "dbm dbv dbu niveau absolu puissance", 6, G_NONE, 0,
     dbl::def, dbl::rows, dbl::calc, dbl::sch, nullptr, nullptr, nullptr, nullptr, 0},
    {"Dissipateur thermique", "T_j=T_a+P\\sum R_{th}", "dissipateur thermique radiateur temperature jonction", 6, G_NONE, 0,
     th::def, th::rows, th::calc, th::sch, nullptr, nullptr, nullptr, nullptr, 0},
    {"Énergie stockée", "W=\\tfrac12CU^2,\\ \\tfrac12LI^2", "energie condensateur bobine charge joule", 6, G_NONE, 0,
     nrj::def, nrj::rows, nrj::calc, nrj::sch, nullptr, nullptr, nullptr, nullptr, 0},
};
extern const int g_calcBaseN = sizeof(g_calcBase) / sizeof(g_calcBase[0]);
