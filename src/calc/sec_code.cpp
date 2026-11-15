// Section : code couleur des resistances, series E, marquages CMS.
#include "CalcUtil.hpp"

using namespace cu;

static const char *const COLORS = "Noir|Marron|Rouge|Orange|Jaune|Vert|Bleu|Violet|Gris|Blanc";
static const char *const COLOR_NAMES[10] = {"Noir", "Marron", "Rouge", "Orange", "Jaune",
                                             "Vert", "Bleu", "Violet", "Gris", "Blanc"};
static const char *const MULTS =
    "Argent ×0,01|Or ×0,1|Noir ×1|Marron ×10|Rouge ×100|Orange ×1 k|Jaune ×10 k|Vert ×100 k|Bleu ×1 M|Violet ×10 M|Gris ×100 M|Blanc ×1 G";
static const char *const MULT_NAMES[12] = {"Argent", "Or", "Noir", "Marron", "Rouge", "Orange",
                                            "Jaune", "Vert", "Bleu", "Violet", "Gris", "Blanc"};
static const char *const TOLS = "Marron ±1 %|Rouge ±2 %|Vert ±0,5 %|Bleu ±0,25 %|Violet ±0,1 %|Gris ±0,05 %|Or ±5 %|Argent ±10 %|Aucun ±20 %";
static const char *const TOL_NAMES[9] = {"Marron", "Rouge", "Vert", "Bleu", "Violet", "Gris", "Or", "Argent", "aucun"};
static const double TOLV[9] = {1, 2, 0.5, 0.25, 0.1, 0.05, 5, 10, 20};

static const uint16_t DIGC[10] = {
    rgb(25, 25, 25), rgb(125, 70, 25), rgb(215, 30, 30), rgb(240, 130, 0), rgb(240, 205, 0),
    rgb(20, 145, 55), rgb(30, 80, 215), rgb(140, 50, 180), rgb(135, 135, 140), rgb(250, 250, 250),
};
static const uint16_t GOLD = rgb(205, 165, 45), SILVER = rgb(185, 188, 198);
static const uint16_t BODY = rgb(226, 204, 160);

static uint16_t multColor(int m) { return m == 0 ? SILVER : m == 1 ? GOLD : DIGC[m - 2]; }
static int tolColorIdx(int t) { static const int8_t d[9] = {1, 2, 5, 6, 7, 8, -1, -2, -3}; return d[t]; }
static uint16_t tolColor(int t)
{
    int d = tolColorIdx(t);
    return d >= 0 ? DIGC[d] : d == -1 ? GOLD : SILVER;
}

// Resistance dessinee en grand avec ses anneaux. sel : anneau mis en evidence (0..n-1, -1 aucun).
static void drawRes(Sch &g, const uint16_t *bands, int n, bool hasTol, int sel, const char *valueText)
{
    Canvas &cv = *g.cv;
    int ox = g.ox, oy = g.oy;
    const int x0 = 26, x1 = 154, y0 = 52, y1 = 96, ym = (y0 + y1) / 2;
    cv.fillRect(ox + 4, oy + ym - 1, x0 - 4, 3, rgb(150, 150, 155));
    cv.fillRect(ox + x1, oy + ym - 1, 175 - x1, 3, rgb(150, 150, 155));
    cv.roundRect(ox + x0, oy + y0, x1 - x0, y1 - y0, 4, BODY, true, rgb(150, 120, 80), true);
    // renflements aux extremites
    cv.roundRect(ox + x0 - 2, oy + y0 - 4, 26, y1 - y0 + 8, 4, BODY, true, rgb(150, 120, 80), true);
    cv.roundRect(ox + x1 - 24, oy + y0 - 4, 26, y1 - y0 + 8, 4, BODY, true, rgb(150, 120, 80), true);
    int nd = hasTol ? n - 1 : n;
    for (int i = 0; i < n; i++) {
        bool tol = hasTol && i == n - 1;
        int x = tol ? x1 - 18 : x0 + 8 + i * (nd >= 4 ? 16 : 19);
        int top = (i == 0 || tol) ? y0 - 3 : y0 + 1, bot = (i == 0 || tol) ? y1 + 3 : y1 - 1;
        cv.fillRect(ox + x, oy + top, 9, bot - top, bands[i]);
        if (bands[i] == DIGC[9]) cv.frame(ox + x, oy + top, 9, bot - top, rgb(170, 170, 170));
        if (i == sel) {
            cv.fillTri(ox + x + 4, oy + top - 3, ox + x - 1, oy + top - 10, ox + x + 9, oy + top - 10, g.accent);
            cv.fillTri(ox + x + 4, oy + bot + 2, ox + x - 1, oy + bot + 9, ox + x + 9, oy + bot + 9, g.accent);
        }
    }
    if (valueText) {
        int w = font::width(BOLD_16, valueText);
        font::drawText(cv, BOLD_16, ox + 90 - w / 2, oy + 132, valueText, g.ink);
    }
}

static bool inSeries(int series, double r)
{
    double n = eNearest(series, r);
    return fm::fabs(n - r) <= 1e-9 * r;
}

// ==========================================================================
// Anneaux -> valeur
// ==========================================================================
namespace c2v {
enum { OR, ORMIN, ORMAX };
enum { CN, B1, B2, B3, BM, BT };
static void def(CalcState &s) { s.c[B1] = 4; s.c[B2] = 7; s.c[B3] = 0; s.c[BM] = 4; s.c[BT] = 6; }
static void rows(const CalcState &s, RowList &r)
{
    bool five = s.c[CN] == 1;
    r.choice("#Anneaux", CN, "4 anneaux|5 anneaux", 0, "4 anneaux : 2 chiffres ; 5 anneaux : 3 chiffres");
    r.choice("#Chiffre 1", B1, COLORS, 1, "Premier anneau (le plus proche d'une extrémité)");
    r.choice("#Chiffre 2", B2, COLORS, 2, "Deuxième chiffre");
    if (five) r.choice("#Chiffre 3", B3, COLORS, 3, "Troisième chiffre");
    r.choice("#Multiplicateur", BM, MULTS, five ? 4 : 3, "Puissance de 10");
    r.choice("#Tolérance", BT, TOLS, five ? 5 : 4, "Dernier anneau, souvent séparé des autres");
    r.head("Résultats");
    r.out("R", OR, U_OHM, "Valeur nominale", true);
    r.out("R_{\\min}", ORMIN, U_OHM, "Valeur minimale (tolérance)");
    r.out("R_{\\max}", ORMAX, U_OHM, "Valeur maximale (tolérance)");
    r.msg(0);
}
static void calc(CalcState &s)
{
    bool five = s.c[CN] == 1;
    double d = five ? s.c[B1] * 100 + s.c[B2] * 10 + s.c[B3] : s.c[B1] * 10 + s.c[B2];
    double r = d * fm::p10(s.c[BM] - 2);
    double t = TOLV[s.c[BT]] / 100;
    s.v[OR] = r;
    s.v[ORMIN] = r * (1 - t);
    s.v[ORMAX] = r * (1 + t);
    if (r <= 0) setMsg(s, 0, "Valeur nulle : premier chiffre noir ?", ML_WARN);
    else if (inSeries(E12, r)) setMsg(s, 0, "Valeur de la série E12", ML_OK);
    else if (inSeries(E24, r)) setMsg(s, 0, "Valeur de la série E24", ML_OK);
    else if (inSeries(E96, r)) setMsg(s, 0, "Valeur de la série E96", ML_OK);
    else setMsg(s, 0, "Valeur hors des séries normalisées", ML_WARN);
}
static void sch(Sch &g, const CalcState &s)
{
    bool five = s.c[CN] == 1;
    uint16_t b[6];
    int n = 0;
    b[n++] = DIGC[s.c[B1]];
    b[n++] = DIGC[s.c[B2]];
    if (five) b[n++] = DIGC[s.c[B3]];
    b[n++] = multColor(s.c[BM]);
    bool hasTol = s.c[BT] != 8;
    if (hasTol) b[n++] = tolColor(s.c[BT]);
    char buf[48];
    int k = fmtValue(buf, sizeof buf, s.v[OR], U_OHM);
    k += scopy(buf + k, (int)sizeof(buf) - k, "  \xC2\xB1");
    k += fmtNumber(buf + k, (int)sizeof(buf) - k, TOLV[s.c[BT]], 3);
    scopy(buf + k, (int)sizeof(buf) - k, " %");
    int sel = g.hilite - 1;
    if (!hasTol && sel == n) sel = -1;
    drawRes(g, b, n, hasTol, sel, buf);
}
}  // namespace c2v

// ==========================================================================
// Valeur -> anneaux
// ==========================================================================
namespace v2c {
enum { R, OR };
enum { CN, BT };
static char s_txt[5][40];
static int s_d[3], s_mult, s_ok;
static void def(CalcState &s) { s.v[R] = 4.7e3; s.c[BT] = 6; }
static void rows(const CalcState &, RowList &r)
{
    r.in("R", R, U_OHM, ST_E24, 0, "Valeur à marquer");
    r.choice("#Anneaux", CN, "4 anneaux|5 anneaux", 0, "4 anneaux : 2 chiffres ; 5 anneaux : 3 chiffres");
    r.choice("#Tolérance", BT, TOLS, 0, "Anneau de tolérance");
    r.head("Code couleur");
    for (int i = 0; i < 4; i++) r.msg(i);
    r.out("R\\ (\\text{marquée})", OR, U_OHM, "Valeur réellement codée (arrondie)", true);
}
static void calc(CalcState &s)
{
    bool five = s.c[CN] == 1;
    int nd = five ? 3 : 2;
    double r = s.v[R];
    s_ok = 0;
    for (int i = 0; i < NMSG; i++) setMsg(s, i, nullptr, ML_INFO);
    if (!(r > 0)) return;
    int e = (int)fm::floor(fm::log10(r) + 1e-12) - (nd - 1);
    double m = fm::floor(r / fm::p10(e) + 0.5);
    if (m >= fm::p10(nd) - 0.5) { m = fm::floor(m / 10 + 0.5); e++; }
    int mi = (int)m;
    s_d[0] = five ? mi / 100 : mi / 10;
    s_d[1] = five ? (mi / 10) % 10 : mi % 10;
    s_d[2] = five ? mi % 10 : 0;
    s_mult = e + 2;
    s.v[OR] = m * fm::p10(e);
    if (s_mult < 0 || s_mult > 11) {
        setMsg(s, 0, "Valeur hors du code couleur", ML_BAD);
        s.v[OR] = NaN();
        return;
    }
    s_ok = 1;
    // messages (4 cases : les deux ou trois chiffres, puis multiplicateur + tolerance)
    for (int i = 0; i < nd; i++) {
        int k = scopy(s_txt[i], 40, "Anneau ");
        s_txt[i][k++] = (char)('1' + i);
        k += scopy(s_txt[i] + k, 40 - k, " : ");
        k += scopy(s_txt[i] + k, 40 - k, COLOR_NAMES[s_d[i]]);
        k += scopy(s_txt[i] + k, 40 - k, " (");
        s_txt[i][k++] = (char)('0' + s_d[i]);
        scopy(s_txt[i] + k, 40 - k, ")");
        setMsg(s, i, s_txt[i], ML_INFO);
    }
    int k = scopy(s_txt[3], 40, "Mult. : ");
    k += scopy(s_txt[3] + k, 40 - k, MULT_NAMES[s_mult]);
    k += scopy(s_txt[3] + k, 40 - k, " · tol. : ");
    scopy(s_txt[3] + k, 40 - k, TOL_NAMES[s.c[BT]]);
    setMsg(s, nd == 3 ? 3 : 2, s_txt[3], ML_INFO);
    if (nd == 2) setMsg(s, 3, fm::fabs(s.v[OR] - r) > 1e-9 * r ? "Valeur arrondie à 2 chiffres" : nullptr, ML_WARN);
}
static void sch(Sch &g, const CalcState &s)
{
    if (!s_ok) return;
    bool five = s.c[CN] == 1;
    uint16_t b[6];
    int n = 0;
    b[n++] = DIGC[s_d[0]];
    b[n++] = DIGC[s_d[1]];
    if (five) b[n++] = DIGC[s_d[2]];
    b[n++] = multColor(s_mult);
    bool hasTol = s.c[BT] != 8;
    if (hasTol) b[n++] = tolColor(s.c[BT]);
    char buf[32];
    fmtValue(buf, sizeof buf, s.v[OR], U_OHM);
    drawRes(g, b, n, hasTol, -1, buf);
}
}  // namespace v2c

// ==========================================================================
// Series E
// ==========================================================================
namespace es {
enum { X, O6, E6_, O12, E12_, O24, E24_, O48, E48_, O96, E96_ };
static void def(CalcState &s) { s.v[X] = 5.3e3; }
static void rows(const CalcState &, RowList &r)
{
    r.in("x", X, U_OHM, ST_NONE, 0, "Valeur à approcher (résistance, ou toute grandeur)");
    static const char *const lab[5] = {"#E6", "#E12", "#E24", "#E48", "#E96"};
    static const char *const desc[5] = {"Série E6 (±20 %)", "Série E12 (±10 %)", "Série E24 (±5 %)",
                                        "Série E48 (±2 %)", "Série E96 (±1 %)"};
    for (int i = 0; i < 5; i++) {
        r.head(desc[i]);
        r.out(lab[i], O6 + 2 * i, U_OHM, "Valeur normalisée la plus proche", i == 2);
        r.out("#écart", O6 + 2 * i + 1, U_PCT, "Écart relatif à x");
    }
}
static void calc(CalcState &s)
{
    static const int ser[5] = {E6, E12, E24, E48, E96};
    for (int i = 0; i < 5; i++) {
        double n = eNearest(ser[i], s.v[X]);
        s.v[O6 + 2 * i] = n;
        s.v[O6 + 2 * i + 1] = (n - s.v[X]) / s.v[X] * 100;
    }
}
static void sch(Sch &g, const CalcState &s)
{
    // regle logarithmique d'une decade
    Canvas &cv = *g.cv;
    const int x = 60, y0 = 160, y1 = 16;   // 1 en bas, 10 en haut
    auto yOf = [&](double m) { return y0 - (int)(fm::log10(m) * (y0 - y1) + 0.5); };
    cv.fillRect(g.ox + x, g.oy + y1, 2, y0 - y1 + 1, g.ink);
    for (int i = 0; i < 96; i++) cv.fillRect(g.ox + x + 2, g.oy + yOf(eMantissa(E96, i) / 100.0), 3, 1, g.label);
    for (int i = 0; i < 24; i++) cv.fillRect(g.ox + x + 2, g.oy + yOf(eMantissa(E24, i) / 100.0), 7, 1, g.ink);
    char buf[16];
    for (int i = 0; i < 6; i++) {
        double m = eMantissa(E6, i) / 100.0;
        int y = yOf(m);
        cv.fillRect(g.ox + x + 2, g.oy + y, 11, 2, g.ink);
        fmtNumber(buf, sizeof buf, m, 2);
        font::drawText(cv, SANS_12, g.ox + x + 16, g.oy + y + 4, buf, g.ink);
    }
    font::drawText(cv, SANS_12, g.ox + x + 16, g.oy + y1 + 4, "10", g.ink);
    // position de x dans sa decade
    double v = s.v[X];
    if (v > 0) {
        double l = fm::log10(v), d = fm::floor(l + 1e-12);
        double m = fm::pow10(l - d);
        int y = yOf(m);
        cv.fillTri(g.ox + x - 2, g.oy + y, g.ox + x - 12, g.oy + y - 6, g.ox + x - 12, g.oy + y + 6, col::red);
        double n24 = s.v[O24] / fm::pow10(d);
        if (n24 >= 9.9999) n24 /= 10;
        int yn = yOf(n24);
        cv.fillTri(g.ox + x + 14, g.oy + yn, g.ox + x + 4, g.oy + yn - 5, g.ox + x + 4, g.oy + yn + 5, g.accent);
        int k = scopy(buf, sizeof buf, "\xC3\x97 ");
        fmtValue(buf + k, (int)sizeof(buf) - k, fm::pow10(d), U_OHM);
        font::drawText(cv, BOLD_12, g.ox + 110, g.oy + 30, buf, g.ink);
        font::drawText(cv, SANS_12, g.ox + 110, g.oy + 60, "\xE2\x96\xB6 x", col::red);
        font::drawText(cv, SANS_12, g.ox + 110, g.oy + 76, "\xE2\x97\x80 E24", g.accent);
    }
}
}  // namespace es

// ==========================================================================
// Marquages CMS et condensateurs
// ==========================================================================
namespace cms {
enum { CODE, OV };
enum { CTYPE };
static char s_msg[48];
static void def(CalcState &s) { s.v[CODE] = 472; }
static void rows(const CalcState &s, RowList &r)
{
    bool cap = s.c[CTYPE] == 1;
    r.choice("#Composant", CTYPE, "résistance|condensateur", 0, "Marquage à 3 ou 4 chiffres");
    r.in("#Code", CODE, U_NONE, ST_INT, 0, "Chiffres significatifs puis nombre de zéros (ex. 472, 1002, 104)",
         RF_NONNEG | RF_INT);
    r.head("Résultats");
    r.out(cap ? "C" : "R", OV, cap ? U_F : U_OHM, cap ? "Code en picofarads" : "Valeur en ohms", true);
    r.msg(0);
    r.msg(1);
}
static void calc(CalcState &s)
{
    bool cap = s.c[CTYPE] == 1;
    int n = (int)(s.v[CODE] + 0.5);
    if (n < 0) n = 0;
    if (n > 99999) n = 99999;
    s.v[CODE] = n;
    double v;
    int nd = n >= 1000 ? 4 : n >= 100 ? 3 : n >= 10 ? 2 : 1;
    if (nd <= 2) v = n;
    else {
        int sig = n / 10, mul = n % 10;
        double k = mul == 8 ? 0.01 : mul == 9 ? 0.1 : fm::p10(mul);
        v = sig * k;
    }
    s.v[OV] = cap ? v * 1e-12 : v;
    int k = scopy(s_msg, sizeof s_msg, "Code ");
    k += itoaSimple(n, s_msg + k);
    if (nd >= 3) {
        k += scopy(s_msg + k, (int)sizeof(s_msg) - k, " = ");
        k += itoaSimple(n / 10, s_msg + k);
        static const char *const sup[8] = {"\xE2\x81\xB0", "\xC2\xB9", "\xC2\xB2", "\xC2\xB3",
                                            "\xE2\x81\xB4", "\xE2\x81\xB5", "\xE2\x81\xB6", "\xE2\x81\xB7"};
        int m = n % 10;
        if (m >= 8) {
            k += scopy(s_msg + k, (int)sizeof(s_msg) - k, " \xC3\xB7 ");
            k += itoaSimple(m == 8 ? 100 : 10, s_msg + k);
        } else {
            k += scopy(s_msg + k, (int)sizeof(s_msg) - k, " \xC3\x97 10");
            k += scopy(s_msg + k, (int)sizeof(s_msg) - k, sup[m]);
        }
    }
    scopy(s_msg + k, (int)sizeof(s_msg) - k, cap ? " pF" : " \xCE\xA9");
    setMsg(s, 0, s_msg, ML_INFO);
    setMsg(s, 1, cap ? "Lettre de tolérance : J ±5 %, K ±10 %, M ±20 %" : "R = virgule : 4R7 = 4,7 Ω, R10 = 0,1 Ω", ML_INFO);
}
static void sch(Sch &g, const CalcState &s)
{
    Canvas &cv = *g.cv;
    char buf[12];
    itoaSimple((int)s.v[CODE], buf);
    if (s.c[CTYPE] == 0) {
        int x0 = g.ox + 40, y0 = g.oy + 56, w = 100, h = 48;
        cv.fillRect(x0 - 10, y0, 12, h, SILVER);
        cv.fillRect(x0 + w - 2, y0, 12, h, SILVER);
        cv.fillRect(x0, y0, w, h, rgb(30, 30, 34));
        int tw = font::width(BOLD_20, buf);
        font::drawText(cv, BOLD_20, x0 + (w - tw) / 2, y0 + 32, buf, col::white);
    } else {
        int cx = g.ox + 90, cy = g.oy + 70;
        cv.fillRect(cx - 16, cy + 20, 2, 70, rgb(150, 150, 155));
        cv.fillRect(cx + 14, cy + 20, 2, 70, rgb(150, 150, 155));
        cv.disc(cx, cy, 36, rgb(214, 128, 40));
        cv.circle(cx, cy, 36, rgb(160, 90, 30));
        int tw = font::width(BOLD_20, buf);
        font::drawText(cv, BOLD_20, cx - tw / 2, cy + 8, buf, rgb(40, 25, 10));
    }
    char vb[32];
    fmtValue(vb, sizeof vb, s.v[OV], s.c[CTYPE] ? U_F : U_OHM);
    int w = font::width(BOLD_16, vb);
    font::drawText(cv, BOLD_16, g.ox + 90 - w / 2, g.oy + (s.c[CTYPE] ? 166 : 140), vb, g.ink);
}
}  // namespace cms

extern const CalcDef g_calcCode[] = {
    {"Code couleur → valeur", "#4 ou 5 anneaux", "code couleur anneaux resistance lecture", 8, G_NONE, 0,
     c2v::def, c2v::rows, c2v::calc, c2v::sch, nullptr, nullptr, nullptr, nullptr, 0},
    {"Valeur → code couleur", "#Couleurs des anneaux d'une valeur", "code couleur anneaux resistance marquage", 8, G_NONE, 0,
     v2c::def, v2c::rows, v2c::calc, v2c::sch, nullptr, nullptr, nullptr, nullptr, 0},
    {"Séries E (E6 à E96)", "#Valeurs normalisées les plus proches", "serie e12 e24 e96 normalisee valeur standard", 8, G_NONE, 0,
     es::def, es::rows, es::calc, es::sch, nullptr, nullptr, nullptr, nullptr, 0},
    {"Marquages CMS et condensateurs", "#Codes à 3 ou 4 chiffres (472, 104…)", "cms smd marquage code condensateur 104", 8, G_NONE, 0,
     cms::def, cms::rows, cms::calc, cms::sch, nullptr, nullptr, nullptr, nullptr, 0},
};
extern const int g_calcCodeN = sizeof(g_calcCode) / sizeof(g_calcCode[0]);
