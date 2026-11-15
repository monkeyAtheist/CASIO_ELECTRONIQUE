// Section : associations de resistances, condensateurs et inductances.
#include "CalcUtil.hpp"

using namespace cu;

namespace lst {
// cases 0..11 : composants ; puis resultats
enum { OSER = 20, OPAR, ONEAR, OERR, OSUM };
enum { CMODE };
enum { K_R, K_C, K_L };

static const char *const LR[12] = {"R_1", "R_2", "R_3", "R_4", "R_5", "R_6", "R_7", "R_8", "R_9", "R_{10}", "R_{11}", "R_{12}"};
static const char *const LC[12] = {"C_1", "C_2", "C_3", "C_4", "C_5", "C_6", "C_7", "C_8", "C_9", "C_{10}", "C_{11}", "C_{12}"};
static const char *const LL[12] = {"L_1", "L_2", "L_3", "L_4", "L_5", "L_6", "L_7", "L_8", "L_9", "L_{10}", "L_{11}", "L_{12}"};

static const char *const *labels(int k) { return k == K_R ? LR : k == K_C ? LC : LL; }
static uint8_t unitOf(int k) { return k == K_R ? U_OHM : k == K_C ? U_F : U_H; }

static void defR(CalcState &s) { s.n = 3; s.v[0] = 1e3; s.v[1] = 2.2e3; s.v[2] = 4.7e3; }
static void defC(CalcState &s) { s.n = 2; s.v[0] = 100e-9; s.v[1] = 47e-9; s.c[CMODE] = 1; }
static void defL(CalcState &s) { s.n = 2; s.v[0] = 10e-3; s.v[1] = 4.7e-3; }

static void rowsK(const CalcState &s, RowList &r, int k)
{
    r.choice("#Montage", CMODE, "Série|Parallèle", 0, "Association en série ou en parallèle");
    r.head("Composants");
    const char *const *lab = labels(k);
    for (int i = 0; i < s.n; i++)
        r.in(lab[i], i, unitOf(k), k == K_R ? ST_E24 : ST_E12, i + 1,
             k == K_R ? "F4 : ajouter  ·  F5 : retirer  ·  ← → : série E24" : "F4 : ajouter  ·  F5 : retirer  ·  ← → : série E12");
    r.head("Résultats");
    bool par = s.c[CMODE] == 1;
    const char *ls = k == K_R ? "R_{\\text{série}}" : k == K_C ? "C_{\\text{série}}" : "L_{\\text{série}}";
    const char *lp = k == K_R ? "R_{\\text{par.}}" : k == K_C ? "C_{\\text{par.}}" : "L_{\\text{par.}}";
    const char *ds = k == K_C ? "En série : 1/C = 1/C₁ + 1/C₂ + …" : k == K_R ? "En série : R = R₁ + R₂ + …"
                                                                 : "En série (sans couplage) : L = L₁ + L₂ + …";
    const char *dp = k == K_C ? "En parallèle : C = C₁ + C₂ + …" : k == K_R ? "En parallèle : 1/R = 1/R₁ + 1/R₂ + …"
                                                                    : "En parallèle : 1/L = 1/L₁ + 1/L₂ + …";
    r.out(par ? lp : ls, par ? OPAR : OSER, unitOf(k), par ? dp : ds, true);
    r.out(par ? ls : lp, par ? OSER : OPAR, unitOf(k), par ? ds : dp);
    r.out(k == K_R ? "#E24 proche" : "#E12 proche", ONEAR, unitOf(k), "Valeur normalisée la plus proche du résultat");
    r.out("#Écart", OERR, U_PCT, "Écart de la valeur normalisée au résultat");
}

static void calcK(CalcState &s, int k)
{
    if (s.n < 1) s.n = 1;
    if (s.n > MAX_LIST) s.n = MAX_LIST;
    double sum = 0, inv = 0;
    for (int i = 0; i < s.n; i++) {
        sum += s.v[i];
        inv += 1.0 / s.v[i];
    }
    if (k == K_C) {
        s.v[OSER] = 1.0 / inv;
        s.v[OPAR] = sum;
    } else {
        s.v[OSER] = sum;
        s.v[OPAR] = 1.0 / inv;
    }
    double eq = s.c[CMODE] ? s.v[OPAR] : s.v[OSER];
    s.v[ONEAR] = eNearest(k == K_R ? E24 : E12, eq);
    s.v[OERR] = (s.v[ONEAR] - eq) / eq * 100;
}

static void comp(Sch &g, int x0, int y0, int x1, int y1, int k, int tag)
{
    if (k == K_R) g.res(x0, y0, x1, y1, tag);
    else if (k == K_C) g.cap(x0, y0, x1, y1, tag);
    else g.ind(x0, y0, x1, y1, tag);
}

static void schK(Sch &g, const CalcState &s, int k)
{
    const char *const *lab = labels(k);
    int n = s.n;
    if (s.c[CMODE] == 0) {
        // serie : lignes de 3 composants en serpentin
        int rowsN = (n + 2) / 3;
        int y0 = 86 - 18 * (rowsN - 1);
        int xa = 22, seg = 45;
        g.term(10, y0);
        g.tex(8, y0 - 6, "#A", tex::LEFT);
        g.wire(10, y0, xa, y0);
        int endX = xa, endY = y0;
        for (int i = 0; i < n; i++) {
            int row = i / 3, j = i % 3, y = y0 + 36 * row;
            bool ltr = (row & 1) == 0;
            int a = ltr ? xa + seg * j : xa + seg * (3 - j);
            int b = ltr ? a + seg : a - seg;
            comp(g, a, y, b, y, k, i + 1);
            int lx = (a + b) / 2;
            if (k == K_L) g.texSmall(lx, y - 7, lab[i], tex::CENTER, i + 1);
            else g.texSmall(lx, y - 8, lab[i], tex::CENTER, i + 1);
            endX = b;
            endY = y;
            if (j == 2 && i + 1 < n) {
                g.wire(b, y, b, y + 36);
                g.dot(b, y);
            }
        }
        bool lastLtr = (((n - 1) / 3) & 1) == 0;
        int tx = lastLtr ? 168 : 10;
        g.wire(endX, endY, tx, endY);
        g.term(tx, endY);
        g.tex(lastLtr ? 170 : 8, endY - 6, "#B", lastLtr ? tex::RIGHT : tex::LEFT);
    } else {
        // parallele : composants verticaux entre deux rails
        const int yt = 52, yb = 128;
        int sp = 150 / n;
        if (sp > 40) sp = 40;
        int xc = 98;
        int xfirst = xc - sp * (n - 1) / 2, xlast = xfirst + sp * (n - 1);
        g.wire(10, yt, xlast, yt);
        g.wire(10, yb, xlast, yb);
        g.term(10, yt);
        g.term(10, yb);
        g.tex(8, yt - 6, "#A", tex::LEFT);
        g.tex(8, yb + 16, "#B", tex::LEFT);
        for (int i = 0; i < n; i++) {
            int x = xfirst + sp * i;
            comp(g, x, yt, x, yb, k, i + 1);
            g.dot(x, yt);
            g.dot(x, yb);
            if (sp >= 26 || g.on(i + 1)) g.texSmall(x, yb + 18, lab[i], tex::CENTER, i + 1);
        }
    }
}

static void rowsR(const CalcState &s, RowList &r) { rowsK(s, r, K_R); }
static void rowsC(const CalcState &s, RowList &r) { rowsK(s, r, K_C); }
static void rowsL(const CalcState &s, RowList &r) { rowsK(s, r, K_L); }
static void calcR(CalcState &s) { calcK(s, K_R); }
static void calcC(CalcState &s) { calcK(s, K_C); }
static void calcL(CalcState &s) { calcK(s, K_L); }
static void schR(Sch &g, const CalcState &s) { schK(g, s, K_R); }
static void schC(Sch &g, const CalcState &s) { schK(g, s, K_C); }
static void schL(Sch &g, const CalcState &s) { schK(g, s, K_L); }
}  // namespace lst

// ==========================================================================
// Resistance a associer pour obtenir une valeur cible
// ==========================================================================
namespace cible {
enum { R1, RT, ORX, ONEAR, OGOT, OERR };
enum { CMODE };
enum { T_R1 = 1, T_RX };
static void def(CalcState &s) { s.v[R1] = 10e3; s.v[RT] = 6.8e3; s.c[CMODE] = 1; }
static void rows(const CalcState &, RowList &r)
{
    r.choice("#Montage", CMODE, "Série|Parallèle", T_RX, "Rx ajoutée en série ou en parallèle sur R₁");
    r.in("R_1", R1, U_OHM, ST_E24, T_R1, "Résistance déjà en place");
    r.in("R_{\\text{cible}}", RT, U_OHM, ST_E24, 0, "Valeur à obtenir");
    r.head("Résultats");
    r.out("R_x", ORX, U_OHM, "Valeur exacte à associer à R₁", true, T_RX);
    r.out("#E24 proche", ONEAR, U_OHM, "Valeur normalisée E24 la plus proche de Rx", false, T_RX);
    r.out("R_{\\text{obtenue}}", OGOT, U_OHM, "Résultat avec la valeur E24");
    r.out("#Écart", OERR, U_PCT, "Écart à la valeur cible");
    r.msg(0);
}
static void calc(CalcState &s)
{
    double r1 = s.v[R1], rt = s.v[RT], rx;
    bool par = s.c[CMODE] == 1;
    if (par) rx = rt < r1 ? r1 * rt / (r1 - rt) : NaN();
    else rx = rt > r1 ? rt - r1 : NaN();
    s.v[ORX] = rx;
    if (ok(rx)) {
        double n = eNearest(E24, rx);
        s.v[ONEAR] = n;
        s.v[OGOT] = par ? par2(r1, n) : r1 + n;
        s.v[OERR] = (s.v[OGOT] - rt) / rt * 100;
        setMsg(s, 0, par ? "Rx = R₁ R / (R₁ − R)" : "Rx = R − R₁", ML_OK);
    } else {
        s.v[ONEAR] = s.v[OGOT] = s.v[OERR] = NaN();
        setMsg(s, 0, par ? "Impossible : il faut R < R₁ en parallèle" : "Impossible : il faut R > R₁ en série", ML_BAD);
    }
}
static void sch(Sch &g, const CalcState &s)
{
    if (s.c[CMODE]) {
        g.term(12, 86);
        g.wire(12, 86, 44, 86);
        g.wire(44, 56, 44, 116);
        g.wire(140, 56, 140, 116);
        g.wire(140, 86, 166, 86);
        g.term(166, 86);
        g.dot(44, 86);
        g.dot(140, 86);
        g.res(44, 56, 140, 56, T_R1);
        g.res(44, 116, 140, 116, T_RX);
        g.tex(92, 46, "R_1", tex::CENTER, T_R1);
        g.tex(92, 140, "R_x", tex::CENTER, T_RX);
    } else {
        g.term(12, 86);
        g.res(12, 86, 88, 86, T_R1);
        g.res(88, 86, 166, 86, T_RX);
        g.dot(88, 86);
        g.term(166, 86);
        g.tex(50, 74, "R_1", tex::CENTER, T_R1);
        g.tex(127, 74, "R_x", tex::CENTER, T_RX);
    }
}
}  // namespace cible

extern const CalcDef g_calcAssoc[] = {
    {"Résistances", "#Série ou parallèle, jusqu'à 12 résistances", "resistance serie parallele association equivalente", 0, G_NONE, CF_LIST,
     lst::defR, lst::rowsR, lst::calcR, lst::schR, nullptr, nullptr, nullptr, nullptr, 0},
    {"Condensateurs", "#Série ou parallèle, jusqu'à 12 condensateurs", "condensateur capacite serie parallele association", 0, G_NONE, CF_LIST,
     lst::defC, lst::rowsC, lst::calcC, lst::schC, nullptr, nullptr, nullptr, nullptr, 0},
    {"Inductances", "#Série ou parallèle, sans couplage", "inductance bobine serie parallele association", 0, G_NONE, CF_LIST,
     lst::defL, lst::rowsL, lst::calcL, lst::schL, nullptr, nullptr, nullptr, nullptr, 0},
    {"Résistance à associer", "#Trouver Rx pour obtenir une valeur cible", "cible ajouter resistance valeur obtenir", 0, G_NONE, 0,
     cible::def, cible::rows, cible::calc, cible::sch, nullptr, nullptr, nullptr, nullptr, 0},
};
extern const int g_calcAssocN = sizeof(g_calcAssoc) / sizeof(g_calcAssoc[0]);
