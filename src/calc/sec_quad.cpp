// Section : quadripoles (matrices impedance, admittance, hybrides, de
// transfert et de repartition ; cellules en T, en Pi et en L ; attenuateurs).
//
// Convention recepteur aux deux acces : I1 entre par l'acces 1, I2 par
// l'acces 2. Matrice de transfert T (ABCD) : (V1, I1) = T (V2, -I2).
// Toutes les conversions passent par la relation lineaire M x = 0 sur
// x = (V1, I1, V2, I2) : on isole les variables dependantes du type voulu.
#include "CalcUtil.hpp"

using namespace cu;

namespace qp {

enum PT { PT_Z, PT_Y, PT_H, PT_G, PT_T, PT_S, PT_COUNT };
static const char *const PT_CHOICES = "Z|Y|H|G|T (ABCD)|S";

struct M2 {
    Cx m[2][2];
};
struct Rel {
    Cx r[2][4];
};

struct TypeDesc {
    int dep[2], ind[2], sgn[2];
    bool wave;
};
static const TypeDesc TD[PT_COUNT] = {
    {{0, 2}, {1, 3}, {1, 1}, false},    // Z : (V1, V2) = Z (I1, I2)
    {{1, 3}, {0, 2}, {1, 1}, false},    // Y : (I1, I2) = Y (V1, V2)
    {{0, 3}, {1, 2}, {1, 1}, false},    // H : (V1, I2) = H (I1, V2)
    {{1, 2}, {0, 3}, {1, 1}, false},    // G : (I1, V2) = G (V1, I2)
    {{0, 1}, {2, 3}, {1, -1}, false},   // T : (V1, I1) = T (V2, -I2)
    {{1, 3}, {0, 2}, {1, 1}, true},     // S : (b1, b2) = S (a1, a2) ; y = (a1, b1, a2, b2)
};

static const char *const PNAME[PT_COUNT][4] = {
    {"Z_{11}", "Z_{12}", "Z_{21}", "Z_{22}"}, {"Y_{11}", "Y_{12}", "Y_{21}", "Y_{22}"},
    {"h_{11}", "h_{12}", "h_{21}", "h_{22}"}, {"g_{11}", "g_{12}", "g_{21}", "g_{22}"},
    {"A", "B", "C", "D"}, {"S_{11}", "S_{12}", "S_{21}", "S_{22}"},
};
#define RI(a) "\\text{Re}\\," a, "\\text{Im}\\," a
static const char *const PRI[PT_COUNT][8] = {
    {RI("Z_{11}"), RI("Z_{12}"), RI("Z_{21}"), RI("Z_{22}")},
    {RI("Y_{11}"), RI("Y_{12}"), RI("Y_{21}"), RI("Y_{22}")},
    {RI("h_{11}"), RI("h_{12}"), RI("h_{21}"), RI("h_{22}")},
    {RI("g_{11}"), RI("g_{12}"), RI("g_{21}"), RI("g_{22}")},
    {RI("A"), RI("B"), RI("C"), RI("D")},
    {RI("S_{11}"), RI("S_{12}"), RI("S_{21}"), RI("S_{22}")},
};
#undef RI
static const uint8_t PUNIT[PT_COUNT][4] = {
    {U_OHM, U_OHM, U_OHM, U_OHM}, {U_SIE, U_SIE, U_SIE, U_SIE}, {U_OHM, U_NONE, U_NONE, U_SIE},
    {U_SIE, U_NONE, U_NONE, U_OHM}, {U_NONE, U_OHM, U_SIE, U_NONE}, {U_NONE, U_NONE, U_NONE, U_NONE},
};
// Relation de definition (dessinee sous le quadripole).
static const char *const PREL[PT_COUNT] = {
    "\\begin{pmatrix}V_1\\\\V_2\\end{pmatrix}=Z\\begin{pmatrix}I_1\\\\I_2\\end{pmatrix}",
    "\\begin{pmatrix}I_1\\\\I_2\\end{pmatrix}=Y\\begin{pmatrix}V_1\\\\V_2\\end{pmatrix}",
    "\\begin{pmatrix}V_1\\\\I_2\\end{pmatrix}=H\\begin{pmatrix}I_1\\\\V_2\\end{pmatrix}",
    "\\begin{pmatrix}I_1\\\\V_2\\end{pmatrix}=G\\begin{pmatrix}V_1\\\\I_2\\end{pmatrix}",
    "\\begin{pmatrix}V_1\\\\I_1\\end{pmatrix}=T\\begin{pmatrix}V_2\\\\-I_2\\end{pmatrix}",
    "\\begin{pmatrix}b_1\\\\b_2\\end{pmatrix}=S\\begin{pmatrix}a_1\\\\a_2\\end{pmatrix}",
};

// x = K y (ondes -> tensions/courants), y = Kinv x.
static void waveK(double z0, Cx K[4][4], Cx Ki[4][4])
{
    double r = fm::sqrt(z0);
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) { K[i][j] = cx(0); Ki[i][j] = cx(0); }
    for (int p = 0; p < 2; p++) {
        int v = 2 * p, c = 2 * p + 1, a = 2 * p, b = 2 * p + 1;
        K[v][a] = cx(r); K[v][b] = cx(r);
        K[c][a] = cx(1 / r); K[c][b] = cx(-1 / r);
        Ki[a][v] = cx(1 / (2 * r)); Ki[a][c] = cx(z0 / (2 * r));
        Ki[b][v] = cx(1 / (2 * r)); Ki[b][c] = cx(-z0 / (2 * r));
    }
}

static Rel relOf(int t, const M2 &P, double z0)
{
    const TypeDesc &d = TD[t];
    Rel ry;
    for (int r = 0; r < 2; r++) {
        for (int j = 0; j < 4; j++) ry.r[r][j] = cx(0);
        ry.r[r][d.dep[r]] = cx(1);
        for (int c = 0; c < 2; c++) ry.r[r][d.ind[c]] = -(P.m[r][c] * (double)d.sgn[c]);
    }
    if (!d.wave) return ry;
    Cx K[4][4], Ki[4][4];
    waveK(z0, K, Ki);
    Rel rx;
    for (int r = 0; r < 2; r++)
        for (int j = 0; j < 4; j++) {
            Cx acc = cx(0);
            for (int i = 0; i < 4; i++) acc = acc + ry.r[r][i] * Ki[i][j];
            rx.r[r][j] = acc;
        }
    return rx;
}

static bool fromRel(const Rel &R, int t, double z0, M2 &P)
{
    const TypeDesc &d = TD[t];
    Rel ry = R;
    if (d.wave) {
        Cx K[4][4], Ki[4][4];
        waveK(z0, K, Ki);
        for (int r = 0; r < 2; r++)
            for (int j = 0; j < 4; j++) {
                Cx acc = cx(0);
                for (int i = 0; i < 4; i++) acc = acc + R.r[r][i] * K[i][j];
                ry.r[r][j] = acc;
            }
    }
    Cx a = ry.r[0][d.dep[0]], b = ry.r[0][d.dep[1]], c = ry.r[1][d.dep[0]], e = ry.r[1][d.dep[1]];
    Cx det = a * e - b * c;
    double scale = cabs(a * e) + cabs(b * c);
    if (!(cabs(det) > 1e-12 * scale) || cbad(det)) return false;
    // inverse de Md
    Cx i00 = e / det, i01 = -b / det, i10 = -c / det, i11 = a / det;
    for (int r = 0; r < 2; r++)
        for (int k = 0; k < 2; k++) {
            Cx m0 = ry.r[0][d.ind[k]], m1 = ry.r[1][d.ind[k]];
            Cx x = (r == 0 ? i00 * m0 + i01 * m1 : i10 * m0 + i11 * m1);
            P.m[r][k] = -(x * (double)d.sgn[k]);
        }
    for (int r = 0; r < 2; r++)
        for (int k = 0; k < 2; k++)
            if (cbad(P.m[r][k])) return false;
    return true;
}

// Systeme 4x4 : relation (2 lignes) + 2 equations supplementaires.
static bool solve4(const Rel &R, const Cx e2[5], const Cx e3[5], Cx x[4])
{
    Cx A[4][5];
    for (int r = 0; r < 2; r++) {
        for (int j = 0; j < 4; j++) A[r][j] = R.r[r][j];
        A[r][4] = cx(0);
    }
    for (int j = 0; j < 5; j++) { A[2][j] = e2[j]; A[3][j] = e3[j]; }
    for (int c = 0; c < 4; c++) {
        int p = c;
        double best = 0;
        for (int i = c; i < 4; i++) {
            double m = fm::fabs(A[i][c].re) + fm::fabs(A[i][c].im);
            if (m > best) { best = m; p = i; }
        }
        if (!(best > 0)) return false;
        if (p != c) for (int j = 0; j < 5; j++) { Cx t = A[c][j]; A[c][j] = A[p][j]; A[p][j] = t; }
        for (int i = c + 1; i < 4; i++) {
            Cx f = A[i][c] / A[c][c];
            for (int j = c; j < 5; j++) A[i][j] = A[i][j] - f * A[c][j];
        }
    }
    for (int i = 3; i >= 0; i--) {
        Cx acc = A[i][4];
        for (int j = i + 1; j < 4; j++) acc = acc - A[i][j] * x[j];
        x[i] = acc / A[i][i];
    }
    for (int i = 0; i < 4; i++) if (cbad(x[i])) return false;
    return true;
}

struct Loaded {
    Cx ze, zs, av, avg, ai;
};
static void loaded(const Rel &R, double rg, double rl, Loaded &L)
{
    Cx bad = cx(fm::nan(), fm::nan());
    L.ze = L.zs = L.av = L.avg = L.ai = bad;
    Cx x[4];
    const Cx v1one[5] = {cx(1), cx(0), cx(0), cx(0), cx(1)};
    const Cx load[5] = {cx(0), cx(0), cx(1), cx(rl), cx(0)};
    if (solve4(R, v1one, load, x)) {
        L.av = x[2];
        L.ze = cx(1) / x[1];
        L.ai = -x[3] / x[1];
    }
    const Cx gen[5] = {cx(1), cx(rg), cx(0), cx(0), cx(1)};
    if (solve4(R, gen, load, x)) L.avg = x[2];
    const Cx genOff[5] = {cx(1), cx(rg), cx(0), cx(0), cx(0)};
    const Cx inj[5] = {cx(0), cx(0), cx(0), cx(1), cx(1)};
    if (solve4(R, genOff, inj, x)) L.zs = x[2];
}

static Cx csqrt(Cx z)
{
    double m = cabs(z);
    double re = fm::sqrt((m + z.re) / 2), im = fm::sqrt(fm::fabs(m - z.re) / 2);
    return cx(re, z.im < 0 ? -im : im);
}

static void putC(CalcState &s, int slot, Cx z)
{
    s.v[slot] = z.re;
    s.v[slot + 1] = z.im;
}

// Reciprocite et symetrie (via Z, sinon Y, sinon H).
static void props(CalcState &s, int k1, int k2, const Rel &R, double z0)
{
    M2 P;
    int t = -1;
    if (fromRel(R, PT_Z, z0, P)) t = PT_Z;
    else if (fromRel(R, PT_Y, z0, P)) t = PT_Y;
    else if (fromRel(R, PT_H, z0, P)) t = PT_H;
    if (t < 0) { setMsg(s, k1, "", ML_INFO); setMsg(s, k2, "", ML_INFO); return; }
    double sc = cabs(P.m[0][1]) + cabs(P.m[1][0]) + 1e-300;
    bool rec = t == PT_H ? cabs(P.m[0][1] + P.m[1][0]) <= 1e-9 * sc : cabs(P.m[0][1] - P.m[1][0]) <= 1e-9 * sc;
    if (rec) setMsg(s, k1, t == PT_Z ? "Réciproque : Z_12 = Z_21 (passif)" : t == PT_Y ? "Réciproque : Y_12 = Y_21 (passif)" : "Réciproque : h_12 = −h_21", ML_OK);
    else setMsg(s, k1, "Non réciproque (quadripôle actif ou unilatéral)", ML_INFO);
    if (rec && t != PT_H) {
        double sd = cabs(P.m[0][0]) + cabs(P.m[1][1]) + 1e-300;
        bool sym = cabs(P.m[0][0] - P.m[1][1]) <= 1e-9 * sd;
        setMsg(s, k2, sym ? "Symétrique : les deux accès sont interchangeables" : "Non symétrique", sym ? ML_OK : ML_INFO);
    } else setMsg(s, k2, "", ML_INFO);
}

// Boite du quadripole avec generateur et charge.
static void drawPorts(Sch &g, int xa, int xb, int yt, int yb, const char *title, int tagBox, int tagG, int tagL)
{
    g.chip(xa, yt - 6, xb - xa, yb - yt + 12, title, tagBox);
    // acces 1
    g.wire(xa - 26, yt, xa, yt);
    g.wire(xa - 26, yb, xa, yb);
    g.term(xa - 8, yt);
    g.term(xa - 8, yb);
    g.varrow(xa - 8, yb - 4, yt + 5, "V_1", -1, 0);
    g.iarrow(xa - 14, yt, 1, 0, "I_1", xa - 26, yt - 4);
    // acces 2
    g.wire(xb, yt, xb + 26, yt);
    g.wire(xb, yb, xb + 26, yb);
    g.term(xb + 8, yt);
    g.term(xb + 8, yb);
    g.varrow(xb + 8, yb - 4, yt + 5, "V_2", 1, 0);
    g.iarrow(xb + 14, yt, -1, 0, "I_2", xb + 14, yt - 4);
    // generateur et charge
    int xg = xa - 26, xl = xb + 40;
    g.wire(xg, yt, xg - 14, yt, tagG);
    g.res(xg - 14, yt, xg - 14, (yt + yb) / 2, tagG);
    g.vsrc(xg - 14, (yt + yb) / 2, yb, 1, tagG);
    g.wire(xg - 14, yb, xg, yb, tagG);
    g.wire(xb + 26, yt, xl, yt);
    g.wire(xb + 26, yb, xl, yb);
    g.res(xl, yt, xl, yb, tagL);
}

}  // namespace qp

using namespace qp;

// ==========================================================================
// Conversion des matrices
// ==========================================================================
namespace conv {
enum { P0 = 0, Z0 = 8, RG, RL, OP = 12, ODET = 20, OZE = 22, OZS = 24, OAV = 26, OAVG = 28, OAI = 30 };
enum { CSRC, CCPLX, CDST, CPOL };
enum { T_BOX = 1, T_G, T_L };
static void def(CalcState &s)
{
    s.v[P0] = 150; s.v[P0 + 2] = 100; s.v[P0 + 4] = 100; s.v[P0 + 6] = 150;
    s.v[Z0] = 50; s.v[RG] = 50; s.v[RL] = 100;
    s.c[CDST] = PT_Y;
}
static bool needZ0(const CalcState &s) { return s.c[CSRC] == PT_S || s.c[CDST] == PT_S; }
static void rows(const CalcState &s, RowList &r)
{
    int ts = s.c[CSRC], td = s.c[CDST];
    bool cplx = s.c[CCPLX] == 1, pol = s.c[CPOL] == 1;
    r.choice("#Données", CSRC, PT_CHOICES, T_BOX, "Matrice connue");
    r.choice("#Valeurs", CCPLX, "réelles|complexes", T_BOX, "Paramètres réels (résistifs) ou complexes (à une fréquence)");
    for (int k = 0; k < 4; k++) {
        r.in(cplx ? PRI[ts][2 * k] : PNAME[ts][k], P0 + 2 * k, PUNIT[ts][k], ST_NONE, T_BOX,
             cplx ? "Partie réelle" : "Paramètre de la matrice connue", 0);
        if (cplx) r.in(PRI[ts][2 * k + 1], P0 + 2 * k + 1, PUNIT[ts][k], ST_NONE, T_BOX, "Partie imaginaire", 0);
    }
    if (needZ0(s)) r.in("Z_0", Z0, U_OHM, ST_E24, 0, "Impédance de référence des paramètres S");
    r.head("Conversion");
    r.choice("#Afficher", CDST, PT_CHOICES, T_BOX, "Matrice à calculer");
    r.choice("#Notation", CPOL, "a + jb|module ∠ argument", 0, "Affichage des complexes");
    for (int k = 0; k < 4; k++) r.outC(PNAME[td][k], OP + 2 * k, PUNIT[td][k], pol, nullptr, true, T_BOX);
    if (td == PT_T) r.outC("AD-BC", ODET, U_NONE, pol, "Déterminant de T : 1 si le quadripôle est réciproque");
    r.msg(0);
    r.msg(1);
    r.msg(2);
    r.head("Quadripôle chargé");
    r.in("R_g", RG, U_OHM, ST_E24, T_G, "Résistance interne du générateur", RF_NONNEG);
    r.in("R_L", RL, U_OHM, ST_E24, T_L, "Charge sur l'accès 2");
    r.outC("Z_e", OZE, U_OHM, pol, "Impédance d'entrée V₁/I₁ (charge R_L)", true, T_L);
    r.outC("Z_s", OZS, U_OHM, pol, "Impédance de sortie vue de la charge (générateur éteint)", true, T_G);
    r.outC("A_v", OAV, U_NONE, pol, "Gain en tension V₂/V₁");
    r.outC("A_{vg}", OAVG, U_NONE, pol, "Gain depuis le générateur V₂/E_g");
    r.outC("A_i", OAI, U_NONE, pol, "Gain en courant −I₂/I₁ (courant dans la charge / courant d'entrée)");
}
static void calc(CalcState &s)
{
    int ts = s.c[CSRC], td = s.c[CDST];
    bool cplx = s.c[CCPLX] == 1;
    M2 P;
    for (int k = 0; k < 4; k++) P.m[k / 2][k % 2] = cx(s.v[P0 + 2 * k], cplx ? s.v[P0 + 2 * k + 1] : 0);
    double z0 = s.v[Z0];
    Rel R = relOf(ts, P, z0);
    M2 Q;
    if (fromRel(R, td, z0, Q)) {
        for (int k = 0; k < 4; k++) putC(s, OP + 2 * k, Q.m[k / 2][k % 2]);
        putC(s, ODET, Q.m[0][0] * Q.m[1][1] - Q.m[0][1] * Q.m[1][0]);
        setMsg(s, 0, "Conversion possible", ML_OK);
    } else {
        for (int k = 0; k < 10; k++) s.v[OP + k] = NaN();
        static char buf[48];
        int n = scopy(buf, sizeof buf, "La matrice ");
        n += scopy(buf + n, (int)sizeof(buf) - n, td == PT_T ? "T" : td == PT_S ? "S" : td == PT_Z ? "Z" : td == PT_Y ? "Y" : td == PT_H ? "H" : "G");
        scopy(buf + n, (int)sizeof(buf) - n, " n'existe pas (singulière)");
        setMsg(s, 0, buf, ML_BAD);
    }
    props(s, 1, 2, R, z0);
    Loaded L;
    loaded(R, s.v[RG], s.v[RL], L);
    putC(s, OZE, L.ze); putC(s, OZS, L.zs); putC(s, OAV, L.av); putC(s, OAVG, L.avg); putC(s, OAI, L.ai);
}
static void sch(Sch &g, const CalcState &s)
{
    static const char *const box[PT_COUNT] = {"[Z]", "[Y]", "[H]", "[G]", "[T]", "[S]"};
    drawPorts(g, 72, 112, 40, 92, box[s.c[CDST]], T_BOX, T_G, T_L);
    g.texSmall(24, 58, "R_g", tex::RIGHT, T_G);
    g.tex(152, 110, "R_L", tex::CENTER, T_L);
    tex::draw(*g.cv, PREL[s.c[CDST]], g.ox + 89, g.oy + 140, g.ink, tex::CENTER, 1);
}
}  // namespace conv

// ==========================================================================
// Cellules en T, en Pi et en L
// ==========================================================================
namespace cell {
enum { Z1R, Z1C, Z1L, Z2R, Z2C, Z2L, Z3R, Z3C, Z3L, F, RG, RL, Z0,
       OP = 14, OZE = 22, OZS = 24, OAV = 26, OAVG = 28, OZI1 = 30, OZI2 = 32 };
enum { CZ1, CZ2, CZ3, CTOPO, CDST, CPOL };
enum { T_Z1 = 1, T_Z2, T_Z3, T_G, T_L };
static void def(CalcState &s)
{
    s.c[CZ1] = ZT_R; s.c[CZ2] = ZT_C; s.c[CZ3] = ZT_R;
    s.v[Z1R] = 1e3; s.v[Z1C] = 100e-9; s.v[Z1L] = 10e-3;
    s.v[Z2R] = 1e3; s.v[Z2C] = 100e-9; s.v[Z2L] = 10e-3;
    s.v[Z3R] = 1e3; s.v[Z3C] = 100e-9; s.v[Z3L] = 10e-3;
    s.v[F] = 1e3; s.v[RG] = 50; s.v[RL] = 10e3; s.v[Z0] = 50;
    s.c[CDST] = PT_Z;
}
static Cx zk(const CalcState &s, int k, double w) { return zOf(s, CZ1 + k, Z1R + 3 * k, Z1C + 3 * k, Z1L + 3 * k, w); }
// Matrice de transfert T de la cellule.
static bool tOf(const CalcState &s, double w, M2 &T)
{
    Cx z1 = zk(s, 0, w), z2 = zk(s, 1, w), z3 = zk(s, 2, w);
    Cx one = cx(1), zero = cx(0);
    auto mul = [](const M2 &a, const M2 &b) {
        M2 r;
        for (int i = 0; i < 2; i++)
            for (int j = 0; j < 2; j++) r.m[i][j] = a.m[i][0] * b.m[0][j] + a.m[i][1] * b.m[1][j];
        return r;
    };
    M2 ser1{{{one, z1}, {zero, one}}}, sh2{{{one, zero}, {one / z2, one}}}, ser3{{{one, z3}, {zero, one}}};
    M2 sh1{{{one, zero}, {one / z1, one}}}, ser2{{{one, z2}, {zero, one}}}, sh3{{{one, zero}, {one / z3, one}}};
    switch (s.c[CTOPO]) {
    case 0: T = mul(mul(ser1, sh2), ser3); break;    // T
    case 1: T = mul(mul(sh1, ser2), sh3); break;     // Pi
    case 2: T = mul(ser1, sh2); break;               // L : Z1 serie, Z2 parallele
    default: T = mul(sh1, ser2); break;              // L inverse : Z1 parallele, Z2 serie
    }
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 2; j++)
            if (cbad(T.m[i][j])) return false;
    return true;
}
static bool hasZ3(const CalcState &s) { return s.c[CTOPO] < 2; }
static void rows(const CalcState &s, RowList &r)
{
    bool pol = s.c[CPOL] == 1;
    int td = s.c[CDST];
    r.choice("#Cellule", CTOPO, "en T|en Π|en L|en L inversé", 0, "Disposition des impédances");
    zRows(r, s, CZ1, Z1R, Z1C, Z1L, "Z_1", "R_1", "C_1", "L_1", T_Z1);
    zRows(r, s, CZ2, Z2R, Z2C, Z2L, "Z_2", "R_2", "C_2", "L_2", T_Z2);
    if (hasZ3(s)) zRows(r, s, CZ3, Z3R, Z3C, Z3L, "Z_3", "R_3", "C_3", "L_3", T_Z3);
    r.in("f", F, U_HZ, ST_125, 0, "Fréquence de calcul");
    r.head("Matrice");
    r.choice("#Afficher", CDST, PT_CHOICES, 0, "Matrice à calculer");
    r.choice("#Notation", CPOL, "a + jb|module ∠ argument", 0, "Affichage des complexes");
    if (td == PT_S) r.in("Z_0", Z0, U_OHM, ST_E24, 0, "Impédance de référence des paramètres S");
    for (int k = 0; k < 4; k++) r.outC(PNAME[td][k], OP + 2 * k, PUNIT[td][k], pol, nullptr, true);
    r.msg(0);
    r.msg(1);
    r.head("Chargée par R_L");
    r.in("R_g", RG, U_OHM, ST_E24, T_G, "Résistance interne du générateur", RF_NONNEG);
    r.in("R_L", RL, U_OHM, ST_E24, T_L, "Charge");
    r.outC("Z_e", OZE, U_OHM, pol, "Impédance d'entrée (charge R_L)", true);
    r.outC("Z_s", OZS, U_OHM, pol, "Impédance de sortie (générateur éteint)");
    r.outC("A_v", OAV, U_NONE, pol, "V₂/V₁ (F4 : Bode de A_vg)", true);
    r.outC("A_{vg}", OAVG, U_NONE, pol, "V₂/E_g");
    r.outC("Z_{i1}", OZI1, U_OHM, pol, "Impédance image d'entrée √(AB/CD)");
    r.outC("Z_{i2}", OZI2, U_OHM, pol, "Impédance image de sortie √(DB/AC)");
}
static void calc(CalcState &s)
{
    double w = w2pi(s.v[F]);
    M2 T;
    int td = s.c[CDST];
    for (int k = OP; k < OZI2 + 2; k++) s.v[k] = NaN();
    if (!tOf(s, w, T)) {
        setMsg(s, 0, "Impédance nulle en parallèle : cellule dégénérée", ML_BAD);
        setMsg(s, 1, "", ML_INFO);
        return;
    }
    Rel R = relOf(PT_T, T, s.v[Z0]);
    M2 Q;
    if (fromRel(R, td, s.v[Z0], Q)) {
        for (int k = 0; k < 4; k++) putC(s, OP + 2 * k, Q.m[k / 2][k % 2]);
        setMsg(s, 0, "Cellule passive : réciproque", ML_OK);
    } else {
        setMsg(s, 0, "Cette matrice n'existe pas pour cette cellule", ML_BAD);
    }
    bool sym = cabs(T.m[0][0] - T.m[1][1]) <= 1e-9 * (cabs(T.m[0][0]) + cabs(T.m[1][1]));
    setMsg(s, 1, sym ? "Symétrique (A = D)" : "Non symétrique (A ≠ D)", ML_INFO);
    Loaded L;
    loaded(R, s.v[RG], s.v[RL], L);
    putC(s, OZE, L.ze); putC(s, OZS, L.zs); putC(s, OAV, L.av); putC(s, OAVG, L.avg);
    Cx A = T.m[0][0], B = T.m[0][1], C = T.m[1][0], D = T.m[1][1];
    putC(s, OZI1, csqrt((A * B) / (C * D)));
    putC(s, OZI2, csqrt((D * B) / (C * A)));
}
static bool tf(const CalcState &s, double f, Cx &H)
{
    M2 T;
    if (!tOf(s, w2pi(f), T)) return false;
    double rl = s.v[RL], rg = s.v[RG];
    H = cx(rl) / (T.m[0][0] * rl + T.m[0][1] + rg * (T.m[1][0] * rl + T.m[1][1]));
    return !cbad(H);
}
static double fc(const CalcState &s)
{
    double R[3], C[3], L[3];
    int nr = 0, nc = 0, nl = 0;
    for (int k = 0; k < (hasZ3(s) ? 3 : 2); k++) {
        int t = s.c[CZ1 + k];
        if (zHasR(t)) R[nr++] = s.v[Z1R + 3 * k];
        if (zHasC(t)) C[nc++] = s.v[Z1C + 3 * k];
        if (zHasL(t)) L[nl++] = s.v[Z1L + 3 * k];
    }
    if (!nr) R[nr++] = s.v[RL];
    return fCorner(R, nr, C, nc, L, nl);
}
static void sch(Sch &g, const CalcState &s)
{
    const int yt = 52, yb = 122, xa = 34, xb = 144;
    int t = s.c[CTOPO];
    g.wire(xa - 18, yt, xa, yt);
    g.wire(xa - 18, yb, xb + 18, yb);
    g.term(xa - 18, yt);
    g.term(xa - 18, yb);
    g.varrow(xa - 18, yb - 4, yt + 5, "V_1", 1);
    g.term(xb + 18, yt);
    g.term(xb + 18, yb);
    g.varrow(xb + 18, yb - 4, yt + 5, "V_2", -1);
    auto zh = [&](int k, int x0, int x1) {
        g.zdip(x0, yt, x1, yt, s.c[CZ1 + k], T_Z1 + k);
        static const char *const lab[3] = {"Z_1", "Z_2", "Z_3"};
        g.tex((x0 + x1) / 2, yt - 12, lab[k], tex::CENTER, T_Z1 + k);
    };
    auto zv = [&](int k, int x) {
        g.zdip(x, yt, x, yb, s.c[CZ1 + k], T_Z1 + k);
        static const char *const lab[3] = {"Z_1", "Z_2", "Z_3"};
        g.tex(x + 12, (yt + yb) / 2 + 5, lab[k], tex::LEFT, T_Z1 + k);
        g.dot(x, yt);
        g.dot(x, yb);
    };
    switch (t) {
    case 0:
        zh(0, xa, 82);
        zv(1, 89);
        g.wire(82, yt, 96, yt);
        zh(2, 96, xb);
        g.wire(xb, yt, xb + 18, yt);
        break;
    case 1:
        g.wire(xa, yt, 46, yt);
        zv(0, 46);
        zh(1, 46, 116);
        zv(2, 116);
        g.wire(116, yt, xb + 18, yt);
        break;
    case 2:
        zh(0, xa, 104);
        zv(1, 110);
        g.wire(104, yt, xb + 18, yt);
        break;
    default:
        g.wire(xa, yt, 60, yt);
        zv(0, 60);
        zh(1, 60, 130);
        g.wire(130, yt, xb + 18, yt);
        break;
    }
    char buf[48];
    int n = scopy(buf, sizeof buf, "|A_v| = ");
    fmtValue(buf + n, (int)sizeof(buf) - n, fm::hypot(s.v[OAV], s.v[OAV + 1]), U_NONE, 3);
    font::drawRich(*g.cv, SANS_12, g.ox + 8, g.oy + 156, buf, g.volt);
}
}  // namespace cell

// ==========================================================================
// Attenuateurs resistifs en T et en Pi
// ==========================================================================
namespace att {
enum { A, Z1, Z2, OK, OR1, OR2, OR3, OR1N, OR2N, OR3N, OA, OZE, OAMIN };
enum { CTOPO, CIMP, CSER };
enum { T_R1 = 1, T_R2, T_R3 };
static void def(CalcState &s) { s.v[A] = 6; s.v[Z1] = 50; s.v[Z2] = 75; }
static void rows(const CalcState &s, RowList &r)
{
    bool pi = s.c[CTOPO] == 1, diff = s.c[CIMP] == 1;
    r.choice("#Montage", CTOPO, "en T|en Π", 0, "Disposition des résistances");
    r.choice("#Impédances", CIMP, "égales Z_0|différentes Z_1, Z_2", 0, "Atténuateur adapté des deux côtés");
    r.in("A", A, U_DB, ST_LIN1, 0, "Atténuation voulue (en puissance)");
    r.in(diff ? "Z_1" : "Z_0", Z1, U_OHM, ST_E24, 0, diff ? "Impédance côté entrée" : "Impédance caractéristique (50 Ω, 75 Ω, 600 Ω…)");
    if (diff) r.in("Z_2", Z2, U_OHM, ST_E24, 0, "Impédance côté sortie");
    r.choice("#Arrondi", CSER, "E24|E96", 0, "Série normalisée pour les valeurs pratiques");
    r.head("Valeurs exactes");
    if (!pi) {
        r.out("R_1", OR1, U_OHM, "Bras série côté entrée", true, T_R1);
        r.out("R_2", OR2, U_OHM, "Bras série côté sortie", true, T_R2);
        r.out("R_3", OR3, U_OHM, "Résistance en parallèle (centre)", true, T_R3);
    } else {
        r.out("R_1", OR1, U_OHM, "Résistance en parallèle côté entrée", true, T_R1);
        r.out("R_2", OR2, U_OHM, "Résistance en parallèle côté sortie", true, T_R2);
        r.out("R_3", OR3, U_OHM, "Résistance série (centre)", true, T_R3);
    }
    r.out("K", OK, U_NONE, "Rapport 10^(A/20)");
    if (diff) r.out("A_{\\min}", OAMIN, U_DB, "Atténuation minimale possible pour Z₁ ≠ Z₂");
    r.head("Valeurs normalisées");
    r.out("R_1", OR1N, U_OHM, nullptr, false, T_R1);
    r.out("R_2", OR2N, U_OHM, nullptr, false, T_R2);
    r.out("R_3", OR3N, U_OHM, nullptr, false, T_R3);
    r.out("A", OA, U_DB, "Atténuation obtenue avec ces valeurs", true);
    r.out("Z_e", OZE, U_OHM, "Impédance d'entrée (sortie chargée)");
    r.msg(0);
}
static void calc(CalcState &s)
{
    bool pi = s.c[CTOPO] == 1, diff = s.c[CIMP] == 1;
    double z1 = s.v[Z1], z2 = diff ? s.v[Z2] : z1;
    double K = fm::pow10(s.v[A] / 20), K2 = K * K;
    s.v[OK] = K;
    double zm = fm::sqrt(z1 * z2);
    // attenuation minimale (Z1 != Z2) : sqrt(Z1/Z2) + sqrt(Z1/Z2 - 1), Z1 > Z2
    double ra = z1 > z2 ? z1 / z2 : z2 / z1;
    double kmin = fm::sqrt(ra) + fm::sqrt(ra - 1);
    s.v[OAMIN] = 20 * fm::log10(kmin);
    double r3 = 2 * zm * K / (K2 - 1);
    double r1 = z1 * (K2 + 1) / (K2 - 1) - r3;
    double r2 = z2 * (K2 + 1) / (K2 - 1) - r3;
    if (!(K > 1) || r1 < -1e-9 * z1 || r2 < -1e-9 * z2) {
        for (int k = OR1; k <= OZE; k++) s.v[k] = NaN();
        setMsg(s, 0, diff ? "Atténuation trop faible pour ces impédances (voir A_min)" : "Il faut A > 0 dB", ML_BAD);
        return;
    }
    if (r1 < 0) r1 = 0;
    if (r2 < 0) r2 = 0;
    if (pi) {
        // transformation etoile -> triangle
        double sp = r1 * r2 + r2 * r3 + r3 * r1;
        double a = r2 > 0 ? sp / r2 : fm::inf(), b = r1 > 0 ? sp / r1 : fm::inf(), c = sp / r3;
        r1 = a; r2 = b; r3 = c;
    }
    s.v[OR1] = r1; s.v[OR2] = r2; s.v[OR3] = r3;
    int ser = s.c[CSER] ? E96 : E24;
    double n1 = fm::isinf(r1) ? r1 : r1 > 0 ? eNearest(ser, r1) : 0;
    double n2 = fm::isinf(r2) ? r2 : r2 > 0 ? eNearest(ser, r2) : 0;
    double n3 = eNearest(ser, r3);
    s.v[OR1N] = n1; s.v[OR2N] = n2; s.v[OR3N] = n3;
    // attenuation obtenue : P1 / P2 avec charge Z2
    M2 T;
    Cx one = cx(1), zero = cx(0);
    if (!pi) {
        T.m[0][0] = one + cx(n1 / n3); T.m[0][1] = cx(n1 + n2 + n1 * n2 / n3);
        T.m[1][0] = cx(1 / n3); T.m[1][1] = one + cx(n2 / n3);
    } else {
        double y1 = fm::isinf(n1) ? 0 : 1 / n1, y2 = fm::isinf(n2) ? 0 : 1 / n2;
        T.m[0][0] = one + cx(n3 * y2); T.m[0][1] = cx(n3);
        T.m[1][0] = cx(y1 + y2 + n3 * y1 * y2); T.m[1][1] = one + cx(n3 * y1);
    }
    (void)zero;
    Cx zl = cx(z2);
    Cx v1 = T.m[0][0] * zl + T.m[0][1], i1 = T.m[1][0] * zl + T.m[1][1];   // pour V2 = Z2, I2 = -1
    Cx ze = v1 / i1;
    double p1 = (v1.re * i1.re + v1.im * i1.im), p2 = z2;
    s.v[OZE] = ze.re;
    s.v[OA] = 10 * fm::log10(p1 / p2);
    setMsg(s, 0, "Adapté : Z_e = Z_1 quand la sortie est chargée par Z_2", ML_OK);
}
static void sch(Sch &g, const CalcState &s)
{
    const int yt = 52, yb = 122;
    bool pi = s.c[CTOPO] == 1, diff = s.c[CIMP] == 1;
    g.wire(16, yb, 162, yb);
    g.term(16, yt);
    g.term(16, yb);
    g.term(162, yt);
    g.term(162, yb);
    if (!pi) {
        g.res(16, yt, 82, yt, T_R1);
        g.res(96, yt, 162, yt, T_R2);
        g.wire(82, yt, 96, yt);
        g.res(89, yt, 89, yb, T_R3);
        g.dot(89, yt);
        g.dot(89, yb);
        g.tex(49, yt - 10, "R_1", tex::CENTER, T_R1);
        g.tex(129, yt - 10, "R_2", tex::CENTER, T_R2);
        g.tex(97, 92, "R_3", tex::LEFT, T_R3);
    } else {
        g.wire(16, yt, 46, yt);
        g.res(46, yt, 46, yb, T_R1);
        g.res(46, yt, 132, yt, T_R3);
        g.res(132, yt, 132, yb, T_R2);
        g.wire(132, yt, 162, yt);
        g.dot(46, yt); g.dot(46, yb); g.dot(132, yt); g.dot(132, yb);
        g.tex(54, 92, "R_1", tex::LEFT, T_R1);
        g.tex(140, 92, "R_2", tex::LEFT, T_R2);
        g.tex(89, yt - 10, "R_3", tex::CENTER, T_R3);
    }
    g.tex(10, yt - 8, diff ? "Z_1" : "Z_0", tex::LEFT);
    g.tex(168, yt - 8, diff ? "Z_2" : "Z_0", tex::RIGHT);
    char buf[40];
    int n = scopy(buf, sizeof buf, "A = ");
    fmtValue(buf + n, (int)sizeof(buf) - n, s.v[OA], U_DB);
    font::drawText(*g.cv, BOLD_12, g.ox + 89 - font::width(BOLD_12, buf) / 2, g.oy + 152, buf, g.volt);
}
}  // namespace att

extern const CalcDef g_calcQuad[] = {
    {"Conversion des matrices Z, Y, H, G, T, S", "#Paramètres d'un quadripôle, chargé ou non",
     "quadripole matrice impedance admittance hybride transfert abcd chaine repartition s conversion reciproque", 10, G_NONE, 0,
     conv::def, conv::rows, conv::calc, conv::sch},
    {"Quadripôles en T, en Π et en L", "#Matrices, impédances images, Bode",
     "quadripole cellule t pi l matrice impedance image transfert bode", 10, G_BODE, 0,
     cell::def, cell::rows, cell::calc, cell::sch, cell::tf, cell::fc, nullptr, nullptr, 0},
    {"Atténuateurs en T et en Π", "#Résistances d'un atténuateur adapté",
     "attenuateur pad t pi resistance db adaptation impedance", 10, G_NONE, 0,
     att::def, att::rows, att::calc, att::sch},
};
extern const int g_calcQuadN = sizeof(g_calcQuad) / sizeof(g_calcQuad[0]);
