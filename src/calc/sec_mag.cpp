// Section : transformateurs et bobines (transformateur ideal et reel,
// dimensionnement, inductances couplees, inductance de mode commun,
// calcul d'une bobine).
#include "CalcUtil.hpp"

using namespace cu;

static const double MU0 = 1.25663706212e-6;

// Bobine verticale de n spires ; bosses a droite (right) ou a gauche.
static void coilV(Sch &g, int x, int y0, int y1, bool right, int tag, int n = 4)
{
    const int r = 4;
    int bl = 2 * r * n, mid = (y0 + y1) / 2, b0 = mid - bl / 2, b1 = b0 + bl;
    uint16_t c = g.col(tag);
    g.wire(x, y0, x, b0, tag);
    g.wire(x, b1, x, y1, tag);
    if (g.on(tag)) g.rect(right ? x : x - r - 1, b0, r + 2, bl + 1, g.light);
    for (int k = 0; k < n; k++) g.cv->halfCircle(g.ox + x, g.oy + b0 + r + 2 * r * k, r, right ? 1 : 3, c);
}

// Transformateur : primaire en x1 (bosses a droite), secondaire en x2,
// noyau entre les deux, points de polarite en haut.
static void xfo(Sch &g, int x1, int x2, int y0, int y1, int tag1, int tag2, bool core = true)
{
    coilV(g, x1, y0, y1, true, tag1);
    coilV(g, x2, y0, y1, false, tag2);
    int m = (x1 + x2) / 2, mid = (y0 + y1) / 2;
    if (core) {
        g.rect(m - 2, mid - 20, 1, 40, g.ink);
        g.rect(m + 1, mid - 20, 1, 40, g.ink);
    }
    g.cv->disc(g.ox + x1 + 9, g.oy + mid - 20, 2, g.ink);
    g.cv->disc(g.ox + x2 - 9, g.oy + mid - 20, 2, g.ink);
}

static void valText(Sch &g, int x, int y, const char *name, double v, uint8_t unit, int align = 0, bool bold = false)
{
    char buf[48];
    int k = 0;
    buf[0] = 0;
    if (name) { k = scopy(buf, sizeof buf, name); k += scopy(buf + k, (int)sizeof(buf) - k, " = "); }
    fmtValue(buf + k, (int)sizeof(buf) - k, v, unit, 3);
    int f = bold ? BOLD_12 : SANS_12;
    int w = font::richWidth(f, buf);
    if (align == 1) x -= w / 2;
    else if (align == 2) x -= w;
    if (x + w > 177) x = 177 - w;
    if (x < 2) x = 2;
    font::drawRich(*g.cv, f, g.ox + x, g.oy + y, buf, g.volt);
}

// ==========================================================================
// Transformateur ideal
// ==========================================================================
namespace ideal {
enum { N1, N2, M, V1, RL, OM, OV2, OI2, OI1, OS, OZ1 };
enum { CMODE };
enum { T_N1 = 1, T_N2, T_V1, T_RL };
static void def(CalcState &s) { s.v[N1] = 460; s.v[N2] = 24; s.v[M] = 0.05; s.v[V1] = 230; s.v[RL] = 10; }
static void rows(const CalcState &s, RowList &r)
{
    r.choice("#Données", CMODE, "N_1 et N_2|rapport m", 0, "Nombres de spires ou rapport de transformation");
    if (s.c[CMODE] == 0) {
        r.in("N_1", N1, U_NONE, ST_LIN10, T_N1, "Spires du primaire", RF_POS | RF_INT);
        r.in("N_2", N2, U_NONE, ST_LIN1, T_N2, "Spires du secondaire", RF_POS | RF_INT);
    } else {
        r.in("m", M, U_NONE, ST_NONE, 0, "Rapport de transformation N₂/N₁ = V₂/V₁");
    }
    r.in("V_1", V1, U_V, ST_LIN1, T_V1, "Tension efficace au primaire");
    r.in("R_L", RL, U_OHM, ST_E24, T_RL, "Charge au secondaire");
    r.head("Résultats");
    if (s.c[CMODE] == 0) r.out("m", OM, U_NONE, "m = N₂/N₁");
    r.out("V_2", OV2, U_V, "V₂ = m V₁", true);
    r.out("I_2", OI2, U_A, "Courant dans la charge V₂/R_L", false, T_RL);
    r.out("I_1", OI1, U_A, "I₁ = m I₂ (conservation de la puissance)");
    r.out("S", OS, U_VA, "Puissance apparente V₁ I₁ = V₂ I₂");
    r.out("R_L'", OZ1, U_OHM, "Charge ramenée au primaire R_L/m²", true);
}
static void calc(CalcState &s)
{
    double m = s.c[CMODE] == 0 ? s.v[N2] / s.v[N1] : s.v[M];
    s.v[OM] = m;
    s.v[OV2] = m * s.v[V1];
    s.v[OI2] = s.v[OV2] / s.v[RL];
    s.v[OI1] = m * s.v[OI2];
    s.v[OS] = s.v[OV2] * s.v[OI2];
    s.v[OZ1] = s.v[RL] / (m * m);
}
static void sch(Sch &g, const CalcState &s)
{
    const int yt = 46, yb = 118;
    xfo(g, 64, 112, yt, yb, T_N1, T_N2);
    g.wire(20, yt, 64, yt);
    g.wire(20, yb, 64, yb);
    g.vsrc(20, yt, yb, 1, T_V1);
    g.tex(36, 88, "V_1", tex::LEFT, T_V1);
    g.wire(112, yt, 150, yt);
    g.wire(112, yb, 150, yb);
    g.res(150, yt, 150, yb, T_RL);
    g.tex(158, 88, "R_L", tex::LEFT, T_RL);
    g.varrow(134, yb - 4, yt + 6, "V_2", -1);
    g.iarrow(42, yt, 1, 0, "I_1", 36, yt - 5);
    g.iarrow(136, yt, 1, 0, "I_2", 130, yt - 5);
    g.texSmall(64, yt - 16, "N_1", tex::CENTER, T_N1);
    g.texSmall(112, yt - 16, "N_2", tex::CENTER, T_N2);
    valText(g, 89, 148, "V_2", s.v[OV2], U_V, 1, true);
    valText(g, 89, 164, "m", s.v[OM], U_NONE, 1);
}
}  // namespace ideal

// ==========================================================================
// Transformateur reel : modele de Kapp (elements ramenes au secondaire)
// ==========================================================================
namespace kapp {
enum { V1, F, M, RS, LS, RF, LMU, ZR, ZC, ZL, OV20, OI2, OV2, ODV, OREG, OP2, OPJ, OPF, OETA, OI10, OI1, OCOS1 };
enum { CZ };
enum { T_V1 = 1, T_RS, T_LS, T_RF, T_LMU, T_Z };
static void def(CalcState &s)
{
    s.v[V1] = 230; s.v[F] = 50; s.v[M] = 0.105; s.v[RS] = 0.4; s.v[LS] = 1.5e-3; s.v[RF] = 15e3; s.v[LMU] = 20;
    s.v[ZR] = 10; s.v[ZC] = 100e-6; s.v[ZL] = 10e-3; s.c[CZ] = ZT_RL_S;
}
static void rows(const CalcState &s, RowList &r)
{
    r.in("V_1", V1, U_V, ST_LIN1, T_V1, "Tension efficace au primaire");
    r.in("f", F, U_HZ, ST_125, T_V1, "Fréquence du réseau");
    r.in("m", M, U_NONE, ST_NONE, 0, "Rapport de transformation à vide V₂₀/V₁");
    r.head("Modèle (au secondaire)");
    r.in("R_s", RS, U_OHM, ST_E24, T_RS, "Résistance des enroulements R₂ + m² R₁");
    r.in("L_s", LS, U_H, ST_E12, T_LS, "Inductance de fuite ramenée au secondaire");
    r.in("R_f", RF, U_OHM, ST_E24, T_RF, "Résistance des pertes fer (au primaire)");
    r.in("L_\\mu", LMU, U_H, ST_E12, T_LMU, "Inductance magnétisante (au primaire)");
    r.head("Charge");
    zRows(r, s, CZ, ZR, ZC, ZL, "Z_L", "R_L", "C_L", "L_L", T_Z);
    r.head("Résultats");
    r.out("V_{20}", OV20, U_V, "Tension à vide m V₁");
    r.out("V_2", OV2, U_V, "Tension en charge", true, T_Z);
    r.out("I_2", OI2, U_A, "Courant secondaire", true, T_Z);
    r.out("\\Delta V_2", ODV, U_V, "Chute de tension V₂₀ − V₂");
    r.out("\\Delta V_2/V_{20}", OREG, U_PCT, "Chute relative (régulation)");
    r.out("P_2", OP2, U_W, "Puissance active dans la charge");
    r.out("P_J", OPJ, U_W, "Pertes Joule R_s I₂²", false, T_RS);
    r.out("P_{fer}", OPF, U_W, "Pertes fer V₁²/R_f", false, T_RF);
    r.out("\\eta", OETA, U_PCT, "Rendement P₂/(P₂ + P_J + P_fer)", true);
    r.out("I_{10}", OI10, U_A, "Courant à vide au primaire", false, T_LMU);
    r.out("I_1", OI1, U_A, "Courant primaire (m I₂ + courant à vide)");
    r.out("\\cos\\varphi_1", OCOS1, U_NONE, "Facteur de puissance au primaire");
}
static void calc(CalcState &s)
{
    double w = w2pi(s.v[F]), v1 = s.v[V1], m = s.v[M];
    Cx zs = cx(s.v[RS], w * s.v[LS]);
    Cx zl = zOf(s, CZ, ZR, ZC, ZL, w);
    Cx i2 = cx(m * v1) / (zs + zl);
    Cx v2 = zl * i2;
    s.v[OV20] = m * v1;
    s.v[OI2] = cabs(i2);
    s.v[OV2] = cabs(v2);
    s.v[ODV] = s.v[OV20] - s.v[OV2];
    s.v[OREG] = s.v[ODV] / s.v[OV20] * 100;
    double i2m = s.v[OI2];
    s.v[OP2] = zl.re * i2m * i2m;
    s.v[OPJ] = s.v[RS] * i2m * i2m;
    s.v[OPF] = v1 * v1 / s.v[RF];
    s.v[OETA] = s.v[OP2] / (s.v[OP2] + s.v[OPJ] + s.v[OPF]) * 100;
    Cx i10 = cx(v1 / s.v[RF], -v1 / (w * s.v[LMU]));
    s.v[OI10] = cabs(i10);
    Cx i1 = m * i2 + i10;
    s.v[OI1] = cabs(i1);
    s.v[OCOS1] = i1.re / cabs(i1);
}
static void sch(Sch &g, const CalcState &s)
{
    const int yt = 46, yb = 118;
    g.vsrc(10, yt, yb, 1, T_V1);
    g.wire(10, yt, 60, yt);
    g.wire(10, yb, 60, yb);
    g.tex(10, yt - 6, "V_1", tex::CENTER, T_V1);
    g.res(30, yt, 30, yb, T_RF);
    g.ind(46, yt, 46, yb, T_LMU);
    g.dot(30, yt); g.dot(30, yb); g.dot(46, yt); g.dot(46, yb);
    g.texSmall(30, yb + 14, "R_f", tex::CENTER, T_RF);
    g.texSmall(50, yb + 14, "L_\\mu", tex::CENTER, T_LMU);
    xfo(g, 66, 96, yt, yb, 0, 0);
    g.wire(60, yt, 66, yt);
    g.wire(60, yb, 66, yb);
    g.texSmall(81, yt - 12, "m", tex::CENTER);
    g.res(96, yt, 122, yt, T_RS);
    g.ind(122, yt, 148, yt, T_LS);
    g.texSmall(109, yt - 9, "R_s", tex::CENTER, T_RS);
    g.texSmall(135, yt - 10, "L_s", tex::CENTER, T_LS);
    g.wire(148, yt, 166, yt);
    g.wire(96, yb, 166, yb);
    g.zdip(166, yt, 166, yb, s.c[CZ], T_Z);
    g.texSmall(158, 96, "Z_L", tex::RIGHT, T_Z);
    valText(g, 6, 150, "V_2", s.v[OV2], U_V, 0, true);
    valText(g, 96, 150, "\xCE\xB7", s.v[OETA], U_PCT, 0, true);
}
}  // namespace kapp

// ==========================================================================
// Dimensionnement d'un transformateur
// ==========================================================================
namespace dim {
enum { V1, V2, S, F, B, AE, J, ETA, COMP, ON1, ON2, ONV, OI1, OI2, OS1, OS2, OD1, OD2, OPHI, OAEC };
enum { T_P = 1, T_S, T_CORE };
static void def(CalcState &s)
{
    s.v[V1] = 230; s.v[V2] = 12; s.v[S] = 50; s.v[F] = 50; s.v[B] = 1.2; s.v[AE] = 8; s.v[J] = 3; s.v[ETA] = 85; s.v[COMP] = 8;
}
static void rows(const CalcState &, RowList &r)
{
    r.in("V_1", V1, U_V, ST_LIN1, T_P, "Tension efficace du primaire");
    r.in("V_2", V2, U_V, ST_LIN1, T_S, "Tension efficace voulue en charge au secondaire");
    r.in("S", S, U_VA, ST_125, T_S, "Puissance apparente au secondaire");
    r.in("f", F, U_HZ, ST_125, 0, "Fréquence");
    r.in("B_{\\max}", B, U_T, ST_NONE, T_CORE, "Induction maximale (tôles : 1 à 1,4 T ; ferrite : 0,2 à 0,3 T)");
    r.in("A_e", AE, U_CM2, ST_NONE, T_CORE, "Section du noyau (colonne centrale)");
    r.in("J", J, U_AMM2, ST_NONE, 0, "Densité de courant admise (2 à 4 A/mm²)");
    r.in("\\eta", ETA, U_PCT, ST_LIN1, 0, "Rendement estimé (pour I₁)");
    r.in("k_c", COMP, U_PCT, ST_LIN1, T_S, "Spires ajoutées au secondaire pour compenser la chute en charge", RF_NONNEG);
    r.head("Enroulements");
    r.out("n_V", ONV, U_NONE, "Spires par volt 1/(4,44 f B A_e)");
    r.out("N_1", ON1, U_NONE, "N₁ = V₁ n_V", true, T_P);
    r.out("N_2", ON2, U_NONE, "N₂ = V₂ n_V (1 + k_c)", true, T_S);
    r.out("\\Phi_{\\max}", OPHI, U_WB, "Flux maximal B A_e", false, T_CORE);
    r.head("Fils");
    r.out("I_1", OI1, U_A, "S/(η V₁)", false, T_P);
    r.out("I_2", OI2, U_A, "S/V₂", false, T_S);
    r.out("s_1", OS1, U_MM2, "Section du fil primaire I₁/J", false, T_P);
    r.out("s_2", OS2, U_MM2, "Section du fil secondaire I₂/J", false, T_S);
    r.out("d_1", OD1, U_M, "Diamètre du fil primaire (cuivre nu)", true, T_P);
    r.out("d_2", OD2, U_M, "Diamètre du fil secondaire (cuivre nu)", true, T_S);
    r.out("A_e\\approx\\sqrt{S}", OAEC, U_CM2, "Section usuelle à 50 Hz (repère empirique, S en VA)");
}
static void calc(CalcState &s)
{
    double ae = s.v[AE] * 1e-4;
    double nv = 1.0 / (4.44 * s.v[F] * s.v[B] * ae);
    s.v[ONV] = nv;
    s.v[ON1] = fm::ceil(s.v[V1] * nv - 1e-9);
    s.v[ON2] = fm::ceil(s.v[V2] * nv * (1 + s.v[COMP] / 100) - 1e-9);
    s.v[OPHI] = s.v[B] * ae;
    s.v[OI2] = s.v[S] / s.v[V2];
    s.v[OI1] = s.v[S] / (s.v[ETA] / 100 * s.v[V1]);
    s.v[OS1] = s.v[OI1] / s.v[J];
    s.v[OS2] = s.v[OI2] / s.v[J];
    s.v[OD1] = fm::sqrt(4 * s.v[OS1] / fm::PI) * 1e-3;
    s.v[OD2] = fm::sqrt(4 * s.v[OS2] / fm::PI) * 1e-3;
    s.v[OAEC] = fm::sqrt(s.v[S]);
}
static void sch(Sch &g, const CalcState &s)
{
    // noyau en E vu de face : deux fenetres, colonne centrale bobinee
    uint16_t c = g.col(T_CORE), fill = g.on(T_CORE) ? g.light : mix16(g.paper, g.ink, 2);
    const int x0 = 30, y0 = 26, W = 120, H = 96;
    g.rect(x0, y0, W, H, fill);
    g.frame(x0, y0, W, H, c);
    g.rect(x0 + 20, y0 + 18, 24, H - 36, g.paper);
    g.rect(x0 + W - 44, y0 + 18, 24, H - 36, g.paper);
    g.frame(x0 + 20, y0 + 18, 24, H - 36, c);
    g.frame(x0 + W - 44, y0 + 18, 24, H - 36, c);
    // enroulements dans les fenetres
    uint16_t cp = g.col(T_P), cs = g.col(T_S);
    for (int k = 0; k < 2; k++) {
        int wx = k ? x0 + W - 44 : x0 + 20;
        g.rect(wx + (k ? 1 : 13), y0 + 20, 10, H - 40, mix16(g.paper, cp, 6));
        g.rect(wx + (k ? 11 : 1), y0 + 20, 12, H - 40, mix16(g.paper, cs, 4));
    }
    g.texSmall(x0 + W / 2, y0 + H / 2 + 4, "A_e", tex::CENTER, T_CORE);
    valText(g, 6, 138, "N_1", s.v[ON1], U_NONE, 0, true);
    valText(g, 96, 138, "N_2", s.v[ON2], U_NONE, 0, true);
    valText(g, 6, 153, "d_1", s.v[OD1], U_M);
    valText(g, 6, 168, "d_2", s.v[OD2], U_M);
}
}  // namespace dim

// ==========================================================================
// Inductances couplees
// ==========================================================================
namespace coup {
enum { L1, L2, K, M, I1, I2, OM, OK, OLSA, OLSO, OLPA, OLPO, OTA, OTB, OTC, OLF, ON, OW };
enum { CGIV, CVIEW };
enum { T_L1 = 1, T_L2, T_M };
static void def(CalcState &s) { s.v[L1] = 10e-3; s.v[L2] = 2.2e-3; s.v[K] = 0.9; s.v[M] = 4e-3; s.v[I1] = 0.1; s.v[I2] = 0.1; }
static void rows(const CalcState &s, RowList &r)
{
    r.choice("#Schéma", CVIEW, "bobines|modèle en T", 0, "Bobines couplées ou schéma équivalent en T (bornes communes)");
    r.in("L_1", L1, U_H, ST_E12, T_L1, "Inductance propre de la bobine 1");
    r.in("L_2", L2, U_H, ST_E12, T_L2, "Inductance propre de la bobine 2");
    r.choice("#Couplage", CGIV, "coefficient k|mutuelle M", T_M, "Grandeur connue");
    if (s.c[CGIV] == 0) r.in("k", K, U_NONE, ST_NONE, T_M, "Coefficient de couplage (0 à 1)");
    else r.in("M", M, U_H, ST_E12, T_M, "Inductance mutuelle");
    r.head("Couplage");
    if (s.c[CGIV] == 0) r.out("M", OM, U_H, "M = k √(L₁ L₂)", true, T_M);
    else r.out("k", OK, U_NONE, "k = M / √(L₁ L₂)", true, T_M);
    r.out("n", ON, U_NONE, "Rapport équivalent √(L₂/L₁) (k ≈ 1)");
    r.out("L_{f1}", OLF, U_H, "Inductance de fuite vue du primaire (secondaire en court-circuit) (1 − k²) L₁");
    r.head("Associations");
    r.out("L_{s+}", OLSA, U_H, "Série, flux additifs L₁ + L₂ + 2M", true);
    r.out("L_{s-}", OLSO, U_H, "Série, flux soustractifs L₁ + L₂ − 2M");
    r.out("L_{p+}", OLPA, U_H, "Parallèle, flux additifs (L₁L₂ − M²)/(L₁ + L₂ − 2M)");
    r.out("L_{p-}", OLPO, U_H, "Parallèle, flux soustractifs (L₁L₂ − M²)/(L₁ + L₂ + 2M)");
    r.head("Modèle en T");
    r.out("L_1-M", OTA, U_H, "Branche côté 1 (peut être négative)", false, T_L1);
    r.out("L_2-M", OTB, U_H, "Branche côté 2", false, T_L2);
    r.out("M", OTC, U_H, "Branche commune", false, T_M);
    r.head("Énergie");
    r.in("I_1", I1, U_A, ST_125, T_L1, "Courant dans la bobine 1 (entrant par le point)", 0);
    r.in("I_2", I2, U_A, ST_125, T_L2, "Courant dans la bobine 2 (entrant par le point)", 0);
    r.out("W", OW, U_J, "½ L₁I₁² + ½ L₂I₂² + M I₁I₂");
    r.msg(0);
}
static void calc(CalcState &s)
{
    double l1 = s.v[L1], l2 = s.v[L2], r = fm::sqrt(l1 * l2);
    double m = s.c[CGIV] == 0 ? s.v[K] * r : s.v[M];
    double k = m / r;
    s.v[OM] = m;
    s.v[OK] = k;
    s.v[ON] = fm::sqrt(l2 / l1);
    s.v[OLF] = (1 - k * k) * l1;
    s.v[OLSA] = l1 + l2 + 2 * m;
    s.v[OLSO] = l1 + l2 - 2 * m;
    double d = l1 * l2 - m * m;
    s.v[OLPA] = d / (l1 + l2 - 2 * m);
    s.v[OLPO] = d / (l1 + l2 + 2 * m);
    s.v[OTA] = l1 - m;
    s.v[OTB] = l2 - m;
    s.v[OTC] = m;
    s.v[OW] = 0.5 * l1 * s.v[I1] * s.v[I1] + 0.5 * l2 * s.v[I2] * s.v[I2] + m * s.v[I1] * s.v[I2];
    if (!(k >= 0 && k <= 1 + 1e-12)) setMsg(s, 0, "Impossible : il faut 0 ≤ k ≤ 1 (M ≤ √(L₁L₂))", ML_BAD);
    else if (k > 0.95) setMsg(s, 0, "Couplage serré : comportement de transformateur", ML_OK);
    else setMsg(s, 0, "Couplage lâche : fuites importantes", ML_INFO);
}
static void sch(Sch &g, const CalcState &s)
{
    const int yt = 44, yb = 120;
    if (s.c[CVIEW] == 0) {
        xfo(g, 66, 112, yt, yb, T_L1, T_L2, false);
        g.wire(30, yt, 66, yt);
        g.wire(30, yb, 66, yb);
        g.wire(112, yt, 148, yt);
        g.wire(112, yb, 148, yb);
        g.term(30, yt); g.term(30, yb); g.term(148, yt); g.term(148, yb);
        g.tex(58, 88, "L_1", tex::RIGHT, T_L1);
        g.tex(120, 88, "L_2", tex::LEFT, T_L2);
        // double fleche M
        uint16_t c = g.col(T_M);
        g.line(76, 66, 102, 66, c);
        g.cv->arrowHead(g.ox + 76, g.oy + 66, -1, 0, 4, c);
        g.cv->arrowHead(g.ox + 102, g.oy + 66, 1, 0, 4, c);
        g.tex(89, 60, "M", tex::CENTER, T_M);
        g.iarrow(44, yt, 1, 0, "I_1", 38, yt - 5, T_L1);
        g.iarrow(134, yt, -1, 0, "I_2", 130, yt - 5, T_L2);
    } else {
        g.ind(26, yt, 74, yt, T_L1);
        g.ind(104, yt, 152, yt, T_L2);
        g.wire(74, yt, 104, yt);
        g.ind(89, yt, 89, yb, T_M);
        g.dot(89, yt);
        g.dot(89, yb);
        g.wire(14, yb, 164, yb);
        g.wire(14, yt, 26, yt);
        g.wire(152, yt, 164, yt);
        g.term(14, yt); g.term(14, yb); g.term(164, yt); g.term(164, yb);
        g.tex(50, yt - 10, "L_1-M", tex::CENTER, T_L1);
        g.tex(128, yt - 10, "L_2-M", tex::CENTER, T_L2);
        g.tex(98, 88, "M", tex::LEFT, T_M);
    }
    valText(g, 89, 150, s.c[CGIV] == 0 ? "M" : "k", s.c[CGIV] == 0 ? s.v[OM] : s.v[OK], s.c[CGIV] == 0 ? U_H : U_NONE, 1, true);
    valText(g, 89, 166, "L_s+", s.v[OLSA], U_H, 1);
}
}  // namespace coup

// ==========================================================================
// Inductance de mode commun
// ==========================================================================
namespace cmc {
enum { AL, N, F, CP, RR, KF, OL, OLD, OZ, OPHI, OSRF, OIL, OZD };
enum { T_L = 1, T_CP };
static void def(CalcState &s) { s.v[AL] = 5e-6; s.v[N] = 15; s.v[F] = 1e6; s.v[CP] = 10e-12; s.v[RR] = 50; s.v[KF] = 1; }
static Cx zcm(const CalcState &s, double w)
{
    double L = s.v[AL] * s.v[N] * s.v[N];
    Cx zl = cx(0, w * L);
    if (s.v[CP] > 0) return cpar(zl, cx(0, -1.0 / (w * s.v[CP])));
    return zl;
}
static void rows(const CalcState &, RowList &r)
{
    r.in("A_L", AL, U_H, ST_E12, T_L, "Inductance spécifique du tore (par spire²)");
    r.in("N", N, U_NONE, ST_LIN1, T_L, "Nombre de spires de chaque enroulement", RF_POS | RF_INT);
    r.in("C_p", CP, U_F, ST_E12, T_CP, "Capacité parasite entre spires (0 : ignorée)", RF_NONNEG);
    r.in("k_f", KF, U_PCT, ST_NONE, T_L, "Inductance de fuite (mode différentiel) en % de L_mc", RF_NONNEG);
    r.in("R", RR, U_OHM, ST_E24, 0, "Impédance du système de chaque côté (50 Ω pour un RSIL)");
    r.in("f", F, U_HZ, ST_125, 0, "Fréquence de la perturbation");
    r.head("Résultats");
    r.out("L_{mc}", OL, U_H, "Inductance de mode commun A_L N²", true, T_L);
    r.out("L_{md}", OLD, U_H, "Inductance de fuite vue en mode différentiel");
    r.out("|Z_{mc}|", OZ, U_OHM, "Impédance de mode commun à f (avec C_p)", true);
    r.out("\\varphi", OPHI, U_DEG, "Argument de Z_mc : +90° inductif, −90° capacitif");
    r.out("f_{r}", OSRF, U_HZ, "Résonance propre 1/(2π√(L C_p))", false, T_CP);
    r.out("IL", OIL, U_DB, "Atténuation d'insertion 20 log|1 + Z_mc/(2R)|", true);
    r.out("|Z_{md}|", OZD, U_OHM, "Impédance en mode différentiel ω L_md");
    r.msg(0);
}
static void calc(CalcState &s)
{
    double w = w2pi(s.v[F]);
    double L = s.v[AL] * s.v[N] * s.v[N];
    s.v[OL] = L;
    s.v[OLD] = L * s.v[KF] / 100;
    Cx z = zcm(s, w);
    s.v[OZ] = cabs(z);
    s.v[OPHI] = fm::deg(carg(z));
    s.v[OSRF] = s.v[CP] > 0 ? 1.0 / (fm::TWO_PI * fm::sqrt(L * s.v[CP])) : NaN();
    s.v[OIL] = db(cabs(cx(1) + z / (2 * s.v[RR])));
    s.v[OZD] = w * s.v[OLD];
    if (s.v[CP] > 0 && s.v[F] > s.v[OSRF]) setMsg(s, 0, "Au-delà de f_r : la bobine devient capacitive", ML_WARN);
    else setMsg(s, 0, "Les courants utiles (mode différentiel) ne saturent pas le noyau", ML_INFO);
}
static bool tf(const CalcState &s, double f, Cx &H)
{
    Cx z = zcm(s, w2pi(f));
    H = cx(2 * s.v[RR]) / (cx(2 * s.v[RR]) + z);
    return !cbad(H);
}
static double fc(const CalcState &s)
{
    double L = s.v[AL] * s.v[N] * s.v[N];
    return s.v[RR] / (fm::TWO_PI * L) * 4;
}
static void sch(Sch &g, const CalcState &s)
{
    // deux enroulements sur un tore, sens des courants
    const int cx0 = 89, cy0 = 72;
    uint16_t c = g.col(T_L);
    g.cv->circle(g.ox + cx0, g.oy + cy0, 30, c);
    g.cv->circle(g.ox + cx0, g.oy + cy0, 18, c);
    for (int k = 0; k < 5; k++) {
        int x = cx0 - 22 + k * 3;
        g.line(x, cy0 - 30 + k * 2, x + 6, cy0 - 14 + k * 3, mix16(c, g.paper, 4));
        int x2 = cx0 + 10 + k * 3;
        g.line(x2, cy0 + 14 - k * 3, x2 + 6, cy0 + 30 - k * 2, mix16(c, g.paper, 4));
    }
    g.wire(10, 40, 60, 40);
    g.wire(118, 40, 168, 40);
    g.wire(10, 104, 60, 104);
    g.wire(118, 104, 168, 104);
    g.texSmall(10, 54, "#Phase", tex::LEFT);
    g.texSmall(10, 118, "#Neutre", tex::LEFT);
    g.iarrow(40, 40, 1, 0, "I", 36, 34);
    g.iarrow(40, 104, -1, 0, "I", 36, 98);
    // courant de mode commun : meme sens sur les deux fils
    uint16_t r = rgb(200, 50, 50);
    g.cv->arrowHead(g.ox + 150, g.oy + 46, 1, 0, 4, r);
    g.cv->arrowHead(g.ox + 150, g.oy + 110, 1, 0, 4, r);
    font::drawText(*g.cv, SANS_12, g.ox + 177 - font::width(SANS_12, "mode commun"), g.oy + 128, "mode commun", r);
    valText(g, 89, 152, "L_mc", s.v[OL], U_H, 1, true);
    valText(g, 89, 168, "IL", s.v[OIL], U_DB, 1);
    (void)s;
}
}  // namespace cmc

// ==========================================================================
// Inductance d'une bobine
// ==========================================================================
namespace coil {
enum { N, L, D, LEN, MUR, H, DE, DI, AL, I, OL, ON, OW, OB, OLW };
enum { CTYPE, CGOAL };
enum { T_C = 1 };
static void def(CalcState &s)
{
    s.v[N] = 50; s.v[L] = 100e-6; s.v[D] = 20e-3; s.v[LEN] = 40e-3; s.v[MUR] = 2000; s.v[H] = 10e-3;
    s.v[DE] = 25e-3; s.v[DI] = 15e-3; s.v[AL] = 2e-6; s.v[I] = 1;
}
static double factor(const CalcState &s)
{
    // L = factor * N^2
    switch (s.c[CTYPE]) {
    case 0: {
        double a = fm::PI * s.v[D] * s.v[D] / 4;
        return MU0 * a / (s.v[LEN] + 0.45 * s.v[D]);
    }
    case 1: return MU0 * s.v[MUR] * s.v[H] * fm::ln(s.v[DE] / s.v[DI]) / fm::TWO_PI;
    default: return s.v[AL];
    }
}
static void rows(const CalcState &s, RowList &r)
{
    int t = s.c[CTYPE];
    r.choice("#Bobine", CTYPE, "solénoïde à air|tore|noyau A_L", T_C, "Forme de la bobine");
    r.choice("#Calculer", CGOAL, "L (N connu)|N (L voulue)", 0, "Grandeur cherchée");
    if (s.c[CGOAL] == 0) r.in("N", N, U_NONE, ST_LIN1, T_C, "Nombre de spires", RF_POS | RF_INT);
    else r.in("L", L, U_H, ST_E12, T_C, "Inductance voulue");
    if (t == 0) {
        r.in("d", D, U_M, ST_NONE, T_C, "Diamètre moyen des spires");
        r.in("\\ell", LEN, U_M, ST_NONE, T_C, "Longueur du bobinage");
    } else if (t == 1) {
        r.in("\\mu_r", MUR, U_NONE, ST_125, T_C, "Perméabilité relative du matériau");
        r.in("h", H, U_M, ST_NONE, T_C, "Hauteur du tore");
        r.in("D_e", DE, U_M, ST_NONE, T_C, "Diamètre extérieur");
        r.in("D_i", DI, U_M, ST_NONE, T_C, "Diamètre intérieur");
    } else {
        r.in("A_L", AL, U_H, ST_E12, T_C, "Inductance spécifique (notice du noyau, par spire²)");
    }
    r.in("I", I, U_A, ST_125, 0, "Courant (énergie et induction)", RF_NONNEG);
    r.head("Résultats");
    if (s.c[CGOAL] == 0) r.out("L", OL, U_H, t == 0 ? "Formule de Wheeler μ₀ N² A/(ℓ + 0,45 d)" : t == 1 ? "μ₀ μ_r N² h ln(D_e/D_i)/(2π)" : "A_L N²", true);
    else {
        r.out("N", ON, U_NONE, "Spires arrondies à l'entier supérieur", true);
        r.out("L", OL, U_H, "Inductance obtenue avec N entier");
    }
    r.out("W", OW, U_J, "Énergie ½ L I²");
    if (t != 2) r.out("B", OB, U_T, t == 0 ? "Induction au centre ≈ μ₀ N I/ℓ" : "Induction moyenne μ₀ μ_r N I/ℓ_m");
    if (t == 0) r.out("\\ell_{fil}", OLW, U_M, "Longueur de fil ≈ π d N");
}
static void calc(CalcState &s)
{
    double k = factor(s);
    double n = s.c[CGOAL] == 0 ? s.v[N] : fm::ceil(fm::sqrt(s.v[L] / k) - 1e-9);
    s.v[ON] = n;
    s.v[OL] = k * n * n;
    s.v[OW] = 0.5 * s.v[OL] * s.v[I] * s.v[I];
    if (s.c[CTYPE] == 0) s.v[OB] = MU0 * n * s.v[I] / s.v[LEN];
    else if (s.c[CTYPE] == 1) s.v[OB] = MU0 * s.v[MUR] * n * s.v[I] / (fm::PI * (s.v[DE] + s.v[DI]) / 2);
    else s.v[OB] = NaN();
    s.v[OLW] = fm::PI * s.v[D] * n;
}
static void sch(Sch &g, const CalcState &s)
{
    uint16_t c = g.col(T_C);
    int t = s.c[CTYPE];
    if (t == 0) {
        // solenoide vu de cote : spires inclinees
        const int x0 = 40, x1 = 140, yt = 46, yb = 98;
        for (int k = 0; k <= 10; k++) {
            int x = x0 + (x1 - x0) * k / 10;
            g.line(x, yt, x + 6, yb, c);
            if (k < 10) g.line(x + 6, yb, x + 10, yt, mix16(c, g.paper, 5));
        }
        g.wire(20, yt, x0, yt);
        g.wire(x1 + 6, yb, 160, yb);
        g.harrow(x0, x1 + 6, 116, "\\ell", 1);
        g.varrow(28, yb, yt, "d", -1);
    } else if (t == 1) {
        g.cv->circle(g.ox + 89, g.oy + 72, 34, c);
        g.cv->circle(g.ox + 89, g.oy + 72, 20, c);
        for (int k = 0; k < 12; k++) {
            double a = fm::TWO_PI * k / 12;
            int x0 = 89 + fm::iround(17 * fm::cos(a)), y0 = 72 + fm::iround(17 * fm::sin(a));
            int x1 = 89 + fm::iround(37 * fm::cos(a)), y1 = 72 + fm::iround(37 * fm::sin(a));
            g.line(x0, y0, x1, y1, mix16(c, g.paper, 3));
        }
        g.harrow(55, 123, 118, "D_e", 1);
    } else {
        g.ind(30, 72, 148, 72, T_C);
        g.rect(40, 60, 98, 2, c);
        g.rect(40, 63, 98, 2, c);
    }
    valText(g, 89, 152, "L", s.v[OL], U_H, 1, true);
    valText(g, 89, 168, "N", s.v[ON], U_NONE, 1);
}
}  // namespace coil

extern const CalcDef g_calcMag[] = {
    {"Transformateur idéal", "#V₂ = m V₁, I₁ = m I₂, charge ramenée", "transformateur ideal rapport spires tension courant impedance ramenee", 11, G_NONE, 0,
     ideal::def, ideal::rows, ideal::calc, ideal::sch},
    {"Transformateur réel (modèle de Kapp)", "#Chute de tension, pertes, rendement", "transformateur reel kapp chute tension pertes fer joule rendement fuite magnetisant", 11, G_NONE, 0,
     kapp::def, kapp::rows, kapp::calc, kapp::sch},
    {"Dimensionnement d'un transformateur", "#Spires par volt, N₁, N₂, diamètre des fils", "transformateur dimensionnement spires volt noyau induction fil section densite courant", 11, G_NONE, 0,
     dim::def, dim::rows, dim::calc, dim::sch},
    {"Inductances couplées", "#Mutuelle, associations, modèle en T", "inductance couplee mutuelle couplage k serie parallele modele t fuite", 11, G_NONE, 0,
     coup::def, coup::rows, coup::calc, coup::sch},
    {"Inductance de mode commun", "#L_mc, impédance, atténuation, résonance", "inductance mode commun self cem filtre secteur tore attenuation insertion", 11, G_BODE, 0,
     cmc::def, cmc::rows, cmc::calc, cmc::sch, cmc::tf, cmc::fc, nullptr, "Atténuation", 3, nullptr, nullptr, nullptr, 7},
    {"Inductance d'une bobine", "#Solénoïde à air, tore, noyau A_L", "inductance bobine solenoide tore al spires wheeler calcul", 11, G_NONE, 0,
     coil::def, coil::rows, coil::calc, coil::sch},
};
extern const int g_calcMagN = sizeof(g_calcMag) / sizeof(g_calcMag[0]);
