// Section : antennes et radiofrequences (longueur d'onde, antennes filaires,
// bilan de liaison, ROS et abaque de Smith, adaptation en L, bruit,
// horizon radio et zone de Fresnel).
#include "CalcUtil.hpp"

using namespace cu;

static const double C0 = 299792458.0;
static const double KB = 1.380649e-23;

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

static const char *bandName(double f)
{
    if (f < 3e3) return "Bande : ELF/SLF/ULF (< 3 kHz)";
    if (f < 30e3) return "Bande : VLF (3 à 30 kHz)";
    if (f < 300e3) return "Bande : LF, ondes longues (30 à 300 kHz)";
    if (f < 3e6) return "Bande : MF, ondes moyennes (0,3 à 3 MHz)";
    if (f < 30e6) return "Bande : HF, ondes courtes (3 à 30 MHz)";
    if (f < 300e6) return "Bande : VHF (30 à 300 MHz)";
    if (f < 3e9) return "Bande : UHF (0,3 à 3 GHz)";
    if (f < 30e9) return "Bande : SHF, hyperfréquences (3 à 30 GHz)";
    if (f < 300e9) return "Bande : EHF, ondes millimétriques (30 à 300 GHz)";
    return "Au-delà des radiofréquences (> 300 GHz)";
}

// ==========================================================================
// Longueur d'onde
// ==========================================================================
namespace lam {
enum { F, LAM, K, OF, OL0, OL, OL2, OL4, OL8, OT, OBETA };
enum { CGOAL };
enum { T_W = 1 };
static void def(CalcState &s) { s.v[F] = 433.92e6; s.v[LAM] = 2; s.v[K] = 1; }
static void rows(const CalcState &s, RowList &r)
{
    r.choice("#Calculer", CGOAL, "λ depuis f|f depuis λ", 0);
    if (s.c[CGOAL] == 0) r.in("f", F, U_HZ, ST_125, T_W, "Fréquence");
    else r.in("\\lambda_0", LAM, U_M, ST_NONE, T_W, "Longueur d'onde dans le vide");
    r.in("k", K, U_NONE, ST_NONE, 0, "Coefficient de vélocité (vide : 1 ; coaxial PE : 0,66 ; mousse : 0,8)");
    r.head("Résultats");
    if (s.c[CGOAL] == 1) r.out("f", OF, U_HZ, "c / λ₀", true);
    else r.out("\\lambda_0", OL0, U_M, "Longueur d'onde dans le vide c/f", true);
    r.out("\\lambda", OL, U_M, "Dans le milieu (câble) : k λ₀", true);
    r.out("\\lambda/2", OL2, U_M, "Demi-onde (dans le milieu)");
    r.out("\\lambda/4", OL4, U_M, "Quart d'onde (dans le milieu)");
    r.out("\\lambda/8", OL8, U_M, "Huitième d'onde");
    r.out("T", OT, U_S, "Période 1/f");
    r.out("\\beta", OBETA, U_NONE, "Nombre d'onde 2π/λ (rad/m)");
    r.msg(0);
}
static void calc(CalcState &s)
{
    double f = s.c[CGOAL] == 0 ? s.v[F] : C0 / s.v[LAM];
    s.v[OF] = f;
    s.v[OL0] = C0 / f;
    s.v[OL] = s.v[K] * s.v[OL0];
    s.v[OL2] = s.v[OL] / 2;
    s.v[OL4] = s.v[OL] / 4;
    s.v[OL8] = s.v[OL] / 8;
    s.v[OT] = 1 / f;
    s.v[OBETA] = fm::TWO_PI / s.v[OL];
    setMsg(s, 0, bandName(f), ML_INFO);
}
static void sch(Sch &g, const CalcState &s)
{
    const int X0 = 14, X1 = 166, Y = 64, A = 30;
    g.line(X0, Y, X1, Y, g.label);
    int px = 0, py = 0;
    for (int i = 0; i <= 152; i++) {
        double a = fm::TWO_PI * i / 76.0;
        int x = (g.ox + X0 + i) * 16, y = (int)((g.oy + Y - A * fm::sin(a)) * 16);
        if (i) { g.cv->lineSub(px, py, x, y, g.col(T_W)); g.cv->lineSub(px, py + 8, x, y + 8, g.col(T_W)); }
        px = x; py = y;
    }
    g.harrow(X0, X0 + 76, Y + 44, nullptr, 1);
    g.cv->arrowHead(g.ox + X0, g.oy + Y + 44, -1, 0, 5, g.volt);
    g.tex(X0 + 38, Y + 40, "\\lambda", tex::CENTER);
    g.line(X0, Y + 36, X0, Y + 50, g.label);
    g.line(X0 + 76, Y + 36, X0 + 76, Y + 50, g.label);
    g.line(X0 + 76, Y - 34, X0 + 76 + 19, Y - 34, g.volt);
    g.texSmall(X0 + 86, Y - 38, "\\lambda/4", tex::CENTER);
    valText(g, 89, 148, "\xCE\xBB", s.v[OL], U_M, 1, true);
    valText(g, 89, 166, "\xCE\xBB/4", s.v[OL4], U_M, 1);
}
}  // namespace lam

// ==========================================================================
// Antennes filaires
// ==========================================================================
namespace ant {
enum { F, K, OL0, OLT, OLB, OLR, OZ, OG, OAE };
enum { CTYPE };
enum { T_A = 1 };
static const char *const TYPES = "dipôle λ/2|dipôle replié λ/2|quart d'onde (plan de sol)|quart d'onde, radians à 45°";
static void def(CalcState &s) { s.v[F] = 145e6; s.v[K] = 0.95; }
static void rows(const CalcState &s, RowList &r)
{
    int t = s.c[CTYPE];
    r.choice("#Antenne", CTYPE, TYPES, T_A, "Type d'antenne");
    r.in("f", F, U_HZ, ST_125, 0, "Fréquence de travail");
    r.in("K", K, U_NONE, ST_NONE, T_A, "Coefficient de raccourcissement (fil : 0,95 à 0,97 ; tube : 0,93)");
    r.head("Dimensions");
    r.out("\\lambda_0", OL0, U_M, "c / f");
    r.out("L", OLT, U_M, t <= 1 ? "Longueur totale K λ/2" : "Longueur du brin vertical K λ/4", true, T_A);
    if (t <= 1) r.out("L/2", OLB, U_M, "Longueur de chaque brin", true, T_A);
    if (t >= 2) r.out("L_r", OLR, U_M, "Longueur des radians (≈ λ/4)");
    r.head("Caractéristiques");
    r.out("Z", OZ, U_OHM, "Impédance au point d'alimentation (valeur théorique)");
    r.out("G", OG, U_DBI, "Gain théorique (plan de sol parfait pour le quart d'onde)");
    r.out("A_e\\ (\\text{m}^2)", OAE, U_NONE, "Surface effective G λ²/(4π)");
}
static void calc(CalcState &s)
{
    int t = s.c[CTYPE];
    double l0 = C0 / s.v[F];
    s.v[OL0] = l0;
    s.v[OLT] = t <= 1 ? s.v[K] * l0 / 2 : s.v[K] * l0 / 4;
    s.v[OLB] = s.v[OLT] / 2;
    s.v[OLR] = l0 / 4;
    static const double Z[4] = {73.1, 292.4, 36.5, 50};
    static const double G[4] = {2.15, 2.15, 5.15, 5.15};
    s.v[OZ] = Z[t];
    s.v[OG] = G[t];
    s.v[OAE] = fm::pow10(G[t] / 10) * l0 * l0 / (2 * fm::TWO_PI);
}
static void sch(Sch &g, const CalcState &s)
{
    int t = s.c[CTYPE];
    uint16_t c = g.col(T_A);
    if (t <= 1) {
        const int y = 60, x0 = 20, x1 = 158, m = 89;
        g.cv->fillRect(g.ox + x0, g.oy + y - 1, m - 4 - x0, 3, c);
        g.cv->fillRect(g.ox + m + 4, g.oy + y - 1, x1 - m - 4, 3, c);
        if (t == 1) {
            g.line(x0, y, x0, y - 10, c);
            g.line(x1, y, x1, y - 10, c);
            g.cv->fillRect(g.ox + x0, g.oy + y - 11, x1 - x0, 2, c);
        }
        g.wire(m - 4, y, m - 4, y + 40);
        g.wire(m + 4, y, m + 4, y + 40);
        g.texSmall(m, y + 54, "#câble", tex::CENTER);
        g.harrow(x0, x1, y + 22, nullptr, -1);
        g.cv->arrowHead(g.ox + x0, g.oy + y + 22, -1, 0, 5, g.volt);
        g.tex(50, y + 18, "L", tex::CENTER);
    } else {
        const int xb = 89, yb = 108, yt = 26;
        g.cv->fillRect(g.ox + xb - 1, g.oy + yt, 3, yb - yt, c);
        if (t == 2) {
            g.cv->fillRect(g.ox + 24, g.oy + yb, 130, 3, g.ink);
        } else {
            g.line(xb, yb, xb - 52, yb + 30, g.ink);
            g.line(xb, yb, xb + 52, yb + 30, g.ink);
            g.line(xb, yb + 1, xb - 52, yb + 31, g.ink);
            g.line(xb, yb + 1, xb + 52, yb + 31, g.ink);
        }
        g.varrow(xb + 14, yb, yt, "L", 1);
        g.cv->arrowHead(g.ox + xb + 14, g.oy + yb, 0, 1, 5, g.volt);
        g.wire(xb, yb + 2, xb, yb + 40);
    }
    valText(g, 89, 168, "L", s.v[OLT], U_M, 1, true);
}
}  // namespace ant

// ==========================================================================
// Bilan de liaison
// ==========================================================================
namespace link {
enum { PT, LT, GT, F, D, GR, LR, LO, SENS, OPIRE, OPIREW, OFSPL, OPR, OMARGE, ODMAX, OVR, OPRW };
enum { T_TX = 1, T_RX, T_PATH };
static void def(CalcState &s)
{
    s.v[PT] = 14; s.v[LT] = 1; s.v[GT] = 2.15; s.v[F] = 868e6; s.v[D] = 2e3; s.v[GR] = 2.15; s.v[LR] = 1; s.v[LO] = 10; s.v[SENS] = -120;
}
static double fspl(double d, double f) { return 20 * fm::log10(2 * fm::TWO_PI * d * f / C0); }
static void rows(const CalcState &, RowList &r)
{
    r.head("Émission");
    r.in("P_e", PT, U_DBM, ST_LIN1, T_TX, "Puissance de l'émetteur (0 dBm = 1 mW)", 0);
    r.in("L_e", LT, U_DB, ST_LIN1, T_TX, "Pertes du câble et des connecteurs côté émission", RF_NONNEG);
    r.in("G_e", GT, U_DBI, ST_LIN1, T_TX, "Gain de l'antenne d'émission", 0);
    r.head("Propagation");
    r.in("f", F, U_HZ, ST_125, T_PATH, "Fréquence");
    r.in("d", D, U_M, ST_125, T_PATH, "Distance entre les antennes");
    r.in("L_{sup}", LO, U_DB, ST_LIN1, T_PATH, "Pertes supplémentaires (obstacles, pluie, marge de fading)", RF_NONNEG);
    r.head("Réception");
    r.in("G_r", GR, U_DBI, ST_LIN1, T_RX, "Gain de l'antenne de réception", 0);
    r.in("L_r", LR, U_DB, ST_LIN1, T_RX, "Pertes du câble côté réception", RF_NONNEG);
    r.in("S", SENS, U_DBM, ST_LIN1, T_RX, "Sensibilité du récepteur", 0);
    r.head("Bilan");
    r.out("PIRE", OPIRE, U_DBM, "Puissance isotrope rayonnée équivalente P_e − L_e + G_e");
    r.out("PIRE", OPIREW, U_W, "PIRE en watts");
    r.out("FSL", OFSPL, U_DB, "Affaiblissement en espace libre 20 log(4π d f / c)", true, T_PATH);
    r.out("P_r", OPR, U_DBM, "Puissance reçue", true, T_RX);
    r.out("P_r", OPRW, U_W, "Puissance reçue en watts");
    r.out("V_r", OVR, U_V, "Tension efficace sur 50 Ω");
    r.out("M", OMARGE, U_DB, "Marge P_r − S", true);
    r.out("d_{\\max}", ODMAX, U_M, "Portée théorique (marge nulle, espace libre)");
    r.msg(0);
}
static void calc(CalcState &s)
{
    s.v[OPIRE] = s.v[PT] - s.v[LT] + s.v[GT];
    s.v[OPIREW] = fm::pow10(s.v[OPIRE] / 10) * 1e-3;
    s.v[OFSPL] = fspl(s.v[D], s.v[F]);
    s.v[OPR] = s.v[OPIRE] - s.v[OFSPL] - s.v[LO] + s.v[GR] - s.v[LR];
    s.v[OPRW] = fm::pow10(s.v[OPR] / 10) * 1e-3;
    s.v[OVR] = fm::sqrt(s.v[OPRW] * 50);
    s.v[OMARGE] = s.v[OPR] - s.v[SENS];
    s.v[ODMAX] = s.v[D] * fm::pow10(s.v[OMARGE] / 20);
    if (s.v[OMARGE] >= 10) setMsg(s, 0, "Liaison confortable (marge ≥ 10 dB)", ML_OK);
    else if (s.v[OMARGE] >= 0) setMsg(s, 0, "Liaison possible mais marge faible", ML_WARN);
    else setMsg(s, 0, "Liaison impossible : puissance reçue < sensibilité", ML_BAD);
}
static void sch(Sch &g, const CalcState &s)
{
    // deux antennes et la propagation
    auto antenna = [&](int x, int tag) {
        uint16_t c = g.col(tag);
        g.line(x - 8, 30, x, 46, c);
        g.line(x + 8, 30, x, 46, c);
        g.line(x - 8, 30, x + 8, 30, c);
        g.line(x, 46, x, 92, c);
        g.chip(x - 14, 92, 28, 18, nullptr, tag);
    };
    antenna(26, T_TX);
    antenna(152, T_RX);
    g.texSmall(26, 105, "#Tx", tex::CENTER, T_TX);
    g.texSmall(152, 105, "#Rx", tex::CENTER, T_RX);
    uint16_t c = g.col(T_PATH);
    for (int r = 8; r <= 24; r += 8) g.cv->halfCircle(g.ox + 34, g.oy + 40, r, 1, c);
    for (int x = 62; x < 120; x += 6) g.rect(x, 52, 3, 1, c);
    g.cv->arrowHead(g.ox + 124, g.oy + 52, 1, 0, 5, c);
    valText(g, 89, 72, "d", s.v[D], U_M, 1);
    valText(g, 89, 134, "P_r", s.v[OPR], U_DBM, 1, true);
    valText(g, 89, 150, "M", s.v[OMARGE], U_DB, 1, true);
    valText(g, 89, 166, "d_max", s.v[ODMAX], U_M, 1);
}
static void graph(Canvas &cv, const CalcState &s, const GraphArea &a, uint16_t accent)
{
    // P_r (dBm) en fonction de la distance (echelle log), sensibilite
    double d0 = s.v[D];
    double l0 = fm::floor(fm::log10(d0)) - 2, l1 = l0 + 5;
    double base = s.v[OPIRE] - s.v[LO] + s.v[GR] - s.v[LR];
    double pTop = base - fspl(fm::pow10(l0), s.v[F]), pBot = base - fspl(fm::pow10(l1), s.v[F]);
    double sens = s.v[SENS];
    double top = fm::ceil(fm::fmax(pTop, sens) / 20) * 20, bot = fm::floor(fm::fmin(pBot, sens) / 20) * 20;
    if (!fm::isfinite(top) || !fm::isfinite(bot)) return;
    if (top - bot > 300) bot = top - 300;
    const int X0 = a.x + 64, X1 = a.x + a.w - 22, Y0 = a.y + 20, Y1 = a.y + a.h - 28;
    auto xOf = [&](double l) { return X0 + (int)((l - l0) / (l1 - l0) * (X1 - X0)); };
    auto yOf = [&](double p) { return Y0 + (int)((top - p) / (top - bot) * (Y1 - Y0)); };
    uint16_t grid = rgb(226, 230, 238);
    char buf[32];
    for (double p = top; p >= bot - 1e-9; p -= 20) {
        int y = yOf(p);
        cv.fillRect(X0, y, X1 - X0, 1, grid);
        fmtValue(buf, sizeof buf, p, U_DBM, 3);
        font::drawText(cv, SANS_12, X0 - 3 - font::width(SANS_12, buf), y + 4, buf, col::inkSoft);
    }
    for (int d = (int)l0; d <= (int)l1; d++) {
        int x = xOf(d);
        cv.fillRect(x, Y0, 1, Y1 - Y0, grid);
        fmtValue(buf, sizeof buf, fm::pow10(d), U_M, 3);
        font::drawText(cv, SANS_12, x - font::width(SANS_12, buf) / 2, Y1 + 14, buf, col::inkSoft);
    }
    int ys = yOf(sens);
    cv.fillRect(X0, ys, X1 - X0, 1, rgb(200, 40, 50));
    font::drawText(cv, SANS_12, X1 - 80, ys - 4, "sensibilité", rgb(200, 40, 50));
    int px = 0, py = 0;
    for (int i = 0; i <= 200; i++) {
        double l = l0 + (l1 - l0) * i / 200;
        int x = xOf(l), y = yOf(base - fspl(fm::pow10(l), s.v[F]));
        if (i) { cv.line(px, py, x, y, accent); cv.line(px, py + 1, x, y + 1, accent); }
        px = x; py = y;
    }
    int xd = xOf(fm::log10(d0)), yd = yOf(s.v[OPR]);
    cv.disc(xd, yd, 3, accent);
    if (ok(s.v[ODMAX]) && s.v[ODMAX] > 0) {
        double lm = fm::log10(s.v[ODMAX]);
        if (lm >= l0 && lm <= l1) {
            int xm = xOf(lm);
            for (int y = Y0; y < Y1; y += 3) cv.fillRect(xm, y, 1, 1, rgb(200, 40, 50));
            font::drawText(cv, SANS_12, xm + 3, Y0 + 12, "d_max", rgb(200, 40, 50));
        }
    }
    font::drawText(cv, BOLD_12, X0 + 6, Y0 + 12, "P_r(d)", accent);
}
}  // namespace link

// ==========================================================================
// ROS, coefficient de reflexion et abaque de Smith
// ==========================================================================
namespace ros {
enum { Z0, RL, XL, LL, OGM, OGA, OROS, ORL, OML, OPR, OZIN, OPT };
enum { T_L = 1, T_LINE };
static void def(CalcState &s) { s.v[Z0] = 50; s.v[RL] = 75; s.v[XL] = 25; s.v[LL] = 0.1; }
static Cx gammaOf(double z0, Cx zl) { return (zl - cx(z0)) / (zl + cx(z0)); }
static void rows(const CalcState &, RowList &r)
{
    r.in("Z_0", Z0, U_OHM, ST_E24, T_LINE, "Impédance caractéristique de la ligne");
    r.in("R_L", RL, U_OHM, ST_E24, T_L, "Partie réelle de la charge", RF_NONNEG);
    r.in("X_L", XL, U_OHM, ST_NONE, T_L, "Partie imaginaire de la charge (+ inductif, − capacitif)", 0);
    r.head("Désadaptation");
    r.out("|\\Gamma|", OGM, U_NONE, "Module du coefficient de réflexion", true);
    r.out("\\arg\\Gamma", OGA, U_DEG, "Argument du coefficient de réflexion");
    r.out("ROS", OROS, U_NONE, "Rapport d'ondes stationnaires (1+|Γ|)/(1−|Γ|)", true);
    r.out("RL", ORL, U_DB, "Affaiblissement de réflexion −20 log|Γ|");
    r.out("ML", OML, U_DB, "Pertes de désadaptation −10 log(1−|Γ|²)");
    r.out("P_r/P_i", OPR, U_PCT, "Puissance réfléchie |Γ|²");
    r.out("P_t/P_i", OPT, U_PCT, "Puissance transmise 1 − |Γ|²");
    r.head("Ligne sans pertes");
    r.in("\\ell/\\lambda", LL, U_NONE, ST_NONE, T_LINE, "Longueur de la ligne en longueurs d'onde", RF_NONNEG);
    r.outC("Z_e", OZIN, U_OHM, false, "Impédance ramenée à l'entrée de la ligne", true);
}
static void calc(CalcState &s)
{
    Cx zl = cx(s.v[RL], s.v[XL]);
    Cx gm = gammaOf(s.v[Z0], zl);
    double m = cabs(gm);
    s.v[OGM] = m;
    s.v[OGA] = fm::deg(carg(gm));
    s.v[OROS] = m < 1 ? (1 + m) / (1 - m) : fm::inf();
    s.v[ORL] = m > 0 ? -20 * fm::log10(m) : fm::inf();
    s.v[OML] = m < 1 ? -10 * fm::log10(1 - m * m) : fm::inf();
    s.v[OPR] = m * m * 100;
    s.v[OPT] = (1 - m * m) * 100;
    double bl = fm::TWO_PI * s.v[LL];
    double c = fm::cos(bl), sn = fm::sin(bl);
    Cx z0 = cx(s.v[Z0]);
    Cx zin = z0 * (zl * c + cx(0, sn) * z0) / (z0 * c + cx(0, sn) * zl);
    s.v[OZIN] = zin.re;
    s.v[OZIN + 1] = zin.im;
}
static void sch(Sch &g, const CalcState &s)
{
    // abaque de Smith simplifie
    const int cx0 = 89, cy0 = 74, R = 62;
    Canvas &cv = *g.cv;
    int X = g.ox + cx0, Y = g.oy + cy0;
    uint16_t grid = rgb(200, 206, 218);
    cv.circle(X, Y, R, g.ink);
    cv.fillRect(X - R, Y, 2 * R, 1, grid);
    static const double rs[] = {0.2, 0.5, 1, 2, 5};
    for (double r : rs) {
        double c = r / (1 + r), rr = 1 / (1 + r);
        cv.circle(X + fm::iround(c * R), Y, fm::iround(rr * R), grid);
    }
    static const double xs[] = {0.2, 0.5, 1, 2, 5};
    for (double x : xs)
        for (int sg = -1; sg <= 1; sg += 2) {
            // arc de reactance x : centre (1, sg/x), rayon 1/x, dans le disque unite
            double rc = 1 / x;
            int px = 0, py = 0;
            bool have = false;
            for (int i = 0; i <= 120; i++) {
                double a = fm::TWO_PI * i / 120.0;
                double gx = 1 + rc * fm::cos(a), gy = sg * rc + rc * fm::sin(a);
                if (gx * gx + gy * gy > 1.0001) { have = false; continue; }
                int x1 = X + fm::iround(gx * R), y1 = Y - fm::iround(gy * R);
                if (have) cv.line(px, py, x1, y1, grid);
                px = x1; py = y1;
                have = true;
            }
        }
    // point de la charge et cercle de ROS
    Cx zl = cx(s.v[RL], s.v[XL]);
    Cx gm = gammaOf(s.v[Z0], zl);
    double m = cabs(gm);
    if (m <= 1 && m > 0.01) {
        int rr = fm::iround(m * R);
        for (int i = 0; i < 72; i += 2) {
            double a0 = fm::TWO_PI * i / 72, a1 = fm::TWO_PI * (i + 1) / 72;
            cv.line(X + fm::iround(rr * fm::cos(a0)), Y - fm::iround(rr * fm::sin(a0)), X + fm::iround(rr * fm::cos(a1)),
                    Y - fm::iround(rr * fm::sin(a1)), g.volt);
        }
    }
    if (m <= 1) cv.disc(X + fm::iround(gm.re * R), Y - fm::iround(gm.im * R), 4, g.col(T_L) == g.ink ? g.accent : g.accent);
    // point a l'entree de la ligne
    Cx zin = cx(s.v[OZIN], s.v[OZIN + 1]);
    Cx gi = gammaOf(s.v[Z0], zin);
    if (cabs(gi) <= 1) {
        cv.circle(X + fm::iround(gi.re * R), Y - fm::iround(gi.im * R), 4, g.col(T_LINE));
    }
    cv.disc(X, Y, 2, g.ink);
    valText(g, 4, 156, "ROS", s.v[OROS], U_NONE, 0, true);
    valText(g, 177, 156, "|\xCE\x93|", s.v[OGM], U_NONE, 2, true);
    font::drawText(cv, SANS_12, g.ox + 4, g.oy + 170, "\xE2\x97\x8F charge", g.accent);
    font::drawText(cv, SANS_12, g.ox + 92, g.oy + 170, "\xE2\x97\x8B entrée ligne", g.col(T_LINE));
}
}  // namespace ros

// ==========================================================================
// Adaptation par reseau en L
// ==========================================================================
namespace lmatch {
enum { RS, RL, F, OQ, OXS, OXP, OLS, OCS, OLP, OCP, OFB, OFH, OBW };
enum { CTYPE };
enum { T_S = 1, T_P, T_RS, T_RL };
static void def(CalcState &s) { s.v[RS] = 50; s.v[RL] = 300; s.v[F] = 14e6; }
static void rows(const CalcState &s, RowList &r)
{
    bool lp = s.c[CTYPE] == 0;
    r.in("R_s", RS, U_OHM, ST_E24, T_RS, "Résistance de la source");
    r.in("R_L", RL, U_OHM, ST_E24, T_RL, "Résistance de la charge");
    r.in("f", F, U_HZ, ST_125, 0, "Fréquence d'adaptation");
    r.choice("#Réseau", CTYPE, "passe-bas (L série, C parallèle)|passe-haut (C série, L parallèle)", 0);
    r.head("Éléments");
    r.out("Q", OQ, U_NONE, "√(R_haute/R_basse − 1)", true);
    r.out("X_s", OXS, U_OHM, "Réactance série Q R_basse (côté résistance basse)", false, T_S);
    r.out("X_p", OXP, U_OHM, "Réactance parallèle R_haute/Q (côté résistance haute)", false, T_P);
    if (lp) {
        r.out("L_s", OLS, U_H, "Inductance série X_s/ω", true, T_S);
        r.out("C_p", OCP, U_F, "Capacité parallèle 1/(ω X_p)", true, T_P);
    } else {
        r.out("C_s", OCS, U_F, "Capacité série 1/(ω X_s)", true, T_S);
        r.out("L_p", OLP, U_H, "Inductance parallèle X_p/ω", true, T_P);
    }
    r.head("Bande passante");
    r.out("f_1", OFB, U_HZ, "Fréquence basse à −3 dB (calcul exact)");
    r.out("f_2", OFH, U_HZ, "Fréquence haute à −3 dB");
    r.out("B", OBW, U_HZ, "Bande passante f₂ − f₁");
    r.msg(0);
}
// Gain en puissance disponible transmis a la charge (1 a l'adaptation).
static bool tfRaw(const CalcState &s, double f, Cx &H)
{
    double w = w2pi(f), rs = s.v[RS], rl = s.v[RL];
    bool lp = s.c[CTYPE] == 0, srcLow = rs < rl;
    double ls = s.v[OLS], cs = s.v[OCS], lpv = s.v[OLP], cp = s.v[OCP];
    Cx zs = lp ? cx(0, w * ls) : cx(0, -1 / (w * cs));
    Cx zp = lp ? cx(0, -1 / (w * cp)) : cx(0, w * lpv);
    Cx vl;
    if (srcLow) {
        // source - serie - (parallele // charge)
        Cx zpl = cpar(zp, cx(rl));
        vl = zpl / (cx(rs) + zs + zpl);
    } else {
        // source - (parallele) - serie - charge
        Cx zr = zs + cx(rl);
        Cx zx = cpar(zp, zr);
        Cx v1 = zx / (cx(rs) + zx);
        vl = v1 * cx(rl) / zr;
    }
    H = vl * (2 * fm::sqrt(rs / rl));
    return !cbad(H);
}
static bool tf(const CalcState &s, double f, Cx &H) { return tfRaw(s, f, H); }
static void calc(CalcState &s)
{
    double rs = s.v[RS], rl = s.v[RL], w = w2pi(s.v[F]);
    double rh = rs > rl ? rs : rl, rlo = rs > rl ? rl : rs;
    double q = fm::sqrt(rh / rlo - 1);
    s.v[OQ] = q;
    s.v[OXS] = q * rlo;
    s.v[OXP] = rh / q;
    s.v[OLS] = s.v[OXS] / w;
    s.v[OCS] = 1 / (w * s.v[OXS]);
    s.v[OLP] = s.v[OXP] / w;
    s.v[OCP] = 1 / (w * s.v[OXP]);
    s.v[OFB] = s.v[OFH] = s.v[OBW] = NaN();
    if (!(q > 0)) { setMsg(s, 0, "R_s = R_L : aucune adaptation nécessaire", ML_INFO); return; }
    auto g = [&](double l) {
        Cx H;
        tfRaw(s, fm::pow10(l), H);
        return cabs(H);
    };
    double l0 = fm::log10(s.v[F]), tgt = 1 / fm::sqrt(2.0);
    for (int side = 0; side < 2; side++) {
        double lo = l0, hi = l0;
        double stepD = side ? 0.01 : -0.01;
        bool found = false;
        for (int i = 0; i < 400; i++) {
            hi = lo + stepD;
            if (g(hi) < tgt) { found = true; break; }
            lo = hi;
        }
        if (!found) continue;
        for (int it = 0; it < 50; it++) {
            double md = 0.5 * (lo + hi);
            if (g(md) >= tgt) lo = md;
            else hi = md;
        }
        s.v[side ? OFH : OFB] = fm::pow10(0.5 * (lo + hi));
    }
    s.v[OBW] = s.v[OFH] - s.v[OFB];
    setMsg(s, 0, "Élément série du côté de la résistance la plus faible", ML_INFO);
}
static double fc(const CalcState &s) { return s.v[F]; }
static void sch(Sch &g, const CalcState &s)
{
    bool lp = s.c[CTYPE] == 0, srcLow = s.v[RS] < s.v[RL];
    const int yt = 48, yb = 120;
    g.vsrc(12, yt + 30, yb, 1, T_RS);
    g.wire(12, yt, 12, yt + 30);
    g.res(12, yt, 44, yt, T_RS);
    g.tex(28, yt - 10, "R_s", tex::CENTER, T_RS);
    g.wire(12, yb, 152, yb);
    g.res(152, yt, 152, yb, T_RL);
    g.tex(158, 90, "R_L", tex::LEFT, T_RL);
    int xp = srcLow ? 116 : 60;
    int xs0 = srcLow ? 44 : 60, xs1 = srcLow ? 116 : 136;
    if (lp) {
        g.ind(xs0 + 8, yt, xs1 - 8, yt, T_S);
        g.cap(xp, yt, xp, yb, T_P);
    } else {
        g.cap(xs0 + 8, yt, xs1 - 8, yt, T_S);
        g.ind(xp, yt, xp, yb, T_P);
    }
    g.wire(xs0, yt, xs0 + 8, yt);
    g.wire(xs1 - 8, yt, xs1, yt);
    if (srcLow) g.wire(116, yt, 152, yt);
    else { g.wire(44, yt, 60, yt); g.wire(136, yt, 152, yt); }
    g.dot(xp, yt);
    g.dot(xp, yb);
    g.tex((xs0 + xs1) / 2, yt - 10, lp ? "L_s" : "C_s", tex::CENTER, T_S);
    g.tex(xp + 9, 92, lp ? "C_p" : "L_p", tex::LEFT, T_P);
    valText(g, 89, 152, "Q", s.v[OQ], U_NONE, 1, true);
    valText(g, 89, 168, "B", s.v[OBW], U_HZ, 1);
}
}  // namespace lmatch

// ==========================================================================
// Bruit thermique et sensibilite
// ==========================================================================
namespace noise {
enum { T, B, NF, SNR, R, NF2, G1, ON, ONW, OVN, OS, OFT, ONFT };
enum { T_RX = 1 };
static void def(CalcState &s) { s.v[T] = 290; s.v[B] = 125e3; s.v[NF] = 6; s.v[SNR] = -7; s.v[R] = 50; s.v[NF2] = 10; s.v[G1] = 15; }
static void rows(const CalcState &, RowList &r)
{
    r.in("T", T, U_K, ST_LIN10, 0, "Température de bruit de la source (290 K de référence)");
    r.in("B", B, U_HZ, ST_125, 0, "Bande passante du récepteur");
    r.in("NF", NF, U_DB, ST_LIN1, T_RX, "Facteur de bruit du récepteur", RF_NONNEG);
    r.in("SNR_{\\min}", SNR, U_DB, ST_LIN1, 0, "Rapport signal/bruit minimal pour démoduler", 0);
    r.in("R", R, U_OHM, ST_E24, 0, "Résistance (tension de bruit)");
    r.head("Bruit et sensibilité");
    r.out("N", ON, U_DBM, "Puissance de bruit kTB", true);
    r.out("N", ONW, U_W, "kTB en watts");
    r.out("v_n", OVN, U_V, "Tension de bruit efficace √(4kTRB) (circuit ouvert)");
    r.out("S", OS, U_DBM, "Sensibilité kTB + NF + SNR_min", true);
    r.head("Deux étages (Friis)");
    r.in("G_1", G1, U_DB, ST_LIN1, 0, "Gain du premier étage (préampli)", 0);
    r.in("NF_2", NF2, U_DB, ST_LIN1, 0, "Facteur de bruit du second étage", RF_NONNEG);
    r.out("NF_{tot}", ONFT, U_DB, "F = F₁ + (F₂ − 1)/G₁ (NF₁ = NF ci-dessus)", true);
}
static void calc(CalcState &s)
{
    double n = KB * s.v[T] * s.v[B];
    s.v[ONW] = n;
    s.v[ON] = 10 * fm::log10(n / 1e-3);
    s.v[OVN] = fm::sqrt(4 * KB * s.v[T] * s.v[R] * s.v[B]);
    s.v[OS] = s.v[ON] + s.v[NF] + s.v[SNR];
    double f1 = fm::pow10(s.v[NF] / 10), f2 = fm::pow10(s.v[NF2] / 10), g1 = fm::pow10(s.v[G1] / 10);
    double ft = f1 + (f2 - 1) / g1;
    s.v[OFT] = ft;
    s.v[ONFT] = 10 * fm::log10(ft);
}
static void sch(Sch &g, const CalcState &s)
{
    // chaine de reception : antenne, preampli (G1, NF), second etage (NF2)
    uint16_t c = g.ink;
    g.line(14, 20, 20, 32, c);
    g.line(26, 20, 20, 32, c);
    g.line(14, 20, 26, 20, c);
    g.line(20, 32, 20, 50, c);
    g.wire(20, 50, 44, 50);
    auto amp = [&](int x, const char *lab) {
        g.cv->fillTri(g.ox + x, g.oy + 38, g.ox + x, g.oy + 62, g.ox + x + 28, g.oy + 50, g.paper);
        g.line(x, 38, x, 62, c);
        g.line(x, 38, x + 28, 50, c);
        g.line(x, 62, x + 28, 50, c);
        g.texSmall(x + 14, 78, lab, tex::CENTER);
    };
    amp(44, "G_1, NF");
    g.wire(72, 50, 96, 50);
    amp(96, "NF_2");
    g.wire(124, 50, 144, 50);
    g.chip(144, 40, 30, 20, nullptr, T_RX);
    g.texSmall(159, 54, "#dém.", tex::CENTER, T_RX);
    // niveaux
    char buf[64];
    int y = 102;
    struct L { const char *a; double v; uint8_t u; } lv[3] = {
        {"kTB : ", s.v[ON], U_DBM}, {"+ NF : ", s.v[ON] + s.v[NF], U_DBM}, {"+ SNR : ", s.v[OS], U_DBM}};
    for (int k = 0; k < 3; k++) {
        int n = scopy(buf, sizeof buf, lv[k].a);
        fmtValue(buf + n, (int)sizeof(buf) - n, lv[k].v, lv[k].u);
        font::drawText(*g.cv, k == 2 ? BOLD_12 : SANS_12, g.ox + 8, g.oy + y + 18 * k, buf, k == 2 ? g.accent : g.volt);
    }
    valText(g, 8, 164, "NF_tot", s.v[ONFT], U_DB);
}
}  // namespace noise

// ==========================================================================
// Horizon radio et zone de Fresnel
// ==========================================================================
namespace hor {
enum { H1, H2, D, F, OHO, OHR, OR1, OR60, OBULGE, OCLR };
enum { T_A = 1, T_Z };
static const double RE = 6371e3;
static void def(CalcState &s) { s.v[H1] = 10; s.v[H2] = 30; s.v[D] = 10e3; s.v[F] = 2.4e9; }
static void rows(const CalcState &, RowList &r)
{
    r.in("h_1", H1, U_M, ST_LIN1, T_A, "Hauteur de la première antenne");
    r.in("h_2", H2, U_M, ST_LIN1, T_A, "Hauteur de la seconde antenne");
    r.head("Horizon");
    r.out("d_{opt}", OHO, U_M, "Horizon optique √(2R)(√h₁ + √h₂)");
    r.out("d_{radio}", OHR, U_M, "Horizon radio (rayon terrestre ×4/3)", true);
    r.head("Zone de Fresnel");
    r.in("d", D, U_M, ST_125, T_Z, "Longueur de la liaison");
    r.in("f", F, U_HZ, ST_125, T_Z, "Fréquence");
    r.out("r_1", OR1, U_M, "Rayon de la 1ʳᵉ zone de Fresnel au milieu √(λ d/4)", true, T_Z);
    r.out("0{,}6\\,r_1", OR60, U_M, "Dégagement minimal conseillé (60 %)");
    r.out("h_t", OBULGE, U_M, "Renflement de la Terre au milieu d²/(8 k R) (k = 4/3)");
    r.out("h_{\\min}", OCLR, U_M, "Hauteur minimale de l'axe au milieu : h_t + 0,6 r₁");
    r.msg(0);
}
static void calc(CalcState &s)
{
    double a = fm::sqrt(s.v[H1]) + fm::sqrt(s.v[H2]);
    s.v[OHO] = fm::sqrt(2 * RE) * a;
    s.v[OHR] = fm::sqrt(2 * RE * 4 / 3) * a;
    double lam = C0 / s.v[F];
    s.v[OR1] = fm::sqrt(lam * s.v[D] / 4);
    s.v[OR60] = 0.6 * s.v[OR1];
    s.v[OBULGE] = s.v[D] * s.v[D] / (8 * RE * 4 / 3);
    s.v[OCLR] = s.v[OBULGE] + s.v[OR60];
    double hmid = (s.v[H1] + s.v[H2]) / 2;
    if (s.v[D] > s.v[OHR]) setMsg(s, 0, "d > horizon radio : pas de visibilité directe", ML_BAD);
    else if (hmid < s.v[OCLR]) setMsg(s, 0, "Axe trop bas au milieu : zone de Fresnel obstruée", ML_WARN);
    else setMsg(s, 0, "Dégagement suffisant (terrain plat supposé)", ML_OK);
}
static void sch(Sch &g, const CalcState &s)
{
    const int xa = 16, xb = 162, yg = 120;
    // sol courbe
    int px = 0, py = 0;
    for (int i = 0; i <= 40; i++) {
        double u = i / 40.0;
        int x = xa + (int)(u * (xb - xa)), y = yg - (int)(14 * 4 * u * (1 - u));
        if (i) g.line(px, py, x, y, g.ink);
        px = x; py = y;
    }
    double hmax = fm::fmax(s.v[H1], s.v[H2]);
    int ha = 20 + (int)(50 * s.v[H1] / hmax), hb = 20 + (int)(50 * s.v[H2] / hmax);
    g.cv->fillRect(g.ox + xa - 1, g.oy + yg - ha, 3, ha, g.col(T_A));
    g.cv->fillRect(g.ox + xb - 1, g.oy + yg - hb, 3, hb, g.col(T_A));
    g.line(xa, yg - ha, xb, yg - hb, g.volt);
    // ellipse de Fresnel
    uint16_t c = g.col(T_Z);
    for (int sg = -1; sg <= 1; sg += 2) {
        px = xa; py = yg - ha;
        for (int i = 1; i <= 40; i++) {
            double u = i / 40.0;
            int x = xa + (int)(u * (xb - xa));
            double yc = (yg - ha) + u * (ha - hb);
            int y = (int)(yc + sg * 22 * fm::sqrt(4 * u * (1 - u)));
            g.line(px, py, x, y, c);
            px = x; py = y;
        }
    }
    g.texSmall(xa, yg - ha - 4, "h_1", tex::CENTER, T_A);
    g.texSmall(xb, yg - hb - 4, "h_2", tex::CENTER, T_A);
    valText(g, 89, 150, "r_1", s.v[OR1], U_M, 1, true);
    valText(g, 89, 166, "horizon", s.v[OHR], U_M, 1);
}
}  // namespace hor

extern const CalcDef g_calcRf[] = {
    {"Longueur d'onde", "#λ = c/f, quart d'onde, câble coaxial", "longueur onde lambda frequence quart demi coaxial velocite bande", 15, G_NONE, 0,
     lam::def, lam::rows, lam::calc, lam::sch},
    {"Antennes filaires", "#Dipôle, quart d'onde : longueurs, impédance, gain", "antenne dipole quart onde fouet monopole longueur gain dbi impedance radian", 15, G_NONE, 0,
     ant::def, ant::rows, ant::calc, ant::sch},
    {"Bilan de liaison", "#PIRE, affaiblissement en espace libre, marge, portée", "bilan liaison fsl espace libre friis pire eirp sensibilite marge portee dbm", 15, G_CUSTOM, 0,
     link::def, link::rows, link::calc, link::sch, nullptr, nullptr, link::graph, "P_r(d)"},
    {"ROS et abaque de Smith", "#Γ, ROS, pertes, impédance ramenée", "ros vswr tos coefficient reflexion smith adaptation return loss ligne impedance", 15, G_NONE, 0,
     ros::def, ros::rows, ros::calc, ros::sch},
    {"Adaptation par réseau en L", "#L et C pour adapter deux résistances", "adaptation impedance reseau l lc match q bande passante", 15, G_BODE, 0,
     lmatch::def, lmatch::rows, lmatch::calc, lmatch::sch, lmatch::tf, lmatch::fc, nullptr, "Bode", 3},
    {"Bruit thermique et sensibilité", "#kTB, facteur de bruit, Friis", "bruit thermique ktb facteur bruit nf sensibilite friis snr", 15, G_NONE, 0,
     noise::def, noise::rows, noise::calc, noise::sch},
    {"Horizon radio et zone de Fresnel", "#Portée en visibilité, dégagement", "horizon radio fresnel degagement visibilite hauteur antenne", 15, G_NONE, 0,
     hor::def, hor::rows, hor::calc, hor::sch},
};
extern const int g_calcRfN = sizeof(g_calcRf) / sizeof(g_calcRf[0]);
