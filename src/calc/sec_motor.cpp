// Section : optocoupleurs et moteurs (optocoupleur, moteur a courant
// continu, moteur pas a pas, moteur asynchrone triphase).
#include "CalcUtil.hpp"

using namespace cu;

static void valText(Sch &g, int x, int y, const char *name, double v, uint8_t unit, int align = 0, bool bold = false)
{
    char buf[48];
    int k = 0;
    buf[0] = 0;
    if (name) { k = scopy(buf, sizeof buf, name); k += scopy(buf + k, (int)sizeof(buf) - k, " = "); }
    fmtValue(buf + k, (int)sizeof(buf) - k, v, unit, 4);
    int f = bold ? BOLD_12 : SANS_12;
    int w = font::richWidth(f, buf);
    if (align == 1) x -= w / 2;
    else if (align == 2) x -= w;
    if (x + w > 177) x = 177 - w;
    if (x < 2) x = 2;
    font::drawRich(*g.cv, f, g.ox + x, g.oy + y, buf, g.volt);
}

// Symbole de moteur (cercle et M) centre en (x, y).
static void motor(Sch &g, int x, int y, int tag, const char *t = "#M")
{
    g.cv->disc(g.ox + x, g.oy + y, 11, g.on(tag) ? g.light : g.paper);
    g.cv->circle(g.ox + x, g.oy + y, 11, g.col(tag));
    g.cv->circle(g.ox + x, g.oy + y, 10, g.col(tag));
    tex::draw(*g.cv, t, g.ox + x, g.oy + y + 5, g.col(tag), tex::CENTER, 1);
}

// ==========================================================================
// Optocoupleur
// ==========================================================================
namespace opto {
enum { VIN, VF, IF, CTR, VCC, VCES, KM, ORIN, ORINE, OIFR, OPLED, OICMAX, ORLMIN, ORL, OIC, OVL, OVH };
enum { COUT };
enum { T_VIN = 1, T_RIN, T_LED, T_Q, T_RL, T_VCC };
static void def(CalcState &s)
{
    s.v[VIN] = 5; s.v[VF] = 1.2; s.v[IF] = 10e-3; s.v[CTR] = 50; s.v[VCC] = 3.3; s.v[VCES] = 0.3; s.v[KM] = 2;
}
static void rows(const CalcState &s, RowList &r)
{
    bool em = s.c[COUT] == 1;
    r.head("Entrée (LED)");
    r.in("V_{in}", VIN, U_V, ST_LIN01, T_VIN, "Tension de commande (sortie logique, alimentation…)");
    r.in("V_F", VF, U_V, ST_LIN01, T_LED, "Tension directe de la LED (1,1 à 1,4 V)");
    r.in("I_F", IF, U_A, ST_125, T_LED, "Courant voulu dans la LED (5 à 20 mA)");
    r.head("Sortie (phototransistor)");
    r.in("CTR", CTR, U_PCT, ST_LIN10, T_Q, "Taux de transfert minimal I_C/I_F de la notice");
    r.in("V_{CC}", VCC, U_V, ST_LIN01, T_VCC, "Alimentation du côté sortie");
    r.in("V_{CE,sat}", VCES, U_V, ST_LIN01, T_Q, "Tension de saturation du phototransistor", RF_NONNEG);
    r.choice("#Montage", COUT, "collecteur (tirage au +)|émetteur (charge à la masse)", T_RL);
    r.in("k", KM, U_NONE, ST_NONE, T_RL, "Marge sur le CTR (vieillissement, température) : 2 conseillé");
    r.head("Résultats");
    r.out("R_{in}", ORIN, U_OHM, "(V_in − V_F)/I_F", false, T_RIN);
    r.out("R_{in}", ORINE, U_OHM, "Valeur E24 la plus proche", true, T_RIN);
    r.out("I_F", OIFR, U_A, "Courant avec la valeur E24", false, T_LED);
    r.out("P_{LED}", OPLED, U_W, "Puissance dans la LED V_F I_F");
    r.out("I_{C\\max}", OICMAX, U_A, "Courant de sortie possible CTR · I_F", false, T_Q);
    r.out("R_{L\\min}", ORLMIN, U_OHM, "Charge minimale pour saturer : (V_CC − V_CEsat)/(CTR I_F)", false, T_RL);
    r.out("R_L", ORL, U_OHM, "Valeur E24 ≥ k R_Lmin", true, T_RL);
    r.out("I_C", OIC, U_A, "Courant de sortie à l'état passant");
    r.out(em ? "V_{s,haut}" : "V_{s,bas}", OVL, U_V, em ? "Sortie quand la LED est allumée" : "Sortie quand la LED est allumée (≈ V_CEsat)");
    r.out(em ? "V_{s,bas}" : "V_{s,haut}", OVH, U_V, "Sortie quand la LED est éteinte");
    r.msg(0);
}
static void calc(CalcState &s)
{
    bool em = s.c[COUT] == 1;
    double d = s.v[VIN] - s.v[VF];
    s.v[ORIN] = d / s.v[IF];
    s.v[ORINE] = d > 0 ? eNearest(E24, s.v[ORIN]) : NaN();
    s.v[OIFR] = d / s.v[ORINE];
    s.v[OPLED] = s.v[VF] * s.v[OIFR];
    s.v[OICMAX] = s.v[CTR] / 100 * s.v[OIFR];
    s.v[ORLMIN] = (s.v[VCC] - s.v[VCES]) / s.v[OICMAX];
    s.v[ORL] = eCeil(E24, s.v[KM] * s.v[ORLMIN]);
    s.v[OIC] = (s.v[VCC] - s.v[VCES]) / s.v[ORL];
    s.v[OVL] = em ? s.v[VCC] - s.v[VCES] : s.v[VCES];
    s.v[OVH] = em ? 0 : s.v[VCC];
    if (!(d > 0)) setMsg(s, 0, "V_in trop faible : la LED ne s'allume pas", ML_BAD);
    else if (s.v[OIFR] > 50e-3) setMsg(s, 0, "I_F trop grand pour un optocoupleur courant (≤ 50 mA)", ML_WARN);
    else setMsg(s, 0, "Phototransistor saturé : sortie logique franche", ML_OK);
}
static void sch(Sch &g, const CalcState &s)
{
    bool em = s.c[COUT] == 1;
    // boitier
    g.rect(52, 40, 64, 76, g.paper);
    g.frame(52, 40, 64, 76, g.ink);
    for (int y = 42; y < 114; y += 4) g.rect(84, y, 1, 2, g.label);
    // LED
    g.vsrc(12, 52, 122, 0, T_VIN);
    g.tex(24, 96, "V_{in}", tex::LEFT, T_VIN);
    g.wire(12, 52, 20, 52);
    g.res(20, 52, 52, 52, T_RIN);
    g.tex(36, 44, "R_{in}", tex::CENTER, T_RIN);
    g.wire(52, 52, 66, 52, T_LED);
    g.diode(66, 52, 66, 104, 1, T_LED);
    g.wire(66, 104, 52, 104, T_LED);
    g.wire(52, 104, 52, 122);
    g.wire(12, 122, 52, 122);
    g.gnd(30, 122);
    // fleches lumineuses
    uint16_t lc = g.col(T_LED);
    for (int k = 0; k < 2; k++) {
        g.line(72, 70 + 8 * k, 86, 70 + 8 * k, lc);
        g.cv->arrowHead(g.ox + 88, g.oy + 70 + 8 * k, 1, 0, 4, lc);
    }
    // phototransistor (base absente)
    uint16_t qc = g.col(T_Q);
    int bx = 92, by = 78;
    g.cv->fillRect(g.ox + bx + 8, g.oy + by - 9, 2, 19, qc);
    g.line(bx + 10, by - 4, bx + 17, by - 11, qc);
    g.line(bx + 17, by - 11, bx + 17, by - 26, qc);
    g.line(bx + 10, by + 4, bx + 17, by + 11, qc);
    g.line(bx + 17, by + 11, bx + 17, by + 26, qc);
    g.cv->arrowHead(g.ox + bx + 17, g.oy + by + 11, 1, 1, 5, qc);
    int xq = bx + 17;
    g.wire(xq, by - 26, 130, by - 26);
    g.wire(xq, by + 26, 130, by + 26);
    // sortie
    if (!em) {
        g.res(130, 18, 130, by - 26, T_RL);
        g.rail(130, 18, nullptr, T_VCC);
        g.tex(140, 22, "V_{CC}", tex::LEFT, T_VCC);
        g.tex(137, 42, "R_L", tex::LEFT, T_RL);
        g.wire(130, by + 26, 130, 136);
        g.gnd(130, 136);
        g.wire(130, by - 26, 168, by - 26);
        g.term(168, by - 26);
        g.tex(176, by - 12, "v_s", tex::RIGHT);
    } else {
        g.wire(130, by - 26, 130, 30);
        g.rail(130, 30, nullptr, T_VCC);
        g.tex(140, 34, "V_{CC}", tex::LEFT, T_VCC);
        g.res(130, by + 26, 130, 140, T_RL);
        g.gnd(130, 140);
        g.tex(138, 126, "R_L", tex::LEFT, T_RL);
        g.wire(130, by + 26, 168, by + 26);
        g.term(168, by + 26);
        g.tex(176, by + 20, "v_s", tex::RIGHT);
    }
    g.dot(130, em ? by + 26 : by - 26);
    valText(g, 4, 158, "R_in", s.v[ORINE], U_OHM, 0, true);
    valText(g, 177, 158, "R_L", s.v[ORL], U_OHM, 2, true);
}
}  // namespace opto

// ==========================================================================
// Moteur a courant continu
// ==========================================================================
namespace mcc {
enum { U, R, K, CR, I0, OI, OE, OW, ON, OPA, OPU, OPJ, OETA, ON0, OID, OCD, OPMAX, OCEM };
enum { T_U = 1, T_R, T_E, T_LOAD };
static void def(CalcState &s) { s.v[U] = 12; s.v[R] = 2; s.v[K] = 0.02; s.v[CR] = 0.02; s.v[I0] = 0.1; }
static void rows(const CalcState &, RowList &r)
{
    r.in("U", U, U_V, ST_LIN1, T_U, "Tension d'alimentation de l'induit");
    r.in("R", R, U_OHM, ST_E24, T_R, "Résistance de l'induit");
    r.in("K", K, U_VSR, ST_NONE, T_E, "Constante de FEM E = K Ω (= constante de couple)");
    r.in("I_0", I0, U_A, ST_125, T_E, "Courant à vide (pertes mécaniques)", RF_NONNEG);
    r.in("C_r", CR, U_NM, ST_125, T_LOAD, "Couple résistant de la charge", RF_NONNEG);
    r.head("Point de fonctionnement");
    r.out("I", OI, U_A, "I = I₀ + C_r/K", true, T_R);
    r.out("E", OE, U_V, "FEM U − R I", false, T_E);
    r.out("N", ON, U_RPM, "Vitesse de rotation", true, T_E);
    r.out("\\Omega", OW, U_RADS, "Vitesse angulaire E/K");
    r.out("C_{em}", OCEM, U_NM, "Couple électromagnétique K I");
    r.out("P_a", OPA, U_W, "Puissance absorbée U I");
    r.out("P_u", OPU, U_W, "Puissance utile C_r Ω");
    r.out("P_J", OPJ, U_W, "Pertes Joule R I²", false, T_R);
    r.out("\\eta", OETA, U_PCT, "Rendement P_u/P_a", true);
    r.head("Caractéristiques");
    r.out("N_0", ON0, U_RPM, "Vitesse à vide (U − R I₀)/K");
    r.out("I_d", OID, U_A, "Courant de démarrage U/R (rotor bloqué)");
    r.out("C_d", OCD, U_NM, "Couple de démarrage K (I_d − I₀)");
    r.out("P_{u,\\max}", OPMAX, U_W, "Puissance utile maximale (à mi-vitesse)");
    r.msg(0);
}
static void calc(CalcState &s)
{
    double k = s.v[K], r = s.v[R], u = s.v[U];
    double i = s.v[I0] + s.v[CR] / k;
    double e = u - r * i;
    double w = e / k;
    s.v[OI] = i;
    s.v[OE] = e;
    s.v[OW] = w;
    s.v[ON] = w * 60 / fm::TWO_PI;
    s.v[OCEM] = k * i;
    s.v[OPA] = u * i;
    s.v[OPU] = s.v[CR] * w;
    s.v[OPJ] = r * i * i;
    s.v[OETA] = s.v[OPU] / s.v[OPA] * 100;
    s.v[ON0] = (u - r * s.v[I0]) / k * 60 / fm::TWO_PI;
    s.v[OID] = u / r;
    s.v[OCD] = k * (s.v[OID] - s.v[I0]);
    // P_u(Omega) = (K I - K I0) Omega avec I = (U - K Omega)/R : maximum a Omega0/2
    double w0 = (u - r * s.v[I0]) / k;
    s.v[OPMAX] = s.v[OCD] * w0 / 4;
    if (e <= 0) setMsg(s, 0, "Couple trop grand : le moteur cale", ML_BAD);
    else setMsg(s, 0, "F4 : caractéristiques C(N), P(N), η(N)", ML_INFO);
}
static void sch(Sch &g, const CalcState &s)
{
    const int yt = 40, yb = 120;
    g.vsrc(18, yt, yb, 0, T_U);
    g.tex(30, 86, "U", tex::LEFT, T_U);
    g.wire(18, yt, 60, yt);
    g.res(60, yt, 110, yt, T_R);
    g.tex(85, yt - 10, "R", tex::CENTER, T_R);
    g.wire(110, yt, 140, yt);
    g.wire(140, yt, 140, 66, T_E);
    motor(g, 140, 80, T_E);
    g.wire(140, 94, 140, yb, T_E);
    g.wire(18, yb, 140, yb);
    g.varrow(122, 94, 66, "E", -1, T_E);
    g.iarrow(40, yt, 1, 0, "I", 36, yt - 5);
    // arbre et charge
    g.rect(151, 79, 18, 3, g.col(T_LOAD));
    g.texSmall(166, 100, "C_r", tex::CENTER, T_LOAD);
    valText(g, 4, 150, "N", s.v[ON], U_RPM, 0, true);
    valText(g, 4, 166, "\xCE\xB7", s.v[OETA], U_PCT, 0, true);
    valText(g, 177, 166, "I", s.v[OI], U_A, 2);
}
static void graph(Canvas &cv, const CalcState &s, const GraphArea &a, uint16_t accent)
{
    double k = s.v[K], r = s.v[R], u = s.v[U], i0 = s.v[I0];
    double w0 = (u - r * i0) / k;
    if (!(w0 > 0)) return;
    double n0 = w0 * 60 / fm::TWO_PI;
    double cd = s.v[OCD], pmax = s.v[OPMAX];
    const int X0 = a.x + 20, X1 = a.x + a.w - 56, Y0 = a.y + 24, Y1 = a.y + a.h - 30;
    uint16_t axc = rgb(150, 156, 170);
    cv.fillRect(X0, Y1, X1 - X0, 1, axc);
    cv.fillRect(X0, Y0, 1, Y1 - Y0, axc);
    cv.fillRect(X1, Y0, 1, Y1 - Y0, axc);
    uint16_t cC = accent, cP = rgb(30, 110, 220), cE = rgb(20, 150, 70);
    const int NP = 120;
    int pc = 0, pp = 0, pe = 0, px = 0;
    for (int i = 0; i <= NP; i++) {
        double w = w0 * i / NP;
        double I = (u - k * w) / r;
        double C = k * (I - i0);
        double P = C * w;
        double eta = (u * I) > 0 ? P / (u * I) : 0;
        int x = X0 + (X1 - X0) * i / NP;
        int yc = Y1 - (int)(C / cd * (Y1 - Y0));
        int yp = Y1 - (int)(P / (pmax > 0 ? pmax : 1) * (Y1 - Y0) * 0.95);
        int ye = Y1 - (int)(eta * (Y1 - Y0));
        if (i) {
            cv.line(px, pc, x, yc, cC);
            cv.line(px, pc + 1, x, yc + 1, cC);
            cv.line(px, pp, x, yp, cP);
            cv.line(px, pe, x, ye, cE);
        }
        px = x; pc = yc; pp = yp; pe = ye;
    }
    // point de fonctionnement
    double w = s.v[OW];
    if (w > 0 && w < w0) {
        int x = X0 + (int)(w / w0 * (X1 - X0));
        for (int y = Y0; y < Y1; y += 3) cv.fillRect(x, y, 1, 1, rgb(200, 40, 50));
        int yc = Y1 - (int)(k * (s.v[OI] - i0) / cd * (Y1 - Y0));
        cv.disc(x, yc, 3, rgb(200, 40, 50));
    }
    char buf[40];
    fmtValue(buf, sizeof buf, n0, U_RPM, 3);
    font::drawText(cv, SANS_12, X1 - font::width(SANS_12, buf) / 2, Y1 + 14, buf, col::inkSoft);
    font::drawText(cv, SANS_12, X0 - 3, Y1 + 14, "0", col::inkSoft);
    font::drawText(cv, SANS_12, (X0 + X1) / 2 - 4, Y1 + 14, "N", col::inkSoft);
    fmtValue(buf, sizeof buf, cd, U_NM, 3);
    font::drawText(cv, SANS_12, X0 + 4, Y0 - 6, buf, cC);
    int lx = X1 - 64;
    font::drawText(cv, BOLD_12, lx, Y0 + 16, "C(N)", cC);
    font::drawRich(cv, BOLD_12, lx, Y0 + 32, "P_u(N)", cP);
    font::drawText(cv, BOLD_12, lx, Y0 + 48, "\xCE\xB7(N)", cE);
    fmtValue(buf, sizeof buf, pmax, U_W, 3);
    font::drawText(cv, SANS_12, X1 + 4, Y0 + 8, buf, cP);
    font::drawText(cv, SANS_12, X1 + 4, Y0 + 24, "100 %", cE);
}
}  // namespace mcc

// ==========================================================================
// Moteur pas a pas
// ==========================================================================
namespace step {
enum { ANG, F, N, PITCH, L, R, IN, V, OSPR, OUSPR, ORES, ON, OF, OV, OSPMM, OTAU, OTR, OFMAX, ONMAX };
enum { CMICRO, CGOAL };
enum { T_M = 1, T_W };
static const int MICRO[] = {1, 2, 4, 8, 16, 32, 64, 128, 256};
static void def(CalcState &s)
{
    s.v[ANG] = 1.8; s.v[F] = 1000; s.v[N] = 60; s.v[PITCH] = 8e-3; s.v[L] = 3e-3; s.v[R] = 1.5; s.v[IN] = 1.5; s.v[V] = 24;
    s.c[CMICRO] = 4;
}
static void rows(const CalcState &s, RowList &r)
{
    r.in("\\theta", ANG, U_DEG, ST_NONE, T_M, "Angle d'un pas entier (1,8° : 200 pas/tr)");
    r.choice("#Micropas", CMICRO, "1|2|4|8|16|32|64|128|256", T_M, "Division du pas par le pilote");
    r.choice("#Calculer", CGOAL, "vitesse depuis f|f depuis la vitesse", 0);
    if (s.c[CGOAL] == 0) r.in("f", F, U_HZ, ST_125, 0, "Fréquence des impulsions STEP");
    else r.in("N", N, U_RPM, ST_LIN10, 0, "Vitesse voulue");
    r.in("p", PITCH, U_M, ST_NONE, 0, "Pas de la vis ou avance par tour (0 : aucun)", RF_NONNEG);
    r.head("Résultats");
    r.out("#pas/tr", OSPR, U_NONE, "360 / θ");
    r.out("#micropas/tr", OUSPR, U_NONE, "Impulsions par tour");
    r.out("\\Delta\\theta", ORES, U_DEG, "Angle par impulsion");
    if (s.c[CGOAL] == 0) r.out("N", ON, U_RPM, "Vitesse de rotation", true);
    else r.out("f", OF, U_HZ, "Fréquence STEP à programmer", true);
    r.out("v", OV, U_MPS, "Vitesse linéaire avec la vis");
    r.out("#pas/mm", OSPMM, U_NONE, "Impulsions par millimètre");
    r.head("Enroulement");
    r.in("L", L, U_H, ST_E12, T_W, "Inductance d'une phase");
    r.in("R", R, U_OHM, ST_E24, T_W, "Résistance d'une phase");
    r.in("I_n", IN, U_A, ST_NONE, T_W, "Courant nominal par phase");
    r.in("V", V, U_V, ST_LIN1, T_W, "Tension d'alimentation du pilote");
    r.out("\\tau", OTAU, U_S, "Constante de temps L/R");
    r.out("t_m", OTR, U_S, "Temps d'établissement du courant −τ ln(1 − R I_n/V)");
    r.out("f_{\\max}", OFMAX, U_HZ, "Fréquence de pas entiers maximale ≈ 1/(2 t_m)");
    r.out("N_{\\max}", ONMAX, U_RPM, "Vitesse maximale estimée (couple chutant)");
    r.msg(0);
}
static void calc(CalcState &s)
{
    double spr = 360 / s.v[ANG];
    double mu = MICRO[s.c[CMICRO] < 9 ? s.c[CMICRO] : 0];
    s.v[OSPR] = spr;
    s.v[OUSPR] = spr * mu;
    s.v[ORES] = s.v[ANG] / mu;
    if (s.c[CGOAL] == 0) {
        s.v[ON] = s.v[F] / s.v[OUSPR] * 60;
        s.v[OF] = s.v[F];
    } else {
        s.v[OF] = s.v[N] / 60 * s.v[OUSPR];
        s.v[ON] = s.v[N];
    }
    double rev = (s.c[CGOAL] == 0 ? s.v[ON] : s.v[N]) / 60;
    s.v[OV] = s.v[PITCH] > 0 ? rev * s.v[PITCH] : NaN();
    s.v[OSPMM] = s.v[PITCH] > 0 ? s.v[OUSPR] / (s.v[PITCH] * 1e3) : NaN();
    s.v[OTAU] = s.v[L] / s.v[R];
    double x = s.v[R] * s.v[IN] / s.v[V];
    if (x < 1) {
        s.v[OTR] = -s.v[OTAU] * fm::ln(1 - x);
        s.v[OFMAX] = 1 / (2 * s.v[OTR]);
        s.v[ONMAX] = s.v[OFMAX] / spr * 60;
        setMsg(s, 0, "Plus V est grande devant R I_n, plus le moteur tourne vite", ML_INFO);
    } else {
        s.v[OTR] = s.v[OFMAX] = s.v[ONMAX] = NaN();
        setMsg(s, 0, "V trop faible : il faut V > R I_n", ML_BAD);
    }
}
static void sch(Sch &g, const CalcState &s)
{
    // rotor et deux phases
    uint16_t c = g.col(T_M);
    const int cx0 = 89, cy0 = 70;
    g.cv->circle(g.ox + cx0, g.oy + cy0, 36, g.ink);
    g.cv->disc(g.ox + cx0, g.oy + cy0, 14, g.on(T_M) ? g.light : mix16(g.paper, g.ink, 2));
    g.cv->circle(g.ox + cx0, g.oy + cy0, 14, c);
    // dents du rotor
    int nt = 12;
    for (int k = 0; k < nt; k++) {
        double a = fm::TWO_PI * k / nt;
        int x0 = cx0 + fm::iround(14 * fm::cos(a)), y0 = cy0 + fm::iround(14 * fm::sin(a));
        int x1 = cx0 + fm::iround(19 * fm::cos(a)), y1 = cy0 + fm::iround(19 * fm::sin(a));
        g.line(x0, y0, x1, y1, c);
    }
    // poles et bobines
    for (int k = 0; k < 4; k++) {
        double a = fm::TWO_PI * k / 4;
        int x = cx0 + fm::iround(28 * fm::cos(a)), y = cy0 + fm::iround(28 * fm::sin(a));
        uint16_t pc = g.col(T_W);
        g.cv->disc(g.ox + x, g.oy + y, 5, k % 2 ? mix16(g.paper, pc, 6) : mix16(g.paper, g.accent, 8));
    }
    g.texSmall(cx0, cy0 + 4, "#N S", tex::CENTER, T_M);
    char buf[40];
    int n = fmtValue(buf, sizeof buf, s.v[ORES], U_DEG, 3);
    scopy(buf + n, (int)sizeof(buf) - n, " par impulsion");
    font::drawText(*g.cv, SANS_12, g.ox + 89 - font::width(SANS_12, buf) / 2, g.oy + 126, buf, g.volt);
    valText(g, 89, 148, "N", s.v[ON], U_RPM, 1, true);
    valText(g, 89, 166, "f", s.v[OF], U_HZ, 1);
}
}  // namespace step

// ==========================================================================
// Moteur asynchrone triphase
// ==========================================================================
namespace async {
enum { U, F, P, N, PU, ETA, COS, ONS, OG, OW, OCU, OPA, OI, OQ, OS, OVENR, OJ, OPR };
enum { CCOUP };
enum { T_M = 1 };
static void def(CalcState &s) { s.v[U] = 400; s.v[F] = 50; s.v[P] = 2; s.v[N] = 1440; s.v[PU] = 1500; s.v[ETA] = 82; s.v[COS] = 0.8; }
static void rows(const CalcState &, RowList &r)
{
    r.in("U", U, U_V, ST_LIN10, 0, "Tension composée du réseau (entre phases)");
    r.in("f", F, U_HZ, ST_LIN1, 0, "Fréquence du réseau");
    r.in("p", P, U_NONE, ST_INT, T_M, "Nombre de paires de pôles", RF_POS | RF_INT);
    r.choice("#Couplage", CCOUP, "étoile (Y)|triangle (Δ)", T_M, "Couplage des enroulements du stator");
    r.in("N", N, U_RPM, ST_LIN10, T_M, "Vitesse du rotor (plaque signalétique)");
    r.in("P_u", PU, U_W, ST_125, T_M, "Puissance utile (mécanique)");
    r.in("\\eta", ETA, U_PCT, ST_LIN1, T_M, "Rendement");
    r.in("\\cos\\varphi", COS, U_NONE, ST_NONE, T_M, "Facteur de puissance");
    r.head("Résultats");
    r.out("n_s", ONS, U_RPM, "Vitesse de synchronisme 60 f/p", true);
    r.out("g", OG, U_PCT, "Glissement (n_s − N)/n_s", true);
    r.out("\\Omega", OW, U_RADS, "Vitesse angulaire du rotor");
    r.out("C_u", OCU, U_NM, "Couple utile P_u/Ω", true);
    r.out("P_a", OPA, U_W, "Puissance absorbée P_u/η");
    r.out("P_{pertes}", OPR, U_W, "Pertes totales P_a − P_u");
    r.out("I", OI, U_A, "Courant en ligne P_a/(√3 U cos φ)", true);
    r.out("Q", OQ, U_VAR, "Puissance réactive P_a tan φ");
    r.out("S", OS, U_VA, "Puissance apparente √3 U I");
    r.out("V_{enr}", OVENR, U_V, "Tension aux bornes d'un enroulement");
    r.out("J", OJ, U_A, "Courant dans un enroulement");
    r.msg(0);
}
static void calc(CalcState &s)
{
    const double S3 = 1.7320508075688772;
    s.v[ONS] = 60 * s.v[F] / s.v[P];
    s.v[OG] = (s.v[ONS] - s.v[N]) / s.v[ONS] * 100;
    s.v[OW] = s.v[N] * fm::TWO_PI / 60;
    s.v[OCU] = s.v[PU] / s.v[OW];
    s.v[OPA] = s.v[PU] / (s.v[ETA] / 100);
    s.v[OPR] = s.v[OPA] - s.v[PU];
    double c = s.v[COS], sn = fm::sqrt(1 - c * c);
    s.v[OI] = s.v[OPA] / (S3 * s.v[U] * c);
    s.v[OQ] = s.v[OPA] * sn / c;
    s.v[OS] = S3 * s.v[U] * s.v[OI];
    bool star = s.c[CCOUP] == 0;
    s.v[OVENR] = star ? s.v[U] / S3 : s.v[U];
    s.v[OJ] = star ? s.v[OI] : s.v[OI] / S3;
    if (s.v[N] >= s.v[ONS]) setMsg(s, 0, "N ≥ n_s : fonctionnement en génératrice ou à vide", ML_WARN);
    else if (!(c > 0 && c <= 1)) setMsg(s, 0, "cos φ doit être entre 0 et 1", ML_BAD);
    else setMsg(s, 0, "Fonctionnement moteur (0 < g < 1)", ML_OK);
}
static void sch(Sch &g, const CalcState &s)
{
    bool star = s.c[CCOUP] == 0;
    static const char *const ph[3] = {"#L1", "#L2", "#L3"};
    const int mx = 72, my = 98;
    for (int k = 0; k < 3; k++) {
        int y = 24 + 14 * k, x = mx - 12 + 12 * k;
        g.wire(24, y, x, y);
        g.wire(x, y, x, my - 18 + (k == 1 ? 0 : 3));
        g.dot(x, y);
        g.texSmall(4, y + 4, ph[k], tex::LEFT);
    }
    g.cv->disc(g.ox + mx, g.oy + my, 19, g.on(T_M) ? g.light : g.paper);
    g.cv->circle(g.ox + mx, g.oy + my, 19, g.col(T_M));
    g.cv->circle(g.ox + mx, g.oy + my, 18, g.col(T_M));
    tex::draw(*g.cv, "#M", g.ox + mx, g.oy + my + 2, g.col(T_M), tex::CENTER, 1);
    tex::draw(*g.cv, "#3~", g.ox + mx, g.oy + my + 14, g.col(T_M), tex::CENTER, 2);
    g.rect(mx + 19, my - 1, 16, 3, g.ink);
    // couplage des enroulements
    const int cx0 = 140, cy0 = 70;
    uint16_t c = g.col(T_M);
    if (star) {
        for (int k = 0; k < 3; k++) {
            double a = -fm::PI / 2 + fm::TWO_PI * k / 3;
            int x = cx0 + fm::iround(22 * fm::cos(a)), y = cy0 + fm::iround(22 * fm::sin(a));
            g.line(cx0, cy0, x, y, c);
            g.cv->disc(g.ox + (cx0 + x) / 2, g.oy + (cy0 + y) / 2, 3, c);
            g.term(x, y);
        }
        g.dot(cx0, cy0);
    } else {
        int px[3], py[3];
        for (int k = 0; k < 3; k++) {
            double a = -fm::PI / 2 + fm::TWO_PI * k / 3;
            px[k] = cx0 + fm::iround(24 * fm::cos(a));
            py[k] = cy0 + fm::iround(24 * fm::sin(a));
        }
        for (int k = 0; k < 3; k++) {
            int j = (k + 1) % 3;
            g.line(px[k], py[k], px[j], py[j], c);
            g.cv->disc(g.ox + (px[k] + px[j]) / 2, g.oy + (py[k] + py[j]) / 2, 3, c);
        }
        for (int k = 0; k < 3; k++) g.term(px[k], py[k]);
    }
    g.texSmall(cx0, cy0 + 40, star ? "#étoile" : "#triangle", tex::CENTER, T_M);
    valText(g, 4, 148, "n_s", s.v[ONS], U_RPM, 0, true);
    valText(g, 177, 148, "g", s.v[OG], U_PCT, 2, true);
    valText(g, 4, 166, "C_u", s.v[OCU], U_NM);
    valText(g, 177, 166, "I", s.v[OI], U_A, 2);
}
}  // namespace async

extern const CalcDef g_calcMotor[] = {
    {"Optocoupleur", "#R d'entrée, CTR, résistance de charge", "optocoupleur photocoupleur led phototransistor ctr isolation resistance", 14, G_NONE, 0,
     opto::def, opto::rows, opto::calc, opto::sch},
    {"Moteur à courant continu", "#Vitesse, courant, couple, rendement", "moteur courant continu mcc fem couple vitesse rendement induit", 14, G_CUSTOM, 0,
     mcc::def, mcc::rows, mcc::calc, mcc::sch, nullptr, nullptr, mcc::graph, "Courbes"},
    {"Moteur pas à pas", "#Micropas, vitesse, pas/mm, vitesse maximale", "moteur pas a pas stepper micropas vitesse vis pas mm pilote", 14, G_NONE, 0,
     step::def, step::rows, step::calc, step::sch},
    {"Moteur asynchrone triphasé", "#Glissement, couple, courant, couplage", "moteur asynchrone triphase glissement synchronisme couple courant etoile triangle", 14, G_NONE, 0,
     async::def, async::rows, async::calc, async::sch},
};
extern const int g_calcMotorN = sizeof(g_calcMotor) / sizeof(g_calcMotor[0]);
