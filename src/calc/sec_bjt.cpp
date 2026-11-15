// Section : transistors bipolaires (polarisation, commutation, petits signaux).
//
// Les grandeurs sont calculees en valeurs absolues : un PNP se traite comme
// un NPN "retourne" (V_EB, V_EC, courants sortants de la base).
#include "CalcUtil.hpp"

using namespace cu;

static const char *const DESC_BETA = "Gain en courant β = I_C/I_B (h21E de la notice)";
static const char *const DESC_VBE = "Tension base-émetteur en conduction (0,6 à 0,7 V)";
static const char *const DESC_VCES = "Tension collecteur-émetteur de saturation";
static const char *const DESC_VT = "Tension thermique kT/q (≈ 26 mV à 25 °C)";

// Petite etiquette de valeur sur le schema : "V_C = 5,77 V" (name nul : valeur seule).
static void nodeVal(Sch &g, int x, int y, const char *name, double v, uint8_t unit, int align = 0)
{
    char buf[40];
    int k = 0;
    buf[0] = 0;
    if (name) {
        k = scopy(buf, sizeof buf, name);
        k += scopy(buf + k, (int)sizeof(buf) - k, " = ");
    }
    fmtValue(buf + k, (int)sizeof(buf) - k, v, unit, 3);
    int w = font::width(SANS_12, buf);
    int xx = align == 2 ? x - w : align == 1 ? x - w / 2 : x;
    if (xx + w > 177) xx = 177 - w;
    font::drawText(*g.cv, SANS_12, g.ox + xx, g.oy + y, buf, g.volt);
}

// Polarisation d'un etage : I_B depuis le generateur de Thevenin (vth, rth),
// avec saturation eventuelle. Renvoie 0 bloque, 1 lineaire, 2 sature.
struct Bias {
    double ib, ic, ie, vce;
    int mode;
};
static Bias solveBias(double vcc, double vth, double rth, double rc, double re, double beta, double vbe,
                      double vces)
{
    Bias b;
    b.ib = (vth - vbe) / (rth + (beta + 1) * re);
    if (!(b.ib > 0)) {
        b.ib = b.ic = b.ie = 0;
        b.vce = vcc;
        b.mode = 0;
        return b;
    }
    b.ic = beta * b.ib;
    b.ie = b.ib + b.ic;
    b.vce = vcc - rc * b.ic - re * b.ie;
    b.mode = 1;
    if (b.vce < vces) {
        // sature : V_CE = V_CEsat, I_C fixe par le circuit
        double ib = (vth - vbe - re * (vcc - vces) / (rc + re)) / (rth + re * rc / (rc + re));
        if (ib < 0) ib = 0;
        double ic = (vcc - vces - re * ib) / (rc + re);
        if (ic < 0) ic = 0;
        b.ib = ib;
        b.ic = ic;
        b.ie = ib + ic;
        b.vce = vces;
        b.mode = 2;
    }
    return b;
}

static void modeMsg(CalcState &s, int k, const Bias &b, double beta)
{
    static char buf[64];
    if (b.mode == 0) setMsg(s, k, "Bloqué : la jonction base-émetteur ne conduit pas", ML_WARN);
    else if (b.mode == 2) {
        int n = scopy(buf, sizeof buf, "Saturé : β forcé = ");
        fmtNumber(buf + n, (int)sizeof(buf) - n, b.ib > 0 ? b.ic / b.ib : 0, 3);
        setMsg(s, k, buf, ML_WARN);
    } else if (b.vce < 1.0) {
        setMsg(s, k, "Linéaire, mais proche de la saturation", ML_WARN);
    } else {
        (void)beta;
        setMsg(s, k, "Régime linéaire (actif)", ML_OK);
    }
}

// Schema commun : pont (ou R_B) a gauche, transistor, R haut et R bas.
// topTag / botTag : resistances au-dessus et au-dessous du transistor.
struct BjtDraw {
    bool npn, divider, ce;
    int tR1, tR2, tTop, tBot, tQ, tCe;
    const char *lTop, *lBot, *lR1;
};
static void drawBjt(Sch &g, const BjtDraw &d)
{
    const int yt = 18, Y = 152, xb = 40, bx = 84, by = 86;
    const int xq = bx + 17;
    int xr = d.ce ? 140 : xq;
    g.wire(xb, yt, xr, yt);
    g.tex(xr, yt - 4, "V_{CC}", tex::RIGHT);
    g.wire(xb, Y, xr, Y);
    g.dot(72, Y);
    g.gnd(72, Y);
    if (d.divider) {
        g.res(xb, yt, xb, by, d.tR1);
        g.res(xb, by, xb, Y, d.tR2);
        g.tex(xb - 9, 56, d.lR1, tex::RIGHT, d.tR1);
        g.tex(xb - 9, 124, "R_2", tex::RIGHT, d.tR2);
        g.dot(xb, by);
        g.dot(xb, yt);
        g.dot(xb, Y);
    } else if (d.npn) {
        g.res(xb, yt, xb, by, d.tR1);
        g.tex(xb - 9, 56, d.lR1, tex::RIGHT, d.tR1);
        g.wire(xb, by, xb, by);
        g.dot(xb, yt);
    } else {
        g.res(xb, by, xb, Y, d.tR1);
        g.tex(xb - 9, 124, d.lR1, tex::RIGHT, d.tR1);
        g.dot(xb, Y);
    }
    g.wire(xb, by, bx, by);
    g.bjt(bx, by, d.npn, d.tQ);
    g.res(xq, yt, xq, by - 20, d.tTop);
    g.res(xq, by + 20, xq, Y, d.tBot);
    g.tex(xq + 9, 50, d.lTop, tex::LEFT, d.tTop);
    g.tex(xq + 9, 130, d.lBot, tex::LEFT, d.tBot);
    g.dot(xq, yt);
    g.dot(xq, Y);
    if (d.ce) {
        if (d.npn) {
            g.wire(xq, by + 30, 140, by + 30, d.tCe);
            g.cap(140, by + 30, 140, Y, d.tCe);
            g.dot(xq, by + 30);
            g.tex(146, 132, "C_E", tex::LEFT, d.tCe);
        } else {
            g.wire(xq, by - 30, 140, by - 30, d.tCe);
            g.cap(140, by - 30, 140, yt, d.tCe);
            g.dot(xq, by - 30);
            g.tex(146, 46, "C_E", tex::LEFT, d.tCe);
        }
        g.dot(140, d.npn ? Y : yt);
    }
}

// ==========================================================================
// Polarisation par pont diviseur (emetteur commun)
// ==========================================================================
namespace pont {
enum { VCC, R1, R2, RC, RE, BETA, VBE, VCES, VT, OVTH, ORTH, OIB, OIC, OIE, OVB, OVE, OVC, OVCE, OP,
       OGM, ORPI, OAV, OZE, OZS, OIP };
enum { CTYPE, CCE };
enum { T_R1 = 1, T_R2, T_RC, T_RE, T_Q, T_CE };
static void def(CalcState &s)
{
    s.v[VCC] = 12; s.v[R1] = 47e3; s.v[R2] = 10e3; s.v[RC] = 2.2e3; s.v[RE] = 470;
    s.v[BETA] = 200; s.v[VBE] = 0.65; s.v[VCES] = 0.2; s.v[VT] = 26e-3; s.c[CCE] = 1;
}
static void rows(const CalcState &s, RowList &r)
{
    bool npn = s.c[CTYPE] == 0;
    r.choice("#Transistor", CTYPE, "NPN|PNP", T_Q);
    r.in("V_{CC}", VCC, U_V, ST_LIN1, 0, "Tension d'alimentation");
    r.in("R_1", R1, U_OHM, ST_E24, T_R1, "Pont : entre +V_CC et la base");
    r.in("R_2", R2, U_OHM, ST_E24, T_R2, "Pont : entre la base et la masse");
    r.in("R_C", RC, U_OHM, ST_E24, T_RC, "Résistance de collecteur");
    r.in("R_E", RE, U_OHM, ST_E24, T_RE, "Résistance d'émetteur (stabilise le point de repos)", RF_NONNEG);
    r.in("\\beta", BETA, U_NONE, ST_LIN10, T_Q, DESC_BETA);
    r.in(npn ? "V_{BE}" : "V_{EB}", VBE, U_V, ST_LIN01, T_Q, DESC_VBE);
    r.in(npn ? "V_{CE,\\text{sat}}" : "V_{EC,\\text{sat}}", VCES, U_V, ST_LIN01, T_Q, DESC_VCES, RF_NONNEG);
    r.head("Point de repos");
    r.out("V_{th}", OVTH, U_V, npn ? "Thévenin du pont : V_CC R₂/(R₁+R₂)" : "Thévenin du pont (par rapport à la masse)");
    r.out("R_{th}", ORTH, U_OHM, "R₁ ∥ R₂");
    r.out("I_B", OIB, U_A, "I_B = (V_th − V_BE)/(R_th + (β+1) R_E)");
    r.out("I_C", OIC, U_A, "Courant de collecteur", true, T_RC);
    r.out("I_E", OIE, U_A, "I_E = I_B + I_C", false, T_RE);
    r.out("V_B", OVB, U_V, "Potentiel de la base");
    r.out("V_E", OVE, U_V, "Potentiel de l'émetteur");
    r.out("V_C", OVC, U_V, "Potentiel du collecteur");
    r.out(npn ? "V_{CE}" : "V_{EC}", OVCE, U_V, "Tension collecteur-émetteur", true);
    r.out("P", OP, U_W, "Puissance dissipée par le transistor V_CE·I_C");
    r.out("I_{\\text{pont}}", OIP, U_A, "Courant dans R₂ (au moins 10 I_B conseillé)");
    r.msg(0);
    r.msg(1);
    r.head("Petits signaux");
    r.choice("C_E", CCE, "absent|R_E découplée", T_CE, "Condensateur de découplage en parallèle sur R_E");
    r.in("V_T", VT, U_V, ST_NONE, 0, DESC_VT);
    r.out("g_m", OGM, U_SIE, "Transconductance I_C/V_T");
    r.out("r_\\pi", ORPI, U_OHM, "Résistance d'entrée base-émetteur β/g_m");
    r.out("A_v", OAV, U_NONE, "Gain en tension à vide (sortie sur le collecteur)", true);
    r.out("Z_e", OZE, U_OHM, "Impédance d'entrée (pont compris)");
    r.out("Z_s", OZS, U_OHM, "Impédance de sortie ≈ R_C");
}
static void calc(CalcState &s)
{
    bool npn = s.c[CTYPE] == 0;
    double vcc = s.v[VCC], r1 = s.v[R1], r2 = s.v[R2];
    double vthG = vcc * r2 / (r1 + r2);              // par rapport a la masse
    double vth = npn ? vthG : vcc - vthG;             // tension de commande
    double rth = par2(r1, r2);
    Bias b = solveBias(vcc, vth, rth, s.v[RC], s.v[RE], s.v[BETA], s.v[VBE], s.v[VCES]);
    s.v[OVTH] = vthG;
    s.v[ORTH] = rth;
    s.v[OIB] = b.ib;
    s.v[OIC] = b.ic;
    s.v[OIE] = b.ie;
    double ve = s.v[RE] * b.ie, vc = s.v[RC] * b.ic;   // chutes dans R_E et R_C
    if (npn) {
        s.v[OVE] = ve;
        s.v[OVB] = b.mode ? ve + s.v[VBE] : vthG;
        s.v[OVC] = vcc - vc;
    } else {
        s.v[OVE] = vcc - ve;
        s.v[OVB] = b.mode ? vcc - ve - s.v[VBE] : vthG;
        s.v[OVC] = vc;
    }
    s.v[OVCE] = b.vce;
    s.v[OP] = b.vce * b.ic;
    s.v[OIP] = npn ? s.v[OVB] / r2 : (vcc - s.v[OVB]) / r1;
    modeMsg(s, 0, b, s.v[BETA]);
    if (b.mode && s.v[OIP] < 10 * b.ib) setMsg(s, 1, "Pont trop faible : I_pont < 10 I_B", ML_WARN);
    else setMsg(s, 1, "V_E fixé par le pont : point de repos stable", ML_INFO);
    // petits signaux
    if (b.mode == 1) {
        double gm = b.ic / s.v[VT], rpi = s.v[BETA] / gm;
        double reAc = s.c[CCE] ? 0 : s.v[RE];
        s.v[OGM] = gm;
        s.v[ORPI] = rpi;
        s.v[OAV] = -s.v[BETA] * s.v[RC] / (rpi + (s.v[BETA] + 1) * reAc);
        s.v[OZE] = par2(rth, rpi + (s.v[BETA] + 1) * reAc);
        s.v[OZS] = s.v[RC];
    } else {
        s.v[OGM] = s.v[ORPI] = s.v[OAV] = s.v[OZE] = s.v[OZS] = NaN();
    }
}
static void sch(Sch &g, const CalcState &s)
{
    bool npn = s.c[CTYPE] == 0;
    BjtDraw d{npn, true, s.c[CCE] == 1, T_R1, T_R2, npn ? T_RC : T_RE, npn ? T_RE : T_RC, T_Q, T_CE,
              npn ? "R_C" : "R_E", npn ? "R_E" : "R_C", "R_1"};
    drawBjt(g, d);
    nodeVal(g, 44, 100, nullptr, s.v[OVB], U_V);
    nodeVal(g, 110, npn ? 70 : 104, nullptr, s.v[OVC], U_V);
    nodeVal(g, 110, npn ? 104 : 70, nullptr, s.v[OVE], U_V);
}
}  // namespace pont

// ==========================================================================
// Polarisation par resistance de base
// ==========================================================================
namespace rbase {
enum { VCC, RB, RC, RE, BETA, VBE, VCES, OIB, OIC, OIE, OVB, OVE, OVC, OVCE, OP, OBSENS };
enum { CTYPE };
enum { T_RB = 1, T_RC, T_RE, T_Q };
static void def(CalcState &s)
{
    s.v[VCC] = 12; s.v[RB] = 470e3; s.v[RC] = 1e3; s.v[RE] = 0;
    s.v[BETA] = 200; s.v[VBE] = 0.65; s.v[VCES] = 0.2;
}
static void rows(const CalcState &s, RowList &r)
{
    bool npn = s.c[CTYPE] == 0;
    r.choice("#Transistor", CTYPE, "NPN|PNP", T_Q);
    r.in("V_{CC}", VCC, U_V, ST_LIN1, 0, "Tension d'alimentation");
    r.in("R_B", RB, U_OHM, ST_E24, T_RB, npn ? "Entre +V_CC et la base" : "Entre la base et la masse");
    r.in("R_C", RC, U_OHM, ST_E24, T_RC, "Résistance de collecteur");
    r.in("R_E", RE, U_OHM, ST_E24, T_RE, "Résistance d'émetteur (0 si absente)", RF_NONNEG);
    r.in("\\beta", BETA, U_NONE, ST_LIN10, T_Q, DESC_BETA);
    r.in(npn ? "V_{BE}" : "V_{EB}", VBE, U_V, ST_LIN01, T_Q, DESC_VBE);
    r.in(npn ? "V_{CE,\\text{sat}}" : "V_{EC,\\text{sat}}", VCES, U_V, ST_LIN01, T_Q, DESC_VCES, RF_NONNEG);
    r.head("Point de repos");
    r.out("I_B", OIB, U_A, "I_B = (V_CC − V_BE)/(R_B + (β+1) R_E)", false, T_RB);
    r.out("I_C", OIC, U_A, "Courant de collecteur β I_B", true, T_RC);
    r.out("I_E", OIE, U_A, "I_E = I_B + I_C", false, T_RE);
    r.out("V_B", OVB, U_V, "Potentiel de la base");
    r.out("V_E", OVE, U_V, "Potentiel de l'émetteur");
    r.out("V_C", OVC, U_V, "Potentiel du collecteur");
    r.out(npn ? "V_{CE}" : "V_{EC}", OVCE, U_V, "Tension collecteur-émetteur", true);
    r.out("P", OP, U_W, "Puissance dissipée V_CE·I_C");
    r.out("\\frac{\\Delta I_C}{I_C}", OBSENS, U_PCT, "Variation de I_C si β augmente de 50 %");
    r.msg(0);
}
static void calc(CalcState &s)
{
    bool npn = s.c[CTYPE] == 0;
    double vcc = s.v[VCC];
    Bias b = solveBias(vcc, vcc, s.v[RB], s.v[RC], s.v[RE], s.v[BETA], s.v[VBE], s.v[VCES]);
    Bias b2 = solveBias(vcc, vcc, s.v[RB], s.v[RC], s.v[RE], s.v[BETA] * 1.5, s.v[VBE], s.v[VCES]);
    s.v[OIB] = b.ib;
    s.v[OIC] = b.ic;
    s.v[OIE] = b.ie;
    double ve = s.v[RE] * b.ie, vc = s.v[RC] * b.ic;   // chutes dans R_E et R_C
    if (npn) {
        s.v[OVE] = ve;
        s.v[OVB] = b.mode ? ve + s.v[VBE] : vcc;
        s.v[OVC] = vcc - vc;
    } else {
        s.v[OVE] = vcc - ve;
        s.v[OVB] = b.mode ? vcc - ve - s.v[VBE] : 0;
        s.v[OVC] = vc;
    }
    s.v[OVCE] = b.vce;
    s.v[OP] = b.vce * b.ic;
    s.v[OBSENS] = b.ic > 0 ? (b2.ic - b.ic) / b.ic * 100 : NaN();
    modeMsg(s, 0, b, s.v[BETA]);
}
static void sch(Sch &g, const CalcState &s)
{
    bool npn = s.c[CTYPE] == 0;
    BjtDraw d{npn, false, false, T_RB, 0, npn ? T_RC : T_RE, npn ? T_RE : T_RC, T_Q, 0,
              npn ? "R_C" : "R_E", npn ? "R_E" : "R_C", "R_B"};
    drawBjt(g, d);
    nodeVal(g, 44, 100, nullptr, s.v[OVB], U_V);
    nodeVal(g, 110, npn ? 70 : 104, nullptr, s.v[OVC], U_V);
    nodeVal(g, 110, npn ? 104 : 70, nullptr, s.v[OVE], U_V);
}
}  // namespace rbase

// ==========================================================================
// Transistor en commutation (NPN, charge cote collecteur)
// ==========================================================================
namespace sw {
enum { VCC, RL, VIN, BETA, VBE, VCES, KS, OIC, OIBMIN, OIB, ORBMAX, ORB, OIBR, OPT, OPL };
enum { T_RB = 1, T_RL, T_Q, T_VIN };
static void def(CalcState &s)
{
    s.v[VCC] = 12; s.v[RL] = 120; s.v[VIN] = 5; s.v[BETA] = 100; s.v[VBE] = 0.75; s.v[VCES] = 0.2; s.v[KS] = 3;
}
static void rows(const CalcState &, RowList &r)
{
    r.in("V_{CC}", VCC, U_V, ST_LIN1, 0, "Alimentation de la charge");
    r.in("R_L", RL, U_OHM, ST_E24, T_RL, "Charge (relais, lampe, résistance…)");
    r.in("V_{in}", VIN, U_V, ST_LIN01, T_VIN, "Niveau haut de la commande (sortie logique)");
    r.in("\\beta_{\\min}", BETA, U_NONE, ST_LIN10, T_Q, "Gain en courant minimal de la notice");
    r.in("V_{BE,\\text{sat}}", VBE, U_V, ST_LIN01, T_Q, "V_BE en saturation (≈ 0,75 V)");
    r.in("V_{CE,\\text{sat}}", VCES, U_V, ST_LIN01, T_Q, DESC_VCES, RF_NONNEG);
    r.in("k", KS, U_NONE, ST_LIN1, T_RB, "Coefficient de sursaturation (2 à 5)");
    r.head("Résultats");
    r.out("I_C", OIC, U_A, "Courant de la charge saturée (V_CC − V_CEsat)/R_L", true, T_RL);
    r.out("I_{B\\min}", OIBMIN, U_A, "I_C / β_min");
    r.out("I_B", OIB, U_A, "Courant de base visé k·I_Bmin");
    r.out("R_{B\\max}", ORBMAX, U_OHM, "(V_in − V_BE)/(k I_Bmin)", false, T_RB);
    r.out("R_B", ORB, U_OHM, "Valeur E24 inférieure ou égale", true, T_RB);
    r.out("I_B\\ (R_B)", OIBR, U_A, "Courant de base avec la valeur E24");
    r.out("P_T", OPT, U_W, "Puissance dans le transistor saturé");
    r.out("P_L", OPL, U_W, "Puissance dans la charge");
    r.msg(0);
}
static void calc(CalcState &s)
{
    s.v[OIC] = (s.v[VCC] - s.v[VCES]) / s.v[RL];
    s.v[OIBMIN] = s.v[OIC] / s.v[BETA];
    s.v[OIB] = s.v[KS] * s.v[OIBMIN];
    double drive = s.v[VIN] - s.v[VBE];
    if (drive > 0) {
        s.v[ORBMAX] = drive / s.v[OIB];
        s.v[ORB] = eFloor(E24, s.v[ORBMAX]);
        s.v[OIBR] = drive / s.v[ORB];
        setMsg(s, 0, "Transistor saturé : V_CE ≈ V_CEsat", ML_OK);
    } else {
        s.v[ORBMAX] = s.v[ORB] = s.v[OIBR] = NaN();
        setMsg(s, 0, "V_in trop faible : il faut V_in > V_BE", ML_BAD);
    }
    s.v[OPT] = s.v[VCES] * s.v[OIC];
    s.v[OPL] = s.v[OIC] * s.v[OIC] * s.v[RL];
}
static void sch(Sch &g, const CalcState &s)
{
    const int yt = 18, Y = 152, bx = 84, by = 96, xq = bx + 17;
    g.rail(xq, yt, nullptr);
    g.tex(xq + 12, yt + 5, "V_{CC}", tex::LEFT);
    g.res(xq, yt, xq, by - 20, T_RL);
    g.tex(xq + 9, 50, "R_L", tex::LEFT, T_RL);
    g.bjt(bx, by, true, T_Q);
    g.wire(xq, by + 20, xq, Y);
    g.gnd(xq, Y);
    g.res(22, by, bx, by, T_RB);
    g.tex(53, by - 9, "R_B", tex::CENTER, T_RB);
    // commande logique
    g.vsrc(22, by, Y, 0, T_VIN);
    g.gnd(22, Y);
    g.tex(36, 132, "V_{in}", tex::LEFT, T_VIN);
    nodeVal(g, 108, by - 26, "I_C", s.v[OIC], U_A);
    nodeVal(g, 38, by + 24, "I_B", s.v[OIBR], U_A);
}
}  // namespace sw


extern const CalcDef g_calcBjt[] = {
    {"Polarisation par pont diviseur", "#NPN ou PNP, émetteur commun", "bipolaire bjt npn pnp polarisation pont diviseur emetteur commun gain", 4, G_NONE, 0,
     pont::def, pont::rows, pont::calc, pont::sch, nullptr, nullptr, nullptr, nullptr, 0},
    {"Polarisation par résistance de base", "I_B=(V_{CC}-V_{BE})/R_B", "bipolaire bjt resistance base polarisation", 4, G_NONE, 0,
     rbase::def, rbase::rows, rbase::calc, rbase::sch, nullptr, nullptr, nullptr, nullptr, 0},
    {"Transistor en commutation", "#Calcul de R_B pour saturer le transistor", "bipolaire commutation saturation interrupteur relais rb", 4, G_NONE, 0,
     sw::def, sw::rows, sw::calc, sw::sch, nullptr, nullptr, nullptr, nullptr, 0},
};
extern const int g_calcBjtN = sizeof(g_calcBjt) / sizeof(g_calcBjt[0]);
