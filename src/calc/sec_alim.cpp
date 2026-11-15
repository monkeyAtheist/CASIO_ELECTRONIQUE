// Section : alimentations (LM317, regulation Zener, redressement et filtrage).
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
    font::drawText(*g.cv, f, g.ox + x, g.oy + y, buf, g.volt);
}

// ==========================================================================
// Regulateur ajustable LM317
// ==========================================================================
namespace lm {
enum { R1, R2, VREF, IADJ, VE, IS, VST, OVS, OIR1, OPD, OR2E, OVSE, ODROP };
enum { CMODE };
enum { T_R1 = 1, T_R2, T_U, T_VE, T_VS };
static void def(CalcState &s)
{
    s.v[R1] = 240; s.v[R2] = 1.5e3; s.v[VREF] = 1.25; s.v[IADJ] = 50e-6; s.v[VE] = 15; s.v[IS] = 0.5; s.v[VST] = 5;
}
static double vsOf(const CalcState &s, double r2) { return s.v[VREF] * (1 + r2 / s.v[R1]) + s.v[IADJ] * r2; }
static void rows(const CalcState &s, RowList &r)
{
    bool solve = s.c[CMODE] == 1;
    r.choice("#Calculer", CMODE, "V_s|R_2 (V_s voulu)", 0, "Tension de sortie, ou R₂ à choisir");
    r.in("R_1", R1, U_OHM, ST_E24, T_R1, "Entre OUT et ADJ (240 Ω usuel)");
    if (!solve) r.in("R_2", R2, U_OHM, ST_E24, T_R2, "Entre ADJ et la masse");
    else r.in("V_s", VST, U_V, ST_LIN01, T_VS, "Tension de sortie voulue (≥ 1,25 V)");
    r.in("V_e", VE, U_V, ST_LIN1, T_VE, "Tension d'entrée (non régulée)");
    r.in("I_s", IS, U_A, ST_125, T_VS, "Courant consommé par la charge", RF_NONNEG);
    r.in("V_{ref}", VREF, U_V, ST_NONE, T_U, "Référence interne entre OUT et ADJ (1,25 V)");
    r.in("I_{adj}", IADJ, U_A, ST_NONE, T_U, "Courant de la broche ADJ (≈ 50 µA)", RF_NONNEG);
    r.head("Résultats");
    if (solve) {
        r.out("R_2", R2, U_OHM, "R₂ = (V_s − V_ref)/(V_ref/R₁ + I_adj)", true, T_R2);
        r.out("#E24 proche", OR2E, U_OHM, "Valeur normalisée la plus proche", false, T_R2);
        r.out("V_s\\ (E24)", OVSE, U_V, "Tension obtenue avec la valeur E24");
    }
    r.out("V_s", OVS, U_V, "V_s = V_ref (1 + R₂/R₁) + I_adj R₂", !solve, T_VS);
    r.out("I_{R1}", OIR1, U_A, "Courant dans le pont V_ref/R₁ (au moins 5 mA)", false, T_R1);
    r.out("V_e-V_s", ODROP, U_V, "Chute dans le régulateur (au moins 3 V)");
    r.out("P", OPD, U_W, "Puissance dissipée (V_e − V_s) I_s", true, T_U);
    r.msg(0);
    r.msg(1);
}
static void calc(CalcState &s)
{
    if (s.c[CMODE] == 1) {
        double r2 = (s.v[VST] - s.v[VREF]) / (s.v[VREF] / s.v[R1] + s.v[IADJ]);
        if (r2 > 0) {
            s.v[R2] = r2;
            s.v[OR2E] = eNearest(E24, r2);
            s.v[OVSE] = vsOf(s, s.v[OR2E]);
        } else {
            s.v[OR2E] = s.v[OVSE] = NaN();
        }
    }
    s.v[OVS] = vsOf(s, s.v[R2]);
    s.v[OIR1] = s.v[VREF] / s.v[R1];
    s.v[ODROP] = s.v[VE] - s.v[OVS];
    s.v[OPD] = s.v[ODROP] * s.v[IS];
    if (s.c[CMODE] == 1 && s.v[VST] <= s.v[VREF]) setMsg(s, 0, "Impossible : V_s doit dépasser V_ref", ML_BAD);
    else if (s.v[ODROP] < 3) setMsg(s, 0, "Chute < 3 V : régulation non garantie", ML_WARN);
    else setMsg(s, 0, "Chute suffisante pour réguler", ML_OK);
    if (s.v[OPD] > 1.5) setMsg(s, 1, "P > 1,5 W : dissipateur nécessaire", ML_WARN);
    else if (s.v[OIR1] < 5e-3) setMsg(s, 1, "R₁ trop grande : courant minimal non assuré", ML_WARN);
    else setMsg(s, 1, nullptr, ML_INFO);
}
static void sch(Sch &g, const CalcState &s)
{
    const int Y = 152, cx = 52, cy = 28, cw = 70, ch = 54, yo = cy + 14, xa = cx + cw / 2;
    g.chip(cx, cy, cw, ch, nullptr, T_U);
    tex::draw(*g.cv, "#LM317", g.ox + xa, g.oy + cy + 35, g.on(T_U) ? g.accent : g.ink, tex::CENTER, 1);
    g.texSmall(cx + 3, yo + 4, "#IN", tex::LEFT);
    g.texSmall(cx + cw - 3, yo + 4, "#OUT", tex::RIGHT);
    g.texSmall(xa, cy + ch - 2, "#ADJ", tex::CENTER);
    g.wire(14, yo, cx, yo);
    g.wire(cx + cw, yo, 166, yo);
    g.wire(xa, cy + ch, xa, 100);
    g.wire(144, yo, 144, 100);
    g.dot(144, yo);
    g.res(144, 100, xa, 100, T_R1);
    g.tex(116, 92, "R_1", tex::CENTER, T_R1);
    g.dot(xa, 100);
    g.res(xa, 100, xa, Y, T_R2);
    g.tex(xa - 9, 132, "R_2", tex::RIGHT, T_R2);
    groundRail(g, 14, 166, Y, xa);
    termIn(g, 14, yo, Y, "V_e", T_VE);
    termOut(g, 166, yo, Y, "V_s", T_VS);
    valText(g, 176, yo - 5, s.v[OVS], U_V, 2, true);
}
}  // namespace lm

// ==========================================================================
// Regulation par diode Zener
// ==========================================================================
namespace zen {
enum { VEMIN, VEMAX, VZ, IZMIN, ILMAX, ORSM, ORS, OIZM, OPZ, OPR, OIZL };
enum { T_RS = 1, T_Z, T_RL, T_VE };
static void def(CalcState &s) { s.v[VEMIN] = 10; s.v[VEMAX] = 14; s.v[VZ] = 5.1; s.v[IZMIN] = 5e-3; s.v[ILMAX] = 30e-3; }
static void rows(const CalcState &, RowList &r)
{
    r.in("V_{e\\,\\min}", VEMIN, U_V, ST_LIN1, T_VE, "Tension d'entrée la plus basse");
    r.in("V_{e\\,\\max}", VEMAX, U_V, ST_LIN1, T_VE, "Tension d'entrée la plus haute");
    r.in("V_Z", VZ, U_V, ST_LIN01, T_Z, "Tension de la diode Zener");
    r.in("I_{Z\\min}", IZMIN, U_A, ST_125, T_Z, "Courant minimal pour réguler (notice, ≈ 5 mA)");
    r.in("I_{L\\max}", ILMAX, U_A, ST_125, T_RL, "Courant maximal de la charge", RF_NONNEG);
    r.head("Résultats");
    r.out("R_{s\\max}", ORSM, U_OHM, "(V_e,min − V_Z)/(I_L,max + I_Z,min)", false, T_RS);
    r.out("R_s", ORS, U_OHM, "Valeur E24 inférieure ou égale", true, T_RS);
    r.out("I_Z\\ (V_{e\\min})", OIZL, U_A, "Courant Zener au pire cas (pleine charge)", false, T_Z);
    r.out("I_{Z\\max}", OIZM, U_A, "Sans charge, à V_e,max", false, T_Z);
    r.out("P_Z", OPZ, U_W, "Puissance maximale dans la Zener V_Z·I_Z,max", true, T_Z);
    r.out("P_{R_s}", OPR, U_W, "Puissance maximale dans R_s", false, T_RS);
    r.msg(0);
}
static void calc(CalcState &s)
{
    double vz = s.v[VZ];
    if (!(s.v[VEMIN] > vz) || s.v[VEMAX] < s.v[VEMIN]) {
        s.v[ORSM] = s.v[ORS] = s.v[OIZM] = s.v[OPZ] = s.v[OPR] = s.v[OIZL] = NaN();
        setMsg(s, 0, s.v[VEMAX] < s.v[VEMIN] ? "Il faut V_e,max ≥ V_e,min" : "Il faut V_e,min > V_Z", ML_BAD);
        return;
    }
    s.v[ORSM] = (s.v[VEMIN] - vz) / (s.v[ILMAX] + s.v[IZMIN]);
    s.v[ORS] = eFloor(E24, s.v[ORSM]);
    double rs = s.v[ORS];
    s.v[OIZL] = (s.v[VEMIN] - vz) / rs - s.v[ILMAX];
    s.v[OIZM] = (s.v[VEMAX] - vz) / rs;
    s.v[OPZ] = vz * s.v[OIZM];
    s.v[OPR] = (s.v[VEMAX] - vz) * (s.v[VEMAX] - vz) / rs;
    setMsg(s, 0, "Choisir une Zener de puissance ≥ 2 P_Z", ML_INFO);
}
static void sch(Sch &g, const CalcState &s)
{
    const int Y = 150, y = 50, xn = 96;
    g.res(14, y, xn, y, T_RS);
    g.tex(55, y - 10, "R_s", tex::CENTER, T_RS);
    g.diode(xn, Y, xn, y, 2, T_Z);
    g.tex(xn - 12, 104, "D_Z", tex::RIGHT, T_Z);
    g.dot(xn, y);
    g.wire(xn, y, 166, y);
    g.res(136, y, 136, Y, T_RL);
    g.tex(128, 120, "R_L", tex::RIGHT, T_RL);
    g.dot(136, y);
    g.dot(136, Y);
    groundRail(g, 14, 166, Y, xn);
    termIn(g, 14, y, Y, "V_e", T_VE);
    termOut(g, 166, y, Y, "V_Z", T_Z);
    valText(g, 55, y + 18, s.v[ORS], U_OHM, 1, true);
}
}  // namespace zen

// ==========================================================================
// Redressement et filtrage par condensateur
// ==========================================================================
namespace redr {
enum { VEFF, F, VD, C, IL, DVM, OVC, ODV, OVMIN, OVMOY, OTX, OCMIN, OVINV, OFR };
enum { CTYPE };
enum { T_SRC = 1, T_D, T_C, T_RL };
static void def(CalcState &s) { s.v[VEFF] = 12; s.v[F] = 50; s.v[VD] = 0.7; s.v[C] = 2200e-6; s.v[IL] = 0.5; s.v[DVM] = 1; s.c[CTYPE] = 1; }
static int nd(const CalcState &s) { return s.c[CTYPE] == 1 ? 2 : 1; }
static double fr(const CalcState &s) { return s.c[CTYPE] == 0 ? s.v[F] : 2 * s.v[F]; }
static void rows(const CalcState &, RowList &r)
{
    r.choice("#Montage", CTYPE, "simple alternance|pont de Graetz|point milieu", T_D, "Type de redressement");
    r.in("V_{\\text{eff}}", VEFF, U_V, ST_LIN1, T_SRC, "Tension efficace au secondaire (chaque demi-enroulement en point milieu)");
    r.in("f", F, U_HZ, ST_LIN10, T_SRC, "Fréquence du secteur");
    r.in("V_D", VD, U_V, ST_LIN01, T_D, "Chute de tension d'une diode", RF_NONNEG);
    r.in("C", C, U_F, ST_E12, T_C, "Condensateur de filtrage");
    r.in("I", IL, U_A, ST_125, T_RL, "Courant moyen de la charge", RF_NONNEG);
    r.in("\\Delta V_{\\max}", DVM, U_V, ST_LIN01, T_C, "Ondulation acceptée (pour calculer C)");
    r.head("Résultats");
    r.out("V_{\\max}", OVC, U_V, "Crête : V_eff √2 − n V_D", true);
    r.out("f_{\\text{ond}}", OFR, U_HZ, "Fréquence de l'ondulation (f ou 2f)");
    r.out("\\Delta V", ODV, U_V, "Ondulation crête à crête ≈ I / (f_ond C)", true, T_C);
    r.out("V_{\\min}", OVMIN, U_V, "V_max − ΔV");
    r.out("V_{\\text{moy}}", OVMOY, U_V, "Tension moyenne ≈ V_max − ΔV/2", true);
    r.out("#Ondulation", OTX, U_PCT, "ΔV / V_moy");
    r.out("C_{\\min}", OCMIN, U_F, "C pour ΔV = ΔV_max : I / (f_ond ΔV_max)", false, T_C);
    r.out("V_{\\text{inv}}", OVINV, U_V, "Tension inverse maximale sur une diode", false, T_D);
    r.msg(0);
}
static void calc(CalcState &s)
{
    double vm = s.v[VEFF] * fm::sqrt(2.0);
    s.v[OVC] = vm - nd(s) * s.v[VD];
    s.v[OFR] = fr(s);
    s.v[ODV] = s.v[IL] / (fr(s) * s.v[C]);
    s.v[OVMIN] = s.v[OVC] - s.v[ODV];
    s.v[OVMOY] = s.v[OVC] - s.v[ODV] / 2;
    s.v[OTX] = s.v[ODV] / s.v[OVMOY] * 100;
    s.v[OCMIN] = s.v[IL] / (fr(s) * s.v[DVM]);
    s.v[OVINV] = s.c[CTYPE] == 1 ? vm : 2 * vm;
    if (s.v[OVMIN] <= 0) setMsg(s, 0, "Ondulation trop forte : augmenter C", ML_BAD);
    else if (s.v[OTX] > 20) setMsg(s, 0, "Ondulation importante (> 20 %)", ML_WARN);
    else setMsg(s, 0, "Graphe : tension redressée et filtrée", ML_INFO);
}
static void sch(Sch &g, const CalcState &s)
{
    const int yt = 40, Y = 140, xc = 128, xl = 162;
    int t = s.c[CTYPE];
    if (t == 0) {
        g.vsrc(24, yt, Y, 1, T_SRC);
        g.diode(24, yt, 90, yt, 0, T_D);
        g.tex(57, yt - 10, "D", tex::CENTER, T_D);
        g.wire(90, yt, xl, yt);
        g.wire(24, Y, xl, Y);
    } else if (t == 1) {
        const int xa = 30, xb = 104, ym = 90;
        g.diode(xa, Y, xa, ym, 0, T_D);
        g.diode(xa, ym, xa, yt, 0, T_D);
        g.diode(xb, Y, xb, ym, 0, T_D);
        g.diode(xb, ym, xb, yt, 0, T_D);
        g.acH(xa, xb, ym, T_SRC);
        g.dot(xa, ym);
        g.dot(xb, ym);
        g.wire(xa, yt, xl, yt);
        g.wire(xa, Y, xl, Y);
        g.dot(xb, yt);
        g.dot(xb, Y);
    } else {
        // point milieu : deux demi-enroulements, deux diodes
        const int ym = 90, xd = 62;
        g.vsrc(24, yt, ym, 1, T_SRC);
        g.vsrc(24, ym, Y, 1, T_SRC);
        g.diode(24, yt, xd, yt, 0, T_D);
        g.wire(24, Y, 44, Y);
        g.diode(44, Y, 44, 100, 0, T_D);
        g.wire(44, 100, 44, 70);
        g.wire(44, 70, xd, 70);
        g.wire(xd, 70, xd, yt);
        g.dot(xd, yt);
        g.wire(xd, yt, xl, yt);
        g.wireVH(24, ym, 8, ym);
        g.wire(8, ym, 8, 160);
        g.wire(8, 160, xl, 160);
        g.wire(xl, 160, xl, Y);
        g.wire(xc, Y, xl, Y);
    }
    g.cap(xc, yt, xc, Y, T_C);
    g.res(xl, yt, xl, Y, T_RL);
    g.dot(xc, yt);
    g.dot(xc, Y);
    g.tex(xc - 9, 96, "C", tex::RIGHT, T_C);
    g.texSmall(xl, yt - 5, "R_L", tex::CENTER, T_RL);
    g.gnd(t == 2 ? 100 : 80, t == 2 ? 160 : Y);
}
static void graph(Canvas &cv, const CalcState &s, const GraphArea &a, uint16_t accent)
{
    double vm = s.v[VEFF] * fm::sqrt(2.0), drop = nd(s) * s.v[VD];
    double T = 1.0 / s.v[F], tmax = 3 * T;
    int X0 = a.x + 44, X1 = a.x + a.w - 10, Y0 = a.y + 22, Y1 = a.y + a.h - 22;
    double top = vm * 1.08, bot = s.c[CTYPE] == 0 ? -vm * 0.15 : -vm * 0.05;
    auto pyf = [&](double v) { return fm::clampc(Y1 - (v - bot) / (top - bot) * (Y1 - Y0)); };
    uint16_t axc = rgb(150, 156, 170), thin = mix16(accent, col::white, 9);
    cv.fillRect(X0, (int)pyf(0), X1 - X0, 1, axc);
    cv.fillRect(X0, Y0, 1, Y1 - Y0, axc);
    char buf[32];
    for (int k = 1; k <= 3; k++) {
        int x = X0 + (X1 - X0) * k / 3;
        for (int y = Y0; y < Y1; y += 3) cv.fillRect(x, y, 1, 1, axc);
        int n = itoaSimple(k, buf);
        scopy(buf + n, 8, "T");
        font::drawText(cv, SANS_12, x - 8, Y1 + 14, buf, col::inkSoft);
    }
    const int N = 300;
    double v = s.v[OVC] > 0 ? s.v[OVC] : 0, dt = tmax / N;
    int px = 0, pyr = 0, pyc = 0;
    for (int i = 0; i <= N; i++) {
        double t = i * dt;
        double sn = fm::sin(fm::TWO_PI * s.v[F] * t);
        double vr = s.c[CTYPE] == 0 ? (sn > 0 ? vm * sn : 0) : vm * fm::fabs(sn);
        vr = vr > drop ? vr - drop : 0;
        // le condensateur se decharge a courant constant, se recharge par les diodes
        if (i) v -= s.v[IL] * dt / s.v[C];
        if (vr > v) v = vr;
        if (v < 0) v = 0;
        int x = (int)((X0 + (X1 - X0) * (double)i / N) * 16);
        int yr = (int)(pyf(vr) * 16), yc = (int)(pyf(v) * 16);
        if (i) {
            cv.lineSub(px, pyr, x, yr, thin);
            cv.lineSub(px, pyc, x, yc, accent);
            cv.lineSub(px, pyc + 10, x, yc + 10, accent);
        }
        px = x;
        pyr = yr;
        pyc = yc;
    }
    double lv[2] = {s.v[OVC], s.v[OVMIN]};
    for (int i = 0; i < 2; i++) {
        fmtValue(buf, sizeof buf, lv[i], U_V, 3);
        font::drawText(cv, SANS_12, X0 - 4 - font::width(SANS_12, buf), (int)pyf(lv[i]) + 4, buf, col::inkSoft);
    }
    int k = scopy(buf, sizeof buf, "\xCE\x94V = ");
    fmtValue(buf + k, (int)sizeof(buf) - k, s.v[ODV], U_V);
    font::drawText(cv, BOLD_12, X1 - font::width(BOLD_12, buf), a.y + 14, buf, accent);
}
}  // namespace redr

extern const CalcDef g_calcAlim[] = {
    {"Régulateur LM317", "V_s=1{,}25\\,(1+R_2/R_1)", "lm317 regulateur ajustable alimentation", 7, G_NONE, 0,
     lm::def, lm::rows, lm::calc, lm::sch, nullptr, nullptr, nullptr, nullptr, 0},
    {"Régulation par diode Zener", "R_s=(V_e-V_Z)/(I_L+I_Z)", "zener regulation diode stabilisation", 7, G_NONE, 0,
     zen::def, zen::rows, zen::calc, zen::sch, nullptr, nullptr, nullptr, nullptr, 0},
    {"Redressement et filtrage", "\\Delta V\\approx I/(f\\,C)", "redressement filtrage pont graetz condensateur ondulation diode", 7, G_CUSTOM, 0,
     redr::def, redr::rows, redr::calc, redr::sch, nullptr, nullptr, redr::graph, "Courbes", 0},
};
extern const int g_calcAlimN = sizeof(g_calcAlim) / sizeof(g_calcAlim[0]);
