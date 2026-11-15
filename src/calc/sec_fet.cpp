// Section : transistors a effet de champ (MOSFET a enrichissement, JFET).
//
// Le canal P est traite en "miroir" du canal N (tensions comptees depuis
// +V_DD) ; le point de repos est obtenu par dichotomie sur I_D, ce qui couvre
// les trois regimes (bloque, ohmique, saturation).
#include "CalcUtil.hpp"

using namespace cu;

static void nodeVal(Sch &g, int x, int y, double v)
{
    char buf[24];
    fmtValue(buf, sizeof buf, v, U_V, 3);
    int w = font::width(SANS_12, buf);
    if (x + w > 177) x = 177 - w;
    font::drawText(*g.cv, SANS_12, g.ox + x, g.oy + y, buf, g.volt);
}

// Modele de courant : vgs = tension de commande (miroir), vds >= 0.
typedef double (*IdModel)(double p1, double p2, double vgs, double vds, int &region);

// MOSFET : I_D = K (V_GS - V_th)^2 en saturation. p1 = V_th, p2 = K.
static double mosModel(double vth, double k, double vgs, double vds, int &region)
{
    double u = vgs - vth;
    if (u <= 0) { region = 0; return 0; }
    if (vds < 0) vds = 0;
    if (vds >= u) { region = 2; return k * u * u; }
    region = 1;
    return k * (2 * u * vds - vds * vds);
}

// JFET : I_D = I_DSS (1 + V_GS/Vp)^2, Vp = |V_P|. p1 = I_DSS, p2 = Vp.
static double jfetModel(double idss, double vp, double vgs, double vds, int &region)
{
    double a = 1 + vgs / vp;      // 1 - V_GS/V_P
    if (a <= 0) { region = 0; return 0; }
    if (vds < 0) vds = 0;
    double vsat = vgs + vp;        // V_DS de pincement
    if (vds >= vsat) { region = 2; return idss * a * a; }
    region = 1;
    double x = vds / vp;
    return idss * (2 * a * x - x * x);
}

// Point de repos : vg = commande de grille, I_D par dichotomie.
static double solveId(IdModel m, double p1, double p2, double vdd, double vg, double rd, double rs, int &region)
{
    double lo = 0, hi = vdd / (rd + rs);
    if (!(hi > 0)) { region = 0; return 0; }
    for (int i = 0; i < 90; i++) {
        double id = 0.5 * (lo + hi);
        int r;
        double im = m(p1, p2, vg - id * rs, vdd - id * (rd + rs), r);
        if (id > im) hi = id;
        else lo = id;
    }
    double id = 0.5 * (lo + hi);
    m(p1, p2, vg - id * rs, vdd - id * (rd + rs), region);
    return id;
}

static void regionMsg(CalcState &s, int k, int region)
{
    if (region == 0) setMsg(s, k, "Bloqué : V_GS en dessous du seuil", ML_WARN);
    else if (region == 1) setMsg(s, k, "Régime ohmique (linéaire) : V_DS < V_GS − V_th", ML_WARN);
    else setMsg(s, k, "Saturation : amplification possible", ML_OK);
}

// Schema commun : pont (ou R_G) a gauche, transistor, R_D, R_S.
static void drawFet(Sch &g, bool nch, bool divider, bool jfet, bool cs, int tR1, int tR2, int tRD, int tRS, int tQ,
                    int tCs)
{
    const int yt = 18, Y = 152, xb = 40, gx = 82, gy = 86, xq = gx + (jfet ? 19 : 18);
    int xr = cs ? 140 : xq;
    g.wire(xb, yt, xr, yt);
    g.tex(xr, yt - 4, "V_{DD}", tex::RIGHT);
    g.wire(xb, Y, xr, Y);
    g.dot(72, Y);
    g.gnd(72, Y);
    const char *lTop = nch ? "R_D" : "R_S", *lBot = nch ? "R_S" : "R_D";
    int tTop = nch ? tRD : tRS, tBot = nch ? tRS : tRD;
    if (divider) {
        g.res(xb, yt, xb, gy, tR1);
        g.res(xb, gy, xb, Y, tR2);
        g.tex(xb - 9, 56, "R_1", tex::RIGHT, tR1);
        g.tex(xb - 9, 124, "R_2", tex::RIGHT, tR2);
        g.dot(xb, gy);
        g.dot(xb, yt);
        g.dot(xb, Y);
    } else if (nch) {
        g.res(xb, gy, xb, Y, tR1);
        g.tex(xb - 9, 124, "R_G", tex::RIGHT, tR1);
        g.dot(xb, Y);
    } else {
        g.res(xb, yt, xb, gy, tR1);
        g.tex(xb - 9, 56, "R_G", tex::RIGHT, tR1);
        g.dot(xb, yt);
    }
    g.wire(xb, gy, gx, gy);
    if (jfet) g.jfet(gx, gy, nch, tQ);
    else g.mosfet(gx, gy, nch, tQ);
    g.res(xq, yt, xq, gy - 20, tTop);
    g.res(xq, gy + 20, xq, Y, tBot);
    g.tex(xq + 9, 50, lTop, tex::LEFT, tTop);
    g.tex(xq + 9, 130, lBot, tex::LEFT, tBot);
    g.dot(xq, yt);
    g.dot(xq, Y);
    if (cs) {
        int y = nch ? gy + 30 : gy - 30;
        g.wire(xq, y, 140, y, tCs);
        g.cap(140, y, 140, nch ? Y : yt, tCs);
        g.dot(xq, y);
        g.dot(140, nch ? Y : yt);
        g.tex(146, nch ? 132 : 46, "C_S", tex::LEFT, tCs);
    }
}

// ==========================================================================
// MOSFET : polarisation par pont diviseur
// ==========================================================================
namespace mos {
enum { VDD, R1, R2, RD, RS, VTH, K, OVG, OVGS, OID, OVS, OVD, OVDS, OP, OGM, OAV, OZE };
enum { CTYPE, CCS };
enum { T_R1 = 1, T_R2, T_RD, T_RS, T_Q, T_CS };
static void def(CalcState &s)
{
    s.v[VDD] = 15; s.v[R1] = 2.2e6; s.v[R2] = 1e6; s.v[RD] = 4.7e3; s.v[RS] = 1e3;
    s.v[VTH] = 2; s.v[K] = 1e-3; s.c[CCS] = 1;
}
static void rows(const CalcState &s, RowList &r)
{
    bool n = s.c[CTYPE] == 0;
    r.choice("#Canal", CTYPE, "N|P", T_Q, "MOSFET à enrichissement canal N ou P");
    r.in("V_{DD}", VDD, U_V, ST_LIN1, 0, "Tension d'alimentation");
    r.in("R_1", R1, U_OHM, ST_E24, T_R1, "Pont : entre +V_DD et la grille");
    r.in("R_2", R2, U_OHM, ST_E24, T_R2, "Pont : entre la grille et la masse");
    r.in("R_D", RD, U_OHM, ST_E24, T_RD, "Résistance de drain");
    r.in("R_S", RS, U_OHM, ST_E24, T_RS, "Résistance de source", RF_NONNEG);
    r.in(n ? "V_{th}" : "|V_{th}|", VTH, U_V, ST_LIN01, T_Q, "Tension de seuil V_GS(th) de la notice");
    r.in("K", K, U_AV2, ST_125, T_Q, "I_D = K (V_GS − V_th)² en saturation");
    r.head("Point de repos");
    r.out("V_G", OVG, U_V, "Potentiel de grille (pont, I_G = 0)");
    r.out(n ? "V_{GS}" : "V_{SG}", OVGS, U_V, "Tension de commande", true);
    r.out("I_D", OID, U_A, "Courant de drain", true, T_RD);
    r.out("V_S", OVS, U_V, "Potentiel de source");
    r.out("V_D", OVD, U_V, "Potentiel de drain");
    r.out(n ? "V_{DS}" : "V_{SD}", OVDS, U_V, "Tension drain-source", true);
    r.out("P", OP, U_W, "Puissance dissipée V_DS·I_D");
    r.msg(0);
    r.head("Petits signaux");
    r.choice("C_S", CCS, "absent|R_S découplée", T_CS, "Condensateur de découplage en parallèle sur R_S");
    r.out("g_m", OGM, U_SIE, "Transconductance 2K (V_GS − V_th) = 2√(K I_D)");
    r.out("A_v", OAV, U_NONE, "Gain à vide : −g_m R_D / (1 + g_m R_S)", true);
    r.out("Z_e", OZE, U_OHM, "Impédance d'entrée R₁ ∥ R₂");
}
static void calc(CalcState &s)
{
    bool n = s.c[CTYPE] == 0;
    double vdd = s.v[VDD], vgG = vdd * s.v[R2] / (s.v[R1] + s.v[R2]);
    double vg = n ? vgG : vdd - vgG;
    int region;
    double id = solveId(mosModel, s.v[VTH], s.v[K], vdd, vg, s.v[RD], s.v[RS], region);
    double vsd = id * s.v[RS], vdd_ = id * s.v[RD];
    s.v[OVG] = vgG;
    s.v[OID] = id;
    s.v[OVGS] = vg - vsd;
    s.v[OVDS] = vdd - id * (s.v[RD] + s.v[RS]);
    s.v[OVS] = n ? vsd : vdd - vsd;
    s.v[OVD] = n ? vdd - vdd_ : vdd_;
    s.v[OP] = s.v[OVDS] * id;
    s.v[OZE] = par2(s.v[R1], s.v[R2]);
    regionMsg(s, 0, region);
    if (region == 2) {
        double gm = 2 * s.v[K] * (s.v[OVGS] - s.v[VTH]);
        double rs = s.c[CCS] ? 0 : s.v[RS];
        s.v[OGM] = gm;
        s.v[OAV] = -gm * s.v[RD] / (1 + gm * rs);
    } else {
        s.v[OGM] = s.v[OAV] = NaN();
    }
}
static void sch(Sch &g, const CalcState &s)
{
    bool n = s.c[CTYPE] == 0;
    drawFet(g, n, true, false, s.c[CCS] == 1, T_R1, T_R2, T_RD, T_RS, T_Q, T_CS);
    nodeVal(g, 44, 100, s.v[OVG]);
    nodeVal(g, 108, n ? 70 : 104, s.v[OVD]);
    nodeVal(g, 108, n ? 104 : 70, s.v[OVS]);
}
}  // namespace mos

// ==========================================================================
// JFET : autopolarisation ou pont diviseur
// ==========================================================================
namespace jf {
enum { VDD, RG, R1, R2, RD, RS, IDSS, VP, OVG, OVGS, OID, OVS, OVD, OVDS, OGM, OAV, OGM0 };
enum { CTYPE, CBIAS, CCS };
enum { T_RG = 1, T_R2, T_RD, T_RS, T_Q, T_CS };
static void def(CalcState &s)
{
    s.v[VDD] = 15; s.v[RG] = 1e6; s.v[R1] = 1e6; s.v[R2] = 100e3; s.v[RD] = 2.2e3; s.v[RS] = 470;
    s.v[IDSS] = 8e-3; s.v[VP] = 3; s.c[CCS] = 1;
}
static void rows(const CalcState &s, RowList &r)
{
    bool div = s.c[CBIAS] == 1;
    r.choice("#Canal", CTYPE, "N|P", T_Q, "JFET canal N ou P");
    r.choice("#Polarisation", CBIAS, "auto (R_G)|pont R₁-R₂", T_RG, "Grille à la masse par R_G, ou pont R₁-R₂");
    r.in("V_{DD}", VDD, U_V, ST_LIN1, 0, "Tension d'alimentation");
    if (div) {
        r.in("R_1", R1, U_OHM, ST_E24, T_RG, "Pont : entre +V_DD et la grille");
        r.in("R_2", R2, U_OHM, ST_E24, T_R2, "Pont : entre la grille et la masse");
    } else {
        r.in("R_G", RG, U_OHM, ST_E24, T_RG, "Résistance de grille (pas de courant : V_G = 0)");
    }
    r.in("R_D", RD, U_OHM, ST_E24, T_RD, "Résistance de drain");
    r.in("R_S", RS, U_OHM, ST_E24, T_RS, "Résistance de source (fixe V_GS)", RF_NONNEG);
    r.in("I_{DSS}", IDSS, U_A, ST_125, T_Q, "Courant de drain pour V_GS = 0");
    r.in("|V_P|", VP, U_V, ST_LIN01, T_Q, "Tension de pincement |V_GS(off)|");
    r.head("Point de repos");
    r.out("V_{GS}", OVGS, U_V, "Tension grille-source", true);
    r.out("I_D", OID, U_A, "I_D = I_DSS (1 − V_GS/V_P)²", true, T_RD);
    r.out("V_G", OVG, U_V, "Potentiel de grille");
    r.out("V_S", OVS, U_V, "Potentiel de source");
    r.out("V_D", OVD, U_V, "Potentiel de drain");
    r.out("V_{DS}", OVDS, U_V, "Tension drain-source", true);
    r.msg(0);
    r.msg(1);
    r.head("Petits signaux");
    r.choice("C_S", CCS, "absent|R_S découplée", T_CS, "Condensateur de découplage en parallèle sur R_S");
    r.out("g_{m0}", OGM0, U_SIE, "2 I_DSS / |V_P|");
    r.out("g_m", OGM, U_SIE, "g_m0 (1 − V_GS/V_P) = g_m0 √(I_D/I_DSS)");
    r.out("A_v", OAV, U_NONE, "Gain à vide : −g_m R_D / (1 + g_m R_S)", true);
}
static void calc(CalcState &s)
{
    bool n = s.c[CTYPE] == 0, div = s.c[CBIAS] == 1;
    double vdd = s.v[VDD];
    double vgG = div ? vdd * s.v[R2] / (s.v[R1] + s.v[R2]) : (n ? 0 : vdd);
    double vg = n ? vgG : vdd - vgG;    // commande en miroir
    int region;
    double id = solveId(jfetModel, s.v[IDSS], s.v[VP], vdd, vg, s.v[RD], s.v[RS], region);
    double vgsM = vg - id * s.v[RS];    // V_GS (canal N) ou V_SG... en miroir
    double vdsM = vdd - id * (s.v[RD] + s.v[RS]);
    s.v[OID] = id;
    s.v[OVG] = vgG;
    s.v[OVGS] = n ? vgsM : -vgsM;
    s.v[OVDS] = n ? vdsM : -vdsM;
    s.v[OVS] = n ? id * s.v[RS] : vdd - id * s.v[RS];
    s.v[OVD] = n ? vdd - id * s.v[RD] : id * s.v[RD];
    s.v[OGM0] = 2 * s.v[IDSS] / s.v[VP];
    if (region == 0) setMsg(s, 0, "Bloqué : |V_GS| ≥ |V_P|", ML_WARN);
    else if (region == 1) setMsg(s, 0, "Régime ohmique : V_DS trop faible", ML_WARN);
    else setMsg(s, 0, "Saturation (zone de courant constant)", ML_OK);
    if (vgsM > 0.3) setMsg(s, 1, "Jonction grille passante : V_GS doit rester ≤ 0", ML_BAD);
    else setMsg(s, 1, nullptr, ML_INFO);
    if (region == 2) {
        double gm = s.v[OGM0] * (1 + vgsM / s.v[VP]);
        double rs = s.c[CCS] ? 0 : s.v[RS];
        s.v[OGM] = gm;
        s.v[OAV] = -gm * s.v[RD] / (1 + gm * rs);
    } else {
        s.v[OGM] = s.v[OAV] = NaN();
    }
}
static void sch(Sch &g, const CalcState &s)
{
    bool n = s.c[CTYPE] == 0;
    drawFet(g, n, s.c[CBIAS] == 1, true, s.c[CCS] == 1, T_RG, T_R2, T_RD, T_RS, T_Q, T_CS);
    nodeVal(g, 108, n ? 70 : 104, s.v[OVD]);
    nodeVal(g, 108, n ? 104 : 70, s.v[OVS]);
}
}  // namespace jf

// ==========================================================================
// MOSFET en commutation : pertes et temperature
// ==========================================================================
namespace msw {
enum { VDD, ID, RON, ALPHA, QG, IG, FS, RTH, TA, OVDS, OPC, OTSW, OPSW, OPT, OTJ };
enum { T_Q = 1, T_L, T_G };
static void def(CalcState &s)
{
    s.v[VDD] = 24; s.v[ID] = 5; s.v[RON] = 20e-3; s.v[ALPHA] = 50; s.v[QG] = 30e-9; s.v[IG] = 0.5;
    s.v[FS] = 50e3; s.v[RTH] = 40; s.v[TA] = 40;
}
static void rows(const CalcState &, RowList &r)
{
    r.in("V_{DD}", VDD, U_V, ST_LIN1, T_L, "Tension commutée");
    r.in("I_D", ID, U_A, ST_125, T_L, "Courant dans la charge à l'état passant");
    r.in("R_{DS(on)}", RON, U_OHM, ST_E12, T_Q, "Résistance à l'état passant (notice, à chaud si possible)");
    r.in("\\alpha", ALPHA, U_PCT, ST_LIN10, 0, "Rapport cyclique (temps passant / période)", RF_NONNEG);
    r.in("Q_g", QG, U_COUL, ST_125, T_G, "Charge totale de grille (notice)");
    r.in("I_G", IG, U_A, ST_125, T_G, "Courant fourni par le driver de grille");
    r.in("f_s", FS, U_HZ, ST_125, 0, "Fréquence de découpage", RF_NONNEG);
    r.in("R_{th\\,ja}", RTH, U_CW, ST_LIN1, T_Q, "Résistance thermique jonction-ambiant (avec dissipateur)");
    r.in("T_a", TA, U_DEGC, ST_LIN5, 0, "Température ambiante", 0);
    r.head("Résultats");
    r.out("V_{DS(on)}", OVDS, U_V, "Chute à l'état passant R_DS(on)·I_D");
    r.out("P_{\\text{cond}}", OPC, U_W, "Pertes par conduction α R_DS(on) I_D²");
    r.out("t_{\\text{com}}", OTSW, U_S, "Durée d'une commutation ≈ Q_g / I_G");
    r.out("P_{\\text{com}}", OPSW, U_W, "Pertes par commutation ≈ V_DD I_D t_com f_s");
    r.out("P", OPT, U_W, "Pertes totales", true);
    r.out("T_j", OTJ, U_DEGC, "Température de jonction T_a + P·R_th", true);
    r.msg(0);
}
static void calc(CalcState &s)
{
    s.v[OVDS] = s.v[RON] * s.v[ID];
    s.v[OPC] = s.v[ALPHA] / 100 * s.v[RON] * s.v[ID] * s.v[ID];
    s.v[OTSW] = s.v[QG] / s.v[IG];
    s.v[OPSW] = s.v[VDD] * s.v[ID] * s.v[OTSW] * s.v[FS];
    s.v[OPT] = s.v[OPC] + s.v[OPSW];
    s.v[OTJ] = s.v[TA] + s.v[OPT] * s.v[RTH];
    if (s.v[OTJ] > 150) setMsg(s, 0, "T_j > 150 °C : dissipateur insuffisant", ML_BAD);
    else if (s.v[OTJ] > 125) setMsg(s, 0, "T_j > 125 °C : marge faible", ML_WARN);
    else setMsg(s, 0, "Température de jonction acceptable", ML_OK);
}
static void sch(Sch &g, const CalcState &s)
{
    const int yt = 22, Y = 152, gx = 82, gy = 100, xq = gx + 18;
    g.rail(xq, yt, nullptr);
    g.tex(xq + 12, yt + 5, "V_{DD}", tex::LEFT, T_L);
    g.res(xq, yt, xq, gy - 20, T_L);
    g.tex(xq + 9, 56, "#charge", tex::LEFT, T_L);
    g.mosfet(gx, gy, true, T_Q);
    g.wire(xq, gy + 20, xq, Y);
    g.gnd(xq, Y);
    g.res(34, gy, gx, gy, T_G);
    g.tex(58, gy - 9, "R_G", tex::CENTER, T_G);
    g.vsrc(34, gy, Y, 0, T_G);
    g.gnd(34, Y);
    g.tex(46, 130, "#driver", tex::LEFT, T_G);
    char buf[32];
    int k = scopy(buf, sizeof buf, "T_j ");
    fmtValue(buf + k, (int)sizeof(buf) - k, s.v[OTJ], U_DEGC);
    font::drawText(*g.cv, BOLD_12, g.ox + 106, g.oy + 134, buf, s.v[OTJ] > 150 ? col::red : g.volt);
}
}  // namespace msw

extern const CalcDef g_calcFet[] = {
    {"MOSFET : polarisation", "#Canal N ou P, pont diviseur", "mosfet enrichissement polarisation seuil transconductance", 5, G_NONE, 0,
     mos::def, mos::rows, mos::calc, mos::sch, nullptr, nullptr, nullptr, nullptr, 0},
    {"JFET : polarisation", "#Autopolarisation ou pont diviseur", "jfet idss pincement autopolarisation", 5, G_NONE, 0,
     jf::def, jf::rows, jf::calc, jf::sch, nullptr, nullptr, nullptr, nullptr, 0},
    {"MOSFET en commutation", "#Pertes et température de jonction", "mosfet commutation pertes rdson dissipateur hacheur", 5, G_NONE, 0,
     msw::def, msw::rows, msw::calc, msw::sch, nullptr, nullptr, nullptr, nullptr, 0},
};
extern const int g_calcFetN = sizeof(g_calcFet) / sizeof(g_calcFet[0]);
