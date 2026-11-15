// Regime dynamique (petits signaux) des montages a transistors :
// emetteur, collecteur et base communs (bipolaire NPN), source, drain et
// grille communes (JFET ou MOSFET canal N), paire differentielle, parametres
// hybrides du bipolaire.
//
// Le point de repos est calcule depuis le pont de polarisation, puis le
// schema equivalent petits signaux (modele en pi : r_pi, g_m v_be, r_o, C_pi,
// C_mu) est resolu par analyse nodale complexe : en bande passante
// (condensateurs de liaison en court-circuit, capacites du transistor
// ouvertes) pour les gains et impedances, et a chaque frequence pour le
// diagramme de Bode et les frequences de coupure exactes.
#include "CalcUtil.hpp"

using namespace cu;

namespace amp {

enum Topo { T_CE, T_CC, T_CB };      // FET : source, drain, grille communes
enum Dev { D_BJT, D_JFET, D_MOS };

// --------------------------------------------------------------------------
// Analyse nodale
// --------------------------------------------------------------------------
enum { N_GND, N_SRC, N_IN, N_B, N_C, N_E, N_X, N_OUT, N_COUNT };
enum { EK_R, EK_CK, EK_CD, EK_GM, EK_SHORT };   // CK : liaison/decouplage ; CD : capacite du transistor

struct Elem {
    uint8_t kind, a, b;
    double v;
};

struct Model {
    int topo, dev;
    bool ok;               // point de repos utilisable
    double gm, rpi, ro;    // rpi, ro <= 0 : infinis
    double cpi, cmu;
    double rg, r1, r2, rc, re1, re2, rl;
    double ce, cs, cE, cB;
    bool hasCE;            // condensateur de decouplage d'emetteur (EC / SC)
};

enum { SRC_V, SRC_GND, SRC_OFF };

struct Net {
    Elem e[24];
    int n;
    void add(int k, int a, int b, double v)
    {
        if (n < 24) { e[n].kind = (uint8_t)k; e[n].a = (uint8_t)a; e[n].b = (uint8_t)b; e[n].v = v; n++; }
    }
};

// Elements du schema petits signaux. withRg : resistance de la source ;
// withRL : charge.
static void build(const Model &m, Net &net, bool withRg, bool withRL)
{
    net.n = 0;
    if (withRg) {
        if (m.rg > 0) net.add(EK_R, N_SRC, N_IN, m.rg);
        else net.add(EK_SHORT, N_SRC, N_IN, 0);
    }
    // liaison d'entree
    net.add(EK_CK, N_IN, m.topo == T_CB ? N_E : N_B, m.ce);
    // pont de base (alimentation = masse en alternatif)
    if (m.r1 > 0) net.add(EK_R, N_B, N_GND, m.r1);
    if (m.r2 > 0) net.add(EK_R, N_B, N_GND, m.r2);
    if (m.topo == T_CB) net.add(EK_CK, N_B, N_GND, m.cB);
    // collecteur
    if (m.topo == T_CC || !(m.rc > 0)) net.add(EK_SHORT, N_C, N_GND, 0);
    else net.add(EK_R, N_C, N_GND, m.rc);
    // emetteur
    if (m.topo == T_CE) {
        if (m.re1 > 0) net.add(EK_R, N_E, N_X, m.re1);
        else net.add(EK_SHORT, N_E, N_X, 0);
        if (m.re2 > 0) net.add(EK_R, N_X, N_GND, m.re2);
        else net.add(EK_SHORT, N_X, N_GND, 0);
        if (m.hasCE && m.re2 > 0) net.add(EK_CK, N_X, N_GND, m.cE);
    } else {
        if (m.re1 > 0) net.add(EK_R, N_E, N_GND, m.re1);
        else if (m.topo == T_CC) net.add(EK_SHORT, N_E, N_GND, 0);
    }
    // liaison de sortie et charge
    net.add(EK_CK, m.topo == T_CC ? N_E : N_C, N_OUT, m.cs);
    if (withRL) net.add(EK_R, N_OUT, N_GND, m.rl);
    // transistor
    if (m.rpi > 0) net.add(EK_R, N_B, N_E, m.rpi);
    if (m.ro > 0) net.add(EK_R, N_C, N_E, m.ro);
    if (m.cpi > 0) net.add(EK_CD, N_B, N_E, m.cpi);
    if (m.cmu > 0) net.add(EK_CD, N_B, N_C, m.cmu);
    net.add(EK_GM, N_C, N_E, m.gm);
}

struct Sol {
    Cx v[N_COUNT];
    bool ok;
};

static int findRep(int *p, int a)
{
    while (p[a] != a) { p[a] = p[p[a]]; a = p[a]; }
    return a;
}

// mid : bande passante (CK court-circuit, CD ouvert) ; sinon pulsation w.
// skip : element ignore (ouvert), -1 : aucun. Injection : +1 A en ia, -1 A en ib.
static Sol solve(const Net &net, bool mid, double w, int src, int ia, int ib, int skip = -1)
{
    Sol r;
    for (int k = 0; k < N_COUNT; k++) r.v[k] = cx(0);
    r.ok = false;
    int par[N_COUNT];
    for (int k = 0; k < N_COUNT; k++) par[k] = k;
    auto unite = [&](int a, int b) {
        a = findRep(par, a);
        b = findRep(par, b);
        if (a == b) return;
        if (a == N_GND || (a == N_SRC && b != N_GND)) par[b] = a;
        else par[a] = b;
    };
    for (int k = 0; k < net.n; k++) {
        if (k == skip) continue;
        const Elem &e = net.e[k];
        if (e.kind == EK_SHORT || (e.kind == EK_CK && mid)) unite(e.a, e.b);
    }
    if (src == SRC_GND) unite(N_SRC, N_GND);
    int gRep = findRep(par, N_GND), sRep = findRep(par, N_SRC);
    if (src == SRC_V && sRep == gRep) return r;
    // inconnues : noeuds relies a au moins un element
    bool used[N_COUNT];
    for (int k = 0; k < N_COUNT; k++) used[k] = false;
    for (int k = 0; k < net.n; k++) {
        if (k == skip) continue;
        const Elem &e = net.e[k];
        if (mid && e.kind == EK_CD) continue;
        used[findRep(par, e.a)] = used[findRep(par, e.b)] = true;
    }
    if (ia >= 0) used[findRep(par, ia)] = true;
    int idx[N_COUNT];
    int nu = 0;
    for (int k = 0; k < N_COUNT; k++) idx[k] = -1;
    for (int k = 0; k < N_COUNT; k++) {
        int rp = findRep(par, k);
        if (rp != k || !used[k] || rp == gRep || (src == SRC_V && rp == sRep)) continue;
        idx[k] = nu++;
    }
    Cx Y[N_COUNT][N_COUNT + 1];
    for (int i = 0; i < nu; i++)
        for (int j = 0; j <= nu; j++) Y[i][j] = cx(0);
    auto known = [&](int rp) -> Cx { return (src == SRC_V && rp == sRep) ? cx(1) : cx(0); };
    auto addY = [&](int row, int colNode, Cx val) {
        int ir = idx[findRep(par, row)];
        if (ir < 0) return;
        int rc = findRep(par, colNode);
        int ic = idx[rc];
        if (ic >= 0) Y[ir][ic] = Y[ir][ic] + val;
        else Y[ir][nu] = Y[ir][nu] - val * known(rc);
    };
    for (int k = 0; k < net.n; k++) {
        if (k == skip) continue;
        const Elem &e = net.e[k];
        Cx y;
        switch (e.kind) {
        case EK_R: y = cx(1.0 / e.v); break;
        case EK_CK: if (mid) continue; y = cx(0, w * e.v); break;
        case EK_CD: if (mid) continue; y = cx(0, w * e.v); break;
        case EK_GM: {
            // courant g_m (v_B - v_E) sortant du noeud C (vers E)
            Cx g = cx(e.v);
            addY(N_C, N_B, g);
            addY(N_C, N_E, -g);
            addY(N_E, N_B, -g);
            addY(N_E, N_E, g);
            continue;
        }
        default: continue;
        }
        if (findRep(par, e.a) == findRep(par, e.b)) continue;
        addY(e.a, e.a, y);
        addY(e.b, e.b, y);
        addY(e.a, e.b, -y);
        addY(e.b, e.a, -y);
    }
    if (ia >= 0) { int i = idx[findRep(par, ia)]; if (i >= 0) Y[i][nu] = Y[i][nu] + cx(1); }
    if (ib >= 0) { int i = idx[findRep(par, ib)]; if (i >= 0) Y[i][nu] = Y[i][nu] - cx(1); }
    // elimination de Gauss avec pivot partiel
    for (int c = 0; c < nu; c++) {
        int p = c;
        double best = 0;
        for (int i = c; i < nu; i++) {
            double a = fm::fabs(Y[i][c].re) + fm::fabs(Y[i][c].im);
            if (a > best) { best = a; p = i; }
        }
        if (!(best > 0)) return r;
        if (p != c)
            for (int j = 0; j <= nu; j++) { Cx t = Y[c][j]; Y[c][j] = Y[p][j]; Y[p][j] = t; }
        for (int i = c + 1; i < nu; i++) {
            Cx f = Y[i][c] / Y[c][c];
            if (f.re == 0 && f.im == 0) continue;
            for (int j = c; j <= nu; j++) Y[i][j] = Y[i][j] - f * Y[c][j];
        }
    }
    Cx x[N_COUNT];
    for (int i = nu - 1; i >= 0; i--) {
        Cx acc = Y[i][nu];
        for (int j = i + 1; j < nu; j++) acc = acc - Y[i][j] * x[j];
        x[i] = acc / Y[i][i];
    }
    for (int k = 0; k < N_COUNT; k++) {
        int rp = findRep(par, k);
        r.v[k] = idx[rp] >= 0 ? x[idx[rp]] : known(rp);
    }
    r.ok = true;
    for (int k = 0; k < N_COUNT; k++) if (cbad(r.v[k])) r.ok = false;
    return r;
}

// Gain v_s / v_g a la pulsation w (mid : bande passante).
static bool gainAt(const Model &m, bool mid, double w, Cx &H)
{
    Net net;
    build(m, net, true, true);
    Sol s = solve(net, mid, w, SRC_V, -1, -1);
    H = s.v[N_OUT];
    return s.ok;
}

struct Perf {
    double av, avg, ai, gp, ze, zs;
    double fk[4];      // frequences des condensateurs de liaison (constantes de temps)
    double fb, fh;
};

// Resistance vue par le condensateur de liaison k (autres en court-circuit).
static double seenBy(const Net &net, int k)
{
    Sol s = solve(net, true, 0, SRC_GND, net.e[k].a, net.e[k].b, k);
    if (!s.ok) return NaN();
    return (s.v[net.e[k].a] - s.v[net.e[k].b]).re;
}

static void perf(const Model &m, Perf &p)
{
    for (int k = 0; k < 4; k++) p.fk[k] = NaN();
    p.av = p.avg = p.ai = p.gp = p.ze = p.zs = p.fb = p.fh = NaN();
    if (!m.ok) return;
    Net net;
    build(m, net, true, true);
    Sol s = solve(net, true, 0, SRC_V, -1, -1);
    if (!s.ok) return;
    p.avg = s.v[N_OUT].re;
    p.av = s.v[N_IN].re != 0 ? s.v[N_OUT].re / s.v[N_IN].re : NaN();
    Net nz;
    build(m, nz, false, true);
    Sol z = solve(nz, true, 0, SRC_OFF, N_IN, -1);
    p.ze = z.ok ? z.v[N_IN].re : NaN();
    build(m, nz, true, false);
    z = solve(nz, true, 0, SRC_GND, N_OUT, -1);
    p.zs = z.ok ? z.v[N_OUT].re : NaN();
    p.ai = p.av * p.ze / m.rl;
    double ap = fm::fabs(p.av * p.ai);
    p.gp = ap > 0 ? 10 * fm::log10(ap) : NaN();
    // condensateurs de liaison : e, s, E (ou B)
    int slot = 0;
    for (int k = 0; k < net.n && slot < 4; k++) {
        if (net.e[k].kind != EK_CK) continue;
        double R = seenBy(net, k);
        int which;
        const Elem &e = net.e[k];
        if (e.a == N_IN) which = 0;
        else if (e.b == N_OUT) which = 1;
        else which = 2;
        p.fk[which] = (R > 0) ? 1.0 / (fm::TWO_PI * R * e.v) : NaN();
        slot++;
    }
    // frequences de coupure exactes : |A_vg| = |A_vg,0| / sqrt(2)
    double target = fm::fabs(p.avg) / fm::sqrt(2.0);
    auto mag = [&](double l) {
        Cx H;
        if (!gainAt(m, false, fm::TWO_PI * fm::pow10(l), H)) return -1.0;
        return cabs(H);
    };
    const double L0 = -3, L1 = 11, ST = 0.25;
    double prev = mag(L0);
    bool above = prev >= target;
    p.fb = above ? 0 : NaN();
    p.fh = fm::inf();
    bool foundLow = above;
    for (double l = L0 + ST; l <= L1 + 1e-9; l += ST) {
        double cur = mag(l);
        bool a = cur >= target;
        if (a != above) {
            double lo = l - ST, hi = l;
            for (int it = 0; it < 44; it++) {
                double md = 0.5 * (lo + hi);
                if ((mag(md) >= target) == above) lo = md;
                else hi = md;
            }
            double f = fm::pow10(0.5 * (lo + hi));
            if (!foundLow && a) { p.fb = f; foundLow = true; }
            else if (foundLow && !a) { p.fh = f; break; }
        }
        above = a;
    }
    if (!foundLow) p.fb = NaN();
}

// --------------------------------------------------------------------------
// Point de repos
// --------------------------------------------------------------------------
struct Bias {
    double i, v;   // I_C (ou I_D), V_CE (ou V_DS)
    double vgs;    // FET
    int mode;      // 0 bloque, 1 lineaire / saturation (FET), 2 sature / ohmique
};

static Bias bjtBias(double vcc, double r1, double r2, double rc, double re, double beta, double vbe)
{
    Bias b{0, vcc, 0, 0};
    double vth = r1 > 0 ? vcc * r2 / (r1 + r2) : vcc;
    double rth = r1 > 0 ? par2(r1, r2) : r2;
    double ib = (vth - vbe) / (rth + (beta + 1) * re);
    if (!(ib > 0)) return b;
    b.i = beta * ib;
    b.v = vcc - rc * b.i - re * (b.i + ib);
    b.mode = b.v < 0.2 ? 2 : 1;
    return b;
}

static double fetId(int dev, double vgs, double p1, double p2)
{
    // JFET : p1 = I_DSS, p2 = V_P (< 0) ; MOSFET : p1 = K, p2 = V_th
    if (dev == D_JFET) {
        if (vgs <= p2) return 0;
        double u = 1 - vgs / p2;
        return p1 * u * u;
    }
    if (vgs <= p2) return 0;
    return p1 * (vgs - p2) * (vgs - p2);
}

static Bias fetBias(int dev, double vdd, double r1, double r2, double rd, double rs, double p1, double p2)
{
    Bias b{0, vdd, 0, 0};
    double vg = r1 > 0 ? vdd * r2 / (r1 + r2) : 0;
    double hi = fetId(dev, vg, p1, p2);
    double id = hi;
    if (rs > 0 && hi > 0) {
        double lo = 0;
        for (int it = 0; it < 90; it++) {
            double md = 0.5 * (lo + hi);
            if (fetId(dev, vg - md * rs, p1, p2) > md) lo = md;
            else hi = md;
        }
        id = 0.5 * (lo + hi);
    }
    b.i = id;
    b.vgs = vg - id * rs;
    b.v = vdd - (rd + rs) * id;
    if (!(id > 0)) { b.mode = 0; return b; }
    double vov = b.vgs - p2;   // V_GS - V_th (ou - V_P)
    b.mode = b.v >= vov ? 1 : 2;
    return b;
}

static double fetGm(int dev, double vgs, double p1, double p2)
{
    if (dev == D_JFET) return vgs > p2 ? 2 * p1 / fm::fabs(p2) * (1 - vgs / p2) : 0;
    return vgs > p2 ? 2 * p1 * (vgs - p2) : 0;
}

// --------------------------------------------------------------------------
// Etat commun (indices dans CalcState::v)
// --------------------------------------------------------------------------
enum {
    VCC, R1, R2, RC, RE1, RE2, RL, RG, CE_, CS_, CEM, CB_,
    BETA, VBE, VA, VT, CPI, CMU,          // bipolaire
    P1, P2, LAMBDA, CGS, CGD,             // FET (P1 : I_DSS ou K ; P2 : V_P ou V_th)
    OI, OV, OVGS, OGM, ORPI, ORO, OAV, OAVG, OAI, OGP, OZE, OZS, OF0, OF1, OF2, OFB, OFH, OBW,
};
enum { CVIEW, CDEV, CHASCE };
enum {
    TG_RG = 1, TG_CE, TG_R1, TG_R2, TG_RC, TG_RE1, TG_RE2, TG_CEM, TG_CS, TG_RL, TG_Q, TG_CB, TG_VCC,
};

struct Kind {
    int topo;
    bool fet;
};

static int devOf(const Kind &k, const CalcState &s)
{
    if (!k.fet) return D_BJT;
    return s.c[CDEV] ? D_MOS : D_JFET;
}

// Le MOSFET a ses propres cases (K, V_th, pont) : changer de transistor
// conserve des valeurs coherentes pour chacun.
static int sR1(int dev) { return dev == D_MOS ? VA : R1; }
static int sR2(int dev) { return dev == D_MOS ? VT : R2; }
static int sP1(int dev) { return dev == D_MOS ? BETA : P1; }
static int sP2(int dev) { return dev == D_MOS ? VBE : P2; }

static void fillModel(const Kind &k, const CalcState &s, Model &m, Bias *bOut = nullptr)
{
    m.topo = k.topo;
    m.dev = devOf(k, s);
    m.rg = s.v[RG];
    m.r1 = s.v[sR1(m.dev)];
    m.r2 = s.v[sR2(m.dev)];
    m.rc = k.topo == T_CC ? 0 : s.v[RC];
    m.re1 = s.v[RE1];
    m.re2 = k.topo == T_CE ? s.v[RE2] : 0;
    m.rl = s.v[RL];
    m.ce = s.v[CE_];
    m.cs = s.v[CS_];
    m.cE = s.v[CEM];
    m.cB = s.v[CB_];
    m.hasCE = k.topo == T_CE && s.c[CHASCE] == 0;
    double reDc = m.re1 + m.re2;
    Bias b;
    if (m.dev == D_BJT) {
        b = bjtBias(s.v[VCC], m.r1, m.r2, m.rc, reDc, s.v[BETA], s.v[VBE]);
        m.gm = b.i / s.v[VT];
        m.rpi = b.i > 0 ? s.v[BETA] / m.gm : 0;
        m.ro = s.v[VA] > 0 && b.i > 0 ? s.v[VA] / b.i : 0;
        m.cpi = s.v[CPI];
        m.cmu = s.v[CMU];
    } else {
        double p1 = s.v[sP1(m.dev)], p2 = s.v[sP2(m.dev)];
        b = fetBias(m.dev, s.v[VCC], m.r1, m.r2, m.rc, reDc, p1, p2);
        m.gm = fetGm(m.dev, b.vgs, p1, p2);
        m.rpi = 0;
        m.ro = s.v[LAMBDA] > 0 && b.i > 0 ? 1.0 / (s.v[LAMBDA] * b.i) : 0;
        m.cpi = s.v[CGS];
        m.cmu = s.v[CGD];
    }
    m.ok = b.mode == 1 && m.gm > 0;
    if (bOut) *bOut = b;
}

static void defaults(const Kind &k, CalcState &s)
{
    s.v[RG] = 600; s.v[RL] = 10e3; s.v[CE_] = 10e-6; s.v[CS_] = 10e-6; s.v[CEM] = 100e-6; s.v[CB_] = 100e-6;
    s.v[BETA] = 200; s.v[VBE] = 0.65; s.v[VA] = 100; s.v[VT] = 26e-3; s.v[CPI] = 20e-12; s.v[CMU] = 4e-12;
    s.v[P1] = 8e-3; s.v[P2] = -2; s.v[LAMBDA] = 0.01; s.v[CGS] = 5e-12; s.v[CGD] = 2e-12;
    if (!k.fet) {
        s.v[VCC] = 12;
        switch (k.topo) {
        case T_CE: s.v[R1] = 47e3; s.v[R2] = 10e3; s.v[RC] = 4.7e3; s.v[RE1] = 100; s.v[RE2] = 1e3; break;
        case T_CC: s.v[R1] = 22e3; s.v[R2] = 22e3; s.v[RE1] = 2.2e3; s.v[RL] = 1e3; break;
        default: s.v[R1] = 47e3; s.v[R2] = 10e3; s.v[RC] = 4.7e3; s.v[RE1] = 1e3; s.v[RG] = 50; break;
        }
    } else {
        s.v[VCC] = 15;
        switch (k.topo) {
        case T_CE: s.v[RC] = 2.2e3; s.v[RE1] = 0; s.v[RE2] = 470; break;
        case T_CC: s.v[RE1] = 1e3; s.v[RL] = 2.2e3; break;
        default: s.v[RC] = 2.2e3; s.v[RE1] = 470; s.v[RG] = 50; break;
        }
        s.v[R1] = 0; s.v[R2] = 1e6;                 // JFET : autopolarisation
        s.v[VA] = 2.2e6; s.v[VT] = 1e6;             // MOSFET : pont diviseur
        s.v[BETA] = 20e-3; s.v[VBE] = 2;            // MOSFET : K, V_th
    }
}

// Libelles selon le transistor.
struct Names {
    const char *vcc, *rc, *re1, *re2, *re, *cE, *cB, *i, *v;
};
static const Names NB = {"V_{CC}", "R_C", "R_{E1}", "R_{E2}", "R_E", "C_E", "C_B", "I_C", "V_{CE}"};
static const Names NF = {"V_{DD}", "R_D", "R_{S1}", "R_{S2}", "R_S", "C_S", "C_G", "I_D", "V_{DS}"};

static void rowsOf(const Kind &k, const CalcState &s, RowList &r)
{
    const Names &N = k.fet ? NF : NB;
    int dev = devOf(k, s);
    r.choice("#Schéma", CVIEW, "montage|petits signaux", 0, "Montage complet ou schéma équivalent petits signaux");
    if (k.fet) r.choice("#Transistor", CDEV, "JFET canal N|MOSFET canal N", TG_Q);
    r.head("Polarisation");
    r.in(N.vcc, VCC, U_V, ST_LIN1, TG_VCC, "Tension d'alimentation");
    r.in("R_1", sR1(dev), U_OHM, ST_E24, TG_R1, k.fet ? "Pont : entre +V_DD et la grille (0 : absente)" : "Pont : entre +V_CC et la base",
         k.fet ? RF_NONNEG : RF_POS);
    r.in("R_2", sR2(dev), U_OHM, ST_E24, TG_R2, k.fet ? "Entre la grille et la masse (R_G)" : "Pont : entre la base et la masse");
    if (k.topo != T_CC) r.in(N.rc, RC, U_OHM, ST_E24, TG_RC, k.fet ? "Résistance de drain" : "Résistance de collecteur");
    if (k.topo == T_CE) {
        r.in(N.re1, RE1, U_OHM, ST_E24, TG_RE1, k.fet ? "Partie de R_S non découplée (0 possible)" : "Partie de R_E non découplée (0 possible)", RF_NONNEG);
        r.in(N.re2, RE2, U_OHM, ST_E24, TG_RE2, k.fet ? "Partie de R_S découplée par C_S" : "Partie de R_E découplée par C_E", RF_NONNEG);
        r.choice(N.cE, CHASCE, "présent|absent", TG_CEM, "Condensateur de découplage en parallèle sur la 2e résistance");
    } else {
        r.in(N.re, RE1, U_OHM, ST_E24, TG_RE1, k.fet ? "Résistance de source" : "Résistance d'émetteur",
             k.topo == T_CC ? RF_POS : RF_NONNEG);
    }
    r.head("Transistor");
    if (!k.fet) {
        r.in("\\beta", BETA, U_NONE, ST_LIN10, TG_Q, "Gain en courant β = h₂₁ₑ");
        r.in("V_{BE}", VBE, U_V, ST_LIN01, TG_Q, "Tension base-émetteur en conduction");
        r.in("V_A", VA, U_V, ST_LIN10, TG_Q, "Tension d'Early (0 : r_o infinie)", RF_NONNEG);
        r.in("V_T", VT, U_V, ST_NONE, 0, "Tension thermique kT/q (26 mV à 25 °C)");
        r.in("C_\\pi", CPI, U_F, ST_E12, TG_Q, "Capacité base-émetteur (0 : ignorée)", RF_NONNEG);
        r.in("C_\\mu", CMU, U_F, ST_E12, TG_Q, "Capacité base-collecteur (effet Miller, 0 : ignorée)", RF_NONNEG);
    } else {
        if (dev == D_JFET) {
            r.in("I_{DSS}", P1, U_A, ST_E12, TG_Q, "Courant de drain à V_GS = 0");
            r.in("V_P", P2, U_V, ST_LIN01, TG_Q, "Tension de pincement V_GS(off) (négative)", 0);
        } else {
            r.in("K", BETA, U_AV2, ST_E12, TG_Q, "I_D = K (V_GS − V_th)²");
            r.in("V_{th}", VBE, U_V, ST_LIN01, TG_Q, "Tension de seuil (positive)", 0);
        }
        r.in("\\lambda", LAMBDA, U_NONE, ST_NONE, TG_Q, "Modulation de longueur de canal (1/V, 0 : r_ds infinie)", RF_NONNEG);
        r.in("C_{gs}", CGS, U_F, ST_E12, TG_Q, "Capacité grille-source (0 : ignorée)", RF_NONNEG);
        r.in("C_{gd}", CGD, U_F, ST_E12, TG_Q, "Capacité grille-drain (effet Miller, 0 : ignorée)", RF_NONNEG);
    }
    r.head("Source, charge, liaisons");
    r.in("R_g", RG, U_OHM, ST_E24, TG_RG, "Résistance interne du générateur", RF_NONNEG);
    r.in("R_L", RL, U_OHM, ST_E24, TG_RL, "Résistance de charge");
    r.in("C_e", CE_, U_F, ST_E12, TG_CE, "Condensateur de liaison d'entrée");
    r.in("C_s", CS_, U_F, ST_E12, TG_CS, "Condensateur de liaison de sortie");
    if (k.topo == T_CE && s.c[CHASCE] == 0) r.in(N.cE, CEM, U_F, ST_E12, TG_CEM, "Condensateur de découplage");
    if (k.topo == T_CB) r.in(N.cB, CB_, U_F, ST_E12, TG_CB, k.fet ? "Découplage de la grille" : "Découplage de la base");
    r.head("Point de repos");
    r.out(N.i, OI, U_A, k.fet ? "Courant de drain de repos" : "Courant de collecteur de repos", false, TG_Q);
    r.out(N.v, OV, U_V, k.fet ? "Tension drain-source de repos" : "Tension collecteur-émetteur de repos");
    if (k.fet) r.out("V_{GS}", OVGS, U_V, "Tension grille-source de repos");
    r.msg(0);
    r.head("Modèle petits signaux");
    r.out("g_m", OGM, U_SIE, k.fet ? (dev == D_JFET ? "g_m = 2 I_DSS/|V_P| · (1 − V_GS/V_P)" : "g_m = 2 K (V_GS − V_th) = 2√(K I_D)")
                                     : "Transconductance I_C / V_T");
    if (!k.fet) r.out("r_\\pi", ORPI, U_OHM, "r_π = β / g_m = h₁₁ₑ");
    r.out(k.fet ? "r_{ds}" : "r_o", ORO, U_OHM, k.fet ? "r_ds = 1/(λ I_D)" : "r_o = V_A / I_C = 1/h₂₂ₑ");
    r.head("Bande passante (milieu)");
    r.out("A_v", OAV, U_NONE, "Gain en tension v_s / v_e", true);
    r.out("A_{vg}", OAVG, U_NONE, "Gain depuis le générateur v_s / v_g (R_g comprise)");
    r.out("A_i", OAI, U_NONE, "Gain en courant i_s / i_e = A_v Z_e / R_L");
    r.out("G_p", OGP, U_DB, "Gain en puissance 10 log |A_v A_i|");
    r.out("Z_e", OZE, U_OHM, "Impédance d'entrée (vue du générateur, pont compris)", true);
    r.out("Z_s", OZS, U_OHM, "Impédance de sortie (vue de la charge, R_g comprise)", true);
    r.head("Fréquences de coupure");
    r.out("f_{C_e}", OF0, U_HZ, "1/(2π R C_e), R vue par C_e (autres C en court-circuit)", false, TG_CE);
    r.out("f_{C_s}", OF1, U_HZ, "1/(2π R C_s), R vue par C_s", false, TG_CS);
    if (k.topo == T_CE && s.c[CHASCE] == 0)
        r.out(k.fet ? "f_{C_S}" : "f_{C_E}", OF2, U_HZ, "1/(2π R C), R vue par le découplage", false, TG_CEM);
    if (k.topo == T_CB) r.out(k.fet ? "f_{C_G}" : "f_{C_B}", OF2, U_HZ, "1/(2π R C), R vue par le découplage", false, TG_CB);
    r.out("f_b", OFB, U_HZ, "Coupure basse exacte (−3 dB), tous condensateurs", true);
    r.out("f_h", OFH, U_HZ, "Coupure haute exacte (C_π, C_μ ou C_gs, C_gd)", true);
    r.out("B", OBW, U_HZ, "Bande passante f_h − f_b");
    r.msg(1);
}

static void calcOf(const Kind &k, CalcState &s)
{
    Model m;
    Bias b;
    fillModel(k, s, m, &b);
    s.v[OI] = b.i;
    s.v[OV] = b.v;
    s.v[OVGS] = b.vgs;
    const char *ms;
    uint8_t lv = ML_OK;
    if (k.fet && devOf(k, s) == D_JFET && !(s.v[P2] < 0)) { ms = "V_P doit être négative (canal N)"; lv = ML_BAD; m.ok = false; }
    else if (b.mode == 0) { ms = k.fet ? "Bloqué : V_GS sous le seuil" : "Bloqué : la jonction base-émetteur ne conduit pas"; lv = ML_BAD; }
    else if (b.mode == 2) { ms = k.fet ? "Régime ohmique : V_DS < V_GS − V_th" : "Saturé : V_CE trop faible"; lv = ML_BAD; }
    else ms = k.fet ? "Saturation (source de courant) : amplification" : "Régime linéaire : amplification possible";
    setMsg(s, 0, ms, lv);
    s.v[OGM] = m.ok ? m.gm : NaN();
    s.v[ORPI] = m.ok && m.rpi > 0 ? m.rpi : NaN();
    s.v[ORO] = m.ok ? (m.ro > 0 ? m.ro : fm::inf()) : NaN();
    Perf p;
    perf(m, p);
    s.v[OAV] = p.av; s.v[OAVG] = p.avg; s.v[OAI] = p.ai; s.v[OGP] = p.gp; s.v[OZE] = p.ze; s.v[OZS] = p.zs;
    s.v[OF0] = p.fk[0]; s.v[OF1] = p.fk[1]; s.v[OF2] = p.fk[2];
    s.v[OFB] = p.fb; s.v[OFH] = p.fh;
    s.v[OBW] = ok(p.fh) && ok(p.fb) ? p.fh - p.fb : (fm::isinf(p.fh) ? fm::inf() : NaN());
    if (!m.ok) setMsg(s, 1, "Pas de petits signaux hors du régime linéaire", ML_WARN);
    else if (!(s.v[CPI] > 0 || s.v[CMU] > 0) && !k.fet) setMsg(s, 1, "Sans C_π ni C_μ, pas de coupure haute", ML_INFO);
    else setMsg(s, 1, "F4 : diagramme de Bode de A_vg = v_s / v_g", ML_INFO);
}

static bool tfOf(const Kind &k, const CalcState &s, double f, Cx &H)
{
    Model m;
    fillModel(k, s, m);
    if (!m.ok) return false;
    return gainAt(m, false, fm::TWO_PI * f, H);
}

static double fcOf(const CalcState &s)
{
    double fb = s.v[OFB], fh = s.v[OFH];
    if (ok(fb) && fb > 0 && ok(fh)) return fm::sqrt(fb * fh);
    if (ok(fb) && fb > 0) return fb * 100;
    return 1000;
}

// --------------------------------------------------------------------------
// Dessins
// --------------------------------------------------------------------------
static void val(Sch &g, int x, int y, const char *name, double v, uint8_t unit, int align = 0)
{
    char buf[40];
    int k = 0;
    buf[0] = 0;
    if (name) { k = scopy(buf, sizeof buf, name); k += scopy(buf + k, (int)sizeof(buf) - k, " = "); }
    fmtValue(buf + k, (int)sizeof(buf) - k, v, unit, 3);
    int w = font::richWidth(SANS_12, buf);
    int xx = align == 2 ? x - w : align == 1 ? x - w / 2 : x;
    if (xx + w > 177) xx = 177 - w;
    font::drawRich(*g.cv, SANS_12, g.ox + xx, g.oy + y, buf, g.volt);
}

// Symbole du transistor (base / grille en (x, y)) ; renvoie l'abscisse des bornes.
static int device(Sch &g, int dev, int x, int y)
{
    if (dev == D_BJT) { g.bjt(x, y, true, TG_Q); return x + 17; }
    if (dev == D_JFET) { g.jfet(x, y, true, TG_Q); return x + 19; }
    g.mosfet(x, y, true, TG_Q);
    return x + 18;
}

// Tag d'une association (R1 // R2...) : composant selectionne s'il en fait partie.
static int pick(const Sch &g, int a, int b, int c = 0)
{
    if (g.hilite && (g.hilite == a || g.hilite == b || g.hilite == c)) return g.hilite;
    return a;
}

static void drawMontage(Sch &g, const Kind &k, const CalcState &s)
{
    const Names &N = k.fet ? NF : NB;
    int dev = devOf(k, s);
    const int yt = 14, Y = 152, xb = 56, bx = 74, by = 84, xl = 152;
    bool fetSelf = k.fet && !(s.v[sR1(dev)] > 0);
    int xq = bx + (dev == D_BJT ? 17 : dev == D_JFET ? 19 : 18);
    // alimentation
    g.wire(fetSelf ? xq : xb, yt, xq + 18, yt, TG_VCC);
    g.tex(xq + 20, yt + 4, N.vcc, tex::LEFT, TG_VCC);
    // pont de base
    if (!fetSelf) {
        g.res(xb, yt, xb, by, TG_R1);
        g.tex(xb - 8, 52, "R_1", tex::RIGHT, TG_R1);
        g.dot(xb, yt);
    }
    if (k.topo == T_CB) {
        g.res(xb, by, xb, 120, TG_R2);
        g.gnd(xb, 120);
        g.tex(xb - 8, 110, "R_2", tex::RIGHT, TG_R2);
        g.dot(66, by);
        g.cap(66, by, 66, 120, TG_CB);
        g.gnd(66, 120);
        g.texSmall(71, 112, N.cB, tex::LEFT, TG_CB);
    } else {
        g.res(xb, by, xb, Y, TG_R2);
        g.tex(xb - 8, 124, "R_2", tex::RIGHT, TG_R2);
        g.dot(xb, Y);
    }
    g.wire(xb, by, bx, by);
    if (!fetSelf || k.topo == T_CB) g.dot(xb, by);
    device(g, dev, bx, by);
    int yc = by - 20, ye = by + 20;
    // collecteur / drain
    if (k.topo == T_CC) {
        g.wire(xq, yt, xq, yc, TG_VCC);
    } else {
        g.res(xq, yt, xq, yc, TG_RC);
        g.tex(xq + 8, 44, N.rc, tex::LEFT, TG_RC);
    }
    g.dot(xq, yt);
    // entree
    if (k.topo == T_CB) {
        const int yin = 140, yg = 168;
        g.vsrc(8, yin, yg, 1, TG_RG);
        g.gnd(8, yg);
        g.res(8, yin, 30, yin, TG_RG);
        g.tex(19, yin - 8, "R_g", tex::CENTER, TG_RG);
        g.cap(30, yin, 62, yin, TG_CE);
        g.tex(46, yin + 17, "C_e", tex::CENTER, TG_CE);
        g.wire(62, yin, xq, yin);
        g.wire(xq, ye, xq, yin);
        g.res(xq, yin, xq, yg, TG_RE1);
        g.gnd(xq, yg);
        g.dot(xq, yin);
        g.tex(xq + 7, 160, N.re, tex::LEFT, TG_RE1);
    } else {
        g.vsrc(8, by + 8, Y, 1, TG_RG);
        g.wire(8, by, 8, by + 8, TG_RG);
        g.res(8, by, 28, by, TG_RG);
        g.tex(18, by + 17, "R_g", tex::CENTER, TG_RG);
        g.cap(28, by, xb, by, TG_CE);
        g.tex(42, by - 9, "C_e", tex::CENTER, TG_CE);
        g.wire(8, Y, xl, Y);
        g.gnd(30, Y);
    }
    // emetteur / source
    if (k.topo == T_CE) {
        int yx = 126;
        if (s.v[RE1] > 0) g.res(xq, ye, xq, yx, TG_RE1);
        else g.wire(xq, ye, xq, yx);
        g.res(xq, yx, xq, Y, TG_RE2);
        if (s.v[RE1] > 0) g.tex(xq - 6, 120, N.re1, tex::RIGHT, TG_RE1);
        g.tex(xq - 6, 146, N.re2, tex::RIGHT, TG_RE2);
        if (s.c[CHASCE] == 0) {
            g.wire(xq, yx, xq + 24, yx, TG_CEM);
            g.cap(xq + 24, yx, xq + 24, Y, TG_CEM);
            g.dot(xq, yx);
            g.tex(xq + 30, 146, N.cE, tex::LEFT, TG_CEM);
            g.dot(xq + 24, Y);
        }
        g.dot(xq, Y);
    } else if (k.topo == T_CC) {
        g.res(xq, ye, xq, Y, TG_RE1);
        g.tex(xq - 7, 140, N.re, tex::RIGHT, TG_RE1);
        g.dot(xq, Y);
    }
    // sortie
    int yo = k.topo == T_CC ? ye + 12 : yc + 8;
    int xo = 132;
    g.dot(xq, yo);
    g.cap(xq, yo, xo, yo, TG_CS);
    g.tex((xq + xo) / 2 + 2, yo - 9, "C_s", tex::CENTER, TG_CS);
    g.wire(xo, yo, xl, yo);
    int yl = k.topo == T_CB ? 118 : Y;
    g.res(xl, yo, xl, yl, TG_RL);
    if (k.topo == T_CB) g.gnd(xl, yl);
    else g.dot(xl, Y);
    g.tex(xl + 6, (yo + yl) / 2 + 5, "R_L", tex::LEFT, TG_RL);
    g.wire(xl, yo, 170, yo);
    g.dot(xl, yo);
    g.term(170, yo);
    g.tex(177, yo - 6, "v_s", tex::RIGHT);
    // resultat
    val(g, 177, k.topo == T_CB ? 34 : 168, "A_v", s.v[OAV], U_NONE, 2);
}

// Schema equivalent petits signaux.
static void drawSmall(Sch &g, const Kind &k, const CalcState &s)
{
    int dev = devOf(k, s);
    bool fet = dev != D_BJT;
    const int Y = 150, yt = 46;
    const char *gmLab = fet ? "g_mv_{gs}" : "g_mv_{be}";
    const char *roLab = fet ? "r_{ds}" : "r_o";
    bool hasRo = ok(s.v[ORO]) && fm::isfinite(s.v[ORO]);
    bool fetSelf = fet && !(s.v[sR1(dev)] > 0);
    const char *rbLab = fet ? (fetSelf ? "R_2" : "R_1\\parallel R_2") : "R_1\\parallel R_2";
    // generateur
    g.vsrc(10, yt + 40, Y, 1, TG_RG);
    g.wire(10, yt, 10, yt + 40, TG_RG);
    g.res(10, yt, 34, yt, TG_RG);
    g.tex(22, yt + 16, "R_g", tex::CENTER, TG_RG);
    g.tex(22, 104, "v_g", tex::LEFT);
    g.wire(10, Y, 170, Y);
    g.gnd(24, Y);
    if (k.topo == T_CE || k.topo == T_CC) {
        // base / grille
        int xbn = 44;
        g.wire(34, yt, xbn, yt);
        g.res(xbn, yt, xbn, Y, pick(g, TG_R2, TG_R1));
        g.texSmall(xbn + 6, Y + 13, rbLab, tex::CENTER, pick(g, TG_R2, TG_R1));
        g.dot(xbn, yt);
        g.dot(xbn, Y);
        g.texSmall(xbn, yt - 4, fet ? "#G" : "#B", tex::CENTER);
        if (k.topo == T_CE) {
            const int ye = 118, xr = 66, xg = 104, xo = 130, xl = 160;
            if (!fet) {
                g.wire(xbn, yt, xr, yt);
                g.res(xr, yt, xr, ye, TG_Q);
                g.tex(xr + 7, 86, "r_\\pi", tex::LEFT, TG_Q);
            } else {
                g.texSmall(xr + 2, 86, "v_{gs}", tex::LEFT);
            }
            g.wire(xg, yt, xl, yt);
            g.isrc(xg, yt, xg, ye, TG_Q);
            g.texSmall(xg + 2, ye + 13, gmLab, tex::LEFT, TG_Q);
            if (hasRo) {
                g.res(xo, yt, xo, ye, TG_Q);
                g.tex(xo + 7, 86, roLab, tex::LEFT, TG_Q);
                g.dot(xo, yt);
            }
            g.wire(fet ? xg : xr, ye, hasRo ? xo : xg, ye);
            // resistance d'emetteur non decouplee
            double reAc = s.v[RE1] + (s.c[CHASCE] ? s.v[RE2] : 0);
            int xe = 86;
            if (fet) g.wire(xr, ye, xg, ye);
            if (reAc > 0) {
                g.res(xe, ye, xe, Y, s.c[CHASCE] ? pick(g, TG_RE1, TG_RE2, TG_CEM) : TG_RE1);
                g.texSmall(xe - 5, 140, s.c[CHASCE] ? (fet ? "R_S" : "R_E") : (fet ? "R_{S1}" : "R_{E1}"), tex::RIGHT,
                           s.c[CHASCE] ? pick(g, TG_RE1, TG_RE2, TG_CEM) : TG_RE1);
            } else {
                g.wire(xe, ye, xe, Y);
            }
            g.dot(xe, ye);
            g.dot(xe, Y);
            g.texSmall(xe + 4, ye - 4, fet ? "#S" : "#E", tex::LEFT);
            g.res(xl, yt, xl, Y, pick(g, TG_RC, TG_RL));
            g.texSmall(177, Y + 13, fet ? "R_D\\parallel R_L" : "R_C\\parallel R_L", tex::RIGHT, pick(g, TG_RC, TG_RL));
            g.dot(xl, Y);
            g.texSmall(xg + 4, yt - 4, fet ? "#D" : "#C", tex::LEFT);
            g.dot(xg, yt);
            g.tex(176, yt - 4, "v_s", tex::RIGHT);
        } else {
            // collecteur / drain commun : sortie sur l'emetteur
            const int xe = 112, xg = 112, xo = 136, xl = 162;
            if (!fet) {
                g.res(xbn, yt, xe, yt, TG_Q);
                g.tex((xbn + xe) / 2, yt - 8, "r_\\pi", tex::CENTER, TG_Q);
            } else {
                g.texSmall((xbn + xe) / 2, yt + 14, "v_{gs}", tex::CENTER);
            }
            g.wire(xe, yt, xl, yt);
            g.isrc(xg, Y, xg, yt, TG_Q);
            g.texSmall(xg - 12, 112, gmLab, tex::RIGHT, TG_Q);
            if (hasRo) {
                g.res(xo, yt, xo, Y, TG_Q);
                g.texSmall(xo + 6, 100, roLab, tex::LEFT, TG_Q);
                g.dot(xo, yt);
                g.dot(xo, Y);
            }
            g.dot(xg, Y);
            g.res(xl, yt, xl, Y, pick(g, TG_RE1, TG_RL));
            g.texSmall(177, Y + 13, fet ? "R_S\\parallel R_L" : "R_E\\parallel R_L", tex::RIGHT, pick(g, TG_RE1, TG_RL));
            g.dot(xl, Y);
            g.dot(xe, yt);
            g.texSmall(xe + 3, yt - 4, fet ? "#S" : "#E", tex::LEFT);
            g.tex(176, yt - 4, "v_s", tex::RIGHT);
        }
    } else {
        // base / grille commune : entree sur l'emetteur
        const int xe = 46, xr = 72, x1 = 92, x2 = 128, xl = 156, yc = yt, ys = yt + 30;
        g.wire(34, yt, xe, yt);
        g.res(xe, yt, xe, Y, TG_RE1);
        g.texSmall(xe - 4, Y + 13, fet ? "R_S" : "R_E", tex::CENTER, TG_RE1);
        g.dot(xe, yt);
        g.dot(xe, Y);
        g.texSmall(xe - 3, yt - 4, fet ? "#S" : "#E", tex::RIGHT);
        if (!fet) {
            g.wire(xe, yt, xr, yt);
            g.res(xr, yt, xr, Y, TG_Q);
            g.tex(xr + 7, 104, "r_\\pi", tex::LEFT, TG_Q);
            g.dot(xr, Y);
            g.texSmall(xr + 4, Y - 4, "#B", tex::LEFT);
        }
        g.wire(fet ? xe : xr, yt, x1, yt);
        g.isrc(x2, yc, x1, yc, TG_Q);
        g.texSmall((x1 + x2) / 2, yc - 12, gmLab, tex::CENTER, TG_Q);
        if (hasRo) {
            g.wire(x1, yc, x1, ys);
            g.wire(x2, yc, x2, ys);
            g.res(x1, ys, x2, ys, TG_Q);
            g.texSmall((x1 + x2) / 2, ys + 16, roLab, tex::CENTER, TG_Q);
            g.dot(x1, yc);
            g.dot(x2, yc);
        }
        g.wire(x2, yc, 170, yc);
        g.res(xl, yc, xl, Y, pick(g, TG_RC, TG_RL));
        g.texSmall(177, Y + 13, fet ? "R_D\\parallel R_L" : "R_C\\parallel R_L", tex::RIGHT, pick(g, TG_RC, TG_RL));
        g.dot(xl, yc);
        g.dot(xl, Y);
        g.texSmall(x2 + 4, yc + 13, fet ? "#D" : "#C", tex::LEFT);
        g.tex(176, yc - 4, "v_s", tex::RIGHT);
    }
    // resultats
    char buf[48];
    int n = scopy(buf, sizeof buf, "A_v = ");
    fmtValue(buf + n, (int)sizeof(buf) - n, s.v[OAV], U_NONE, 3);
    font::drawRich(*g.cv, SANS_12, g.ox + 4, g.oy + 14, buf, g.volt);
    n = scopy(buf, sizeof buf, "Z_e = ");
    fmtValue(buf + n, (int)sizeof(buf) - n, s.v[OZE], U_OHM, 3);
    font::drawRich(*g.cv, SANS_12, g.ox + 90, g.oy + 14, buf, g.volt);
}

static void schOf(const Kind &k, Sch &g, const CalcState &s)
{
    if (s.c[CVIEW]) drawSmall(g, k, s);
    else drawMontage(g, k, s);
}

#define AMP_FUNCS(NAME, TOPO, FET)                                                              \
    namespace NAME {                                                                            \
    static const Kind K = {TOPO, FET};                                                          \
    static void def(CalcState &s) { defaults(K, s); }                                           \
    static void rows(const CalcState &s, RowList &r) { rowsOf(K, s, r); }                      \
    static void calc(CalcState &s) { calcOf(K, s); }                                            \
    static void sch(Sch &g, const CalcState &s) { schOf(K, g, s); }                            \
    static bool tf(const CalcState &s, double f, Cx &H) { return tfOf(K, s, f, H); }           \
    }
AMP_FUNCS(ce, T_CE, false)
AMP_FUNCS(cc, T_CC, false)
AMP_FUNCS(cb, T_CB, false)
AMP_FUNCS(sc, T_CE, true)
AMP_FUNCS(dc, T_CC, true)
AMP_FUNCS(gc, T_CB, true)

}  // namespace amp

// ==========================================================================
// Paire differentielle
// ==========================================================================
namespace diff {
enum { VCC, VEE, RC, REE, I0, R0, RE, BETA, VBE, VT, OIC, OVC, OVCE, OGM, ORPI, OAD, OAD1, OAMC, OTRMC, OZD, OZMC };
enum { CTAIL };
enum { T_RC = 1, T_REE, T_RE, T_Q, T_U };
static void def(CalcState &s)
{
    s.v[VCC] = 12; s.v[VEE] = 12; s.v[RC] = 10e3; s.v[REE] = 10e3; s.v[I0] = 1e-3; s.v[R0] = 1e6; s.v[RE] = 0;
    s.v[BETA] = 200; s.v[VBE] = 0.65; s.v[VT] = 26e-3;
}
static void rows(const CalcState &s, RowList &r)
{
    r.in("V_{CC}", VCC, U_V, ST_LIN1, T_U, "Alimentation positive");
    r.in("V_{EE}", VEE, U_V, ST_LIN1, T_U, "Alimentation négative −V_EE (valeur positive)");
    r.in("R_C", RC, U_OHM, ST_E24, T_RC, "Résistances de collecteur (identiques)");
    r.in("R_E", RE, U_OHM, ST_E24, T_RE, "Résistances d'émetteur (dégénérescence, 0 possible)", RF_NONNEG);
    r.choice("#Queue", CTAIL, "résistance R_EE|source de courant", T_REE, "Polarisation des émetteurs");
    if (s.c[CTAIL] == 0) r.in("R_{EE}", REE, U_OHM, ST_E24, T_REE, "Résistance commune vers −V_EE");
    else {
        r.in("I_0", I0, U_A, ST_125, T_REE, "Courant de la source (2 I_E)");
        r.in("R_0", R0, U_OHM, ST_E24, T_REE, "Résistance interne de la source de courant");
    }
    r.in("\\beta", BETA, U_NONE, ST_LIN10, T_Q, "Gain en courant des transistors");
    r.in("V_{BE}", VBE, U_V, ST_LIN01, T_Q, "Tension base-émetteur en conduction");
    r.in("V_T", VT, U_V, ST_NONE, 0, "Tension thermique (26 mV)");
    r.head("Point de repos (bases à 0 V)");
    r.out("I_C", OIC, U_A, "Courant de chaque collecteur", false, T_RC);
    r.out("V_C", OVC, U_V, "Potentiel de chaque collecteur");
    r.out("V_{CE}", OVCE, U_V, "Tension collecteur-émetteur");
    r.out("g_m", OGM, U_SIE, "Transconductance I_C / V_T");
    r.out("r_\\pi", ORPI, U_OHM, "β / g_m");
    r.head("Gains et impédances");
    r.out("A_d", OAD, U_NONE, "Différentiel, sortie différentielle (v_s1 − v_s2)/(v_e1 − v_e2)", true);
    r.out("A_{d1}", OAD1, U_NONE, "Différentiel, sortie sur un collecteur v_s1/v_d = A_d / 2");
    r.out("A_{mc}", OAMC, U_NONE, "Mode commun, sortie sur un collecteur");
    r.out("TRMC", OTRMC, U_DB, "Taux de réjection du mode commun 20 log |A_d1/A_mc|", true);
    r.out("Z_d", OZD, U_OHM, "Impédance d'entrée différentielle 2(r_π + (β+1)R_E)");
    r.out("Z_{mc}", OZMC, U_OHM, "Impédance d'entrée de mode commun (deux bases reliées)");
    r.msg(0);
}
static void calc(CalcState &s)
{
    double beta = s.v[BETA], re = s.v[RE];
    double ie, rt;
    if (s.c[CTAIL] == 0) {
        ie = (s.v[VEE] - s.v[VBE]) / (2 * s.v[REE] + re);
        rt = s.v[REE];
    } else {
        ie = s.v[I0] / 2;
        rt = s.v[R0];
    }
    if (!(ie > 0)) {
        for (int k = OIC; k <= OZMC; k++) s.v[k] = NaN();
        setMsg(s, 0, "Bloqué : V_EE < V_BE", ML_BAD);
        return;
    }
    double ic = beta / (beta + 1) * ie;
    s.v[OIC] = ic;
    s.v[OVC] = s.v[VCC] - s.v[RC] * ic;
    s.v[OVCE] = s.v[OVC] + s.v[VBE] + ie * re;
    double gm = ic / s.v[VT], rpi = beta / gm;
    s.v[OGM] = gm;
    s.v[ORPI] = rpi;
    double zd1 = rpi + (beta + 1) * re;
    s.v[OAD] = -beta * s.v[RC] / zd1;
    s.v[OAD1] = s.v[OAD] / 2;
    double zc1 = rpi + (beta + 1) * (re + 2 * rt);
    s.v[OAMC] = -beta * s.v[RC] / zc1;
    s.v[OTRMC] = db(fm::fabs(s.v[OAD1] / s.v[OAMC]));
    s.v[OZD] = 2 * zd1;
    s.v[OZMC] = zc1 / 2;
    if (s.v[OVCE] < 0.3) setMsg(s, 0, "Saturé : V_CE trop faible", ML_BAD);
    else setMsg(s, 0, "Régime linéaire", ML_OK);
}
static void sch(Sch &g, const CalcState &s)
{
    const int yt = 16, by = 84, x1 = 50, x2 = 128, yn = 136;
    // transistors : Q1 base a gauche ; Q2 dessine en miroir (base a droite)
    g.wire(x1 + 17, yt, x2 - 17, yt, T_U);
    g.tex(89, yt - 4 + 0, "V_{CC}", tex::CENTER, T_U);
    g.res(x1 + 17, yt, x1 + 17, by - 20, T_RC);
    g.res(x2 - 17, yt, x2 - 17, by - 20, T_RC);
    g.tex(x1 + 9, 44, "R_C", tex::RIGHT, T_RC);
    g.tex(x2 - 9, 44, "R_C", tex::LEFT, T_RC);
    g.bjt(x1, by, true, T_Q);
    // Q2 : symetrique (dessin a la main)
    {
        uint16_t c = g.col(T_Q);
        int X = g.ox + x2, Y = g.oy + by;
        g.cv->disc(X - 12, Y, 11, g.on(T_Q) ? g.light : g.paper);
        g.cv->circle(X - 12, Y, 11, c);
        g.cv->fillRect(X - 8, Y, 9, 1, c);
        g.cv->fillRect(X - 9, Y - 7, 2, 15, c);
        g.cv->line(X - 10, Y - 4, X - 17, Y - 11, c);
        g.cv->line(X - 17, Y - 11, X - 17, Y - 20, c);
        g.cv->line(X - 10, Y + 4, X - 17, Y + 11, c);
        g.cv->line(X - 17, Y + 11, X - 17, Y + 20, c);
        g.cv->arrowHead(X - 17, Y + 11, -1, 1, 5, c);
    }
    int ye = by + 20;
    if (s.v[RE] > 0) {
        g.res(x1 + 17, ye, x1 + 17, 124, T_RE);
        g.res(x2 - 17, ye, x2 - 17, 124, T_RE);
        g.texSmall(x1 + 11, 118, "R_E", tex::RIGHT, T_RE);
        g.texSmall(x2 - 11, 118, "R_E", tex::LEFT, T_RE);
        g.wire(x1 + 17, 124, x1 + 17, yn);
        g.wire(x2 - 17, 124, x2 - 17, yn);
    } else {
        g.wire(x1 + 17, ye, x1 + 17, yn);
        g.wire(x2 - 17, ye, x2 - 17, yn);
    }
    g.wire(x1 + 17, yn, x2 - 17, yn);
    g.dot(89, yn);
    if (s.c[CTAIL] == 0) g.res(89, yn, 89, 166, T_REE);
    else g.isrc(89, yn, 89, 166, T_REE);
    g.tex(97, 156, s.c[CTAIL] == 0 ? "R_{EE}" : "I_0", tex::LEFT, T_REE);
    g.rect(82, 166, 15, 2, g.col(T_U));
    g.texSmall(76, 168, "#−V_{EE}", tex::RIGHT, T_U);
    // entrees et sorties
    g.wire(14, by, x1, by);
    g.term(14, by);
    g.tex(12, by - 6, "v_{e1}", tex::LEFT);
    g.wire(x2, by, 165, by);
    g.term(165, by);
    g.tex(167, by - 6, "v_{e2}", tex::RIGHT);
    g.wire(x1 + 17, 58, 30, 58);
    g.term(30, 58);
    g.tex(28, 52, "v_{s1}", tex::RIGHT);
    g.wire(x2 - 17, 58, 148, 58);
    g.term(148, 58);
    g.tex(150, 52, "v_{s2}", tex::LEFT);
    g.dot(x1 + 17, 58);
    g.dot(x2 - 17, 58);
    char buf[40];
    int n = scopy(buf, sizeof buf, "A_d = ");
    fmtValue(buf + n, (int)sizeof(buf) - n, s.v[OAD], U_NONE, 3);
    font::drawRich(*g.cv, SANS_12, g.ox + 4, g.oy + 118, buf, g.volt);
}
}  // namespace diff

// ==========================================================================
// Parametres hybrides et modele en pi
// ==========================================================================
namespace hyb {
enum { IC, BETA, VA, VT, FT, CMU, OGM, ORPI, ORE, ORO, OH11, OH21, OH22, OMU, OCPI, OFB };
enum { T_Q = 1 };
static void def(CalcState &s) { s.v[IC] = 1e-3; s.v[BETA] = 200; s.v[VA] = 100; s.v[VT] = 26e-3; s.v[FT] = 300e6; s.v[CMU] = 3e-12; }
static void rows(const CalcState &, RowList &r)
{
    r.in("I_C", IC, U_A, ST_125, T_Q, "Courant de collecteur au point de repos");
    r.in("\\beta", BETA, U_NONE, ST_LIN10, T_Q, "Gain en courant h₂₁ₑ");
    r.in("V_A", VA, U_V, ST_LIN10, T_Q, "Tension d'Early (0 : ignorée)", RF_NONNEG);
    r.in("V_T", VT, U_V, ST_NONE, 0, "Tension thermique kT/q");
    r.in("f_T", FT, U_HZ, ST_125, T_Q, "Fréquence de transition (|β(f)| = 1), 0 : ignorée", RF_NONNEG);
    r.in("C_\\mu", CMU, U_F, ST_E12, T_Q, "Capacité base-collecteur (C_ob)", RF_NONNEG);
    r.head("Modèle en π");
    r.out("g_m", OGM, U_SIE, "I_C / V_T", true);
    r.out("r_\\pi", ORPI, U_OHM, "β / g_m", true);
    r.out("r_e", ORE, U_OHM, "Résistance dynamique d'émetteur V_T / I_E = α / g_m");
    r.out("r_o", ORO, U_OHM, "V_A / I_C");
    r.out("\\mu", OMU, U_NONE, "Gain intrinsèque g_m r_o = V_A / V_T");
    r.out("C_\\pi", OCPI, U_F, "g_m / (2π f_T) − C_μ");
    r.out("f_\\beta", OFB, U_HZ, "Coupure du gain en courant f_T / β");
    r.head("Paramètres hybrides (EC)");
    r.out("h_{11e}", OH11, U_OHM, "Impédance d'entrée = r_π");
    r.out("h_{21e}", OH21, U_NONE, "Gain en courant = β");
    r.out("h_{22e}", OH22, U_SIE, "Admittance de sortie = 1 / r_o");
}
static void calc(CalcState &s)
{
    double gm = s.v[IC] / s.v[VT];
    s.v[OGM] = gm;
    s.v[ORPI] = s.v[BETA] / gm;
    s.v[ORE] = s.v[BETA] / (s.v[BETA] + 1) / gm;
    s.v[ORO] = s.v[VA] > 0 ? s.v[VA] / s.v[IC] : fm::inf();
    s.v[OMU] = s.v[VA] > 0 ? s.v[VA] / s.v[VT] : fm::inf();
    s.v[OCPI] = s.v[FT] > 0 ? gm / (fm::TWO_PI * s.v[FT]) - s.v[CMU] : NaN();
    s.v[OFB] = s.v[FT] > 0 ? s.v[FT] / s.v[BETA] : NaN();
    s.v[OH11] = s.v[ORPI];
    s.v[OH21] = s.v[BETA];
    s.v[OH22] = s.v[VA] > 0 ? s.v[IC] / s.v[VA] : 0;
}
static void sch(Sch &g, const CalcState &s)
{
    // modele en pi complet entre B, C et E
    const int yt = 52, Y = 130, xb = 20, xr = 50, xc = 62, xg = 100, xo = 132, xs = 164;
    g.wire(xb, yt, xr, yt);
    g.term(xb, yt);
    g.texSmall(xb, yt - 6, "#B", tex::CENTER);
    g.res(xr, yt, xr, Y, T_Q);
    g.tex(xr - 7, 96, "r_\\pi", tex::RIGHT, T_Q);
    g.cap(xc + 8, yt, xc + 8, Y, T_Q);
    g.texSmall(xc + 15, 108, "C_\\pi", tex::LEFT, T_Q);
    g.wire(xr, yt, xc + 8, yt);
    g.wire(xr, Y, xs, Y);
    g.dot(xr, yt);
    g.dot(xc + 8, yt);
    // C_mu entre B et C (par le haut)
    g.wire(xc + 8, yt, xc + 8, 24);
    g.cap(xc + 8, 24, xo, 24, T_Q);
    g.texSmall((xc + 8 + xo) / 2, 16, "C_\\mu", tex::CENTER, T_Q);
    g.wire(xo, 24, xo, yt);
    g.isrc(xg, yt, xg, Y, T_Q);
    g.texSmall(xg + 11, 100, "g_mv_{be}", tex::LEFT, T_Q);
    g.wire(xg, yt, xs, yt);
    g.res(xo + 10, yt, xo + 10, Y, T_Q);
    g.texSmall(xo + 16, 76, "r_o", tex::LEFT, T_Q);
    g.dot(xg, yt);
    g.dot(xo, yt);
    g.dot(xo + 10, yt);
    g.dot(xg, Y);
    g.dot(xr, Y);
    g.dot(xc + 8, Y);
    g.dot(xo + 10, Y);
    g.term(xs, yt);
    g.texSmall(xs, yt - 6, "#C", tex::CENTER);
    g.term(xs, Y);
    g.texSmall(xs, Y + 13, "#E", tex::CENTER);
    g.wire(xs, Y, xs, Y);
    char buf[40];
    int n = scopy(buf, sizeof buf, "g_m = ");
    fmtValue(buf + n, (int)sizeof(buf) - n, s.v[OGM], U_SIE, 3);
    font::drawRich(*g.cv, SANS_12, g.ox + 8, g.oy + 150, buf, g.volt);
    n = scopy(buf, sizeof buf, "r_\xCF\x80 = ");
    fmtValue(buf + n, (int)sizeof(buf) - n, s.v[ORPI], U_OHM, 3);
    font::drawRich(*g.cv, SANS_12, g.ox + 8, g.oy + 165, buf, g.volt);
}
}  // namespace hyb

#define AMP_DEF(TITLE, SUB, TAGS, SEC, NS)                                                       \
    {TITLE, SUB, TAGS, SEC, G_BODE, 0, NS::def, NS::rows, NS::calc, NS::sch, NS::tf, amp::fcOf,   \
     nullptr, "Bode", 0, nullptr, nullptr, nullptr, 8}

extern const CalcDef g_calcAmpBjt[] = {
    AMP_DEF("Émetteur commun : petits signaux", "#A_v, Z_e, Z_s, f_b, f_h, Bode et schéma équivalent",
            "bipolaire bjt emetteur commun petits signaux dynamique gain impedance bande passante bode miller", 4, amp::ce),
    AMP_DEF("Collecteur commun : petits signaux", "#Émetteur suiveur : A_v ≈ 1, Z_e grande, Z_s faible",
            "bipolaire bjt collecteur commun emetteur suiveur petits signaux dynamique impedance", 4, amp::cc),
    AMP_DEF("Base commune : petits signaux", "#Z_e faible, gain élevé, large bande",
            "bipolaire bjt base commune petits signaux dynamique gain", 4, amp::cb),
    {"Paire différentielle", "#A_d, A_mc, TRMC, impédances d'entrée", "bipolaire paire differentielle mode commun trmc cmrr amplificateur", 4, G_NONE, 0,
     diff::def, diff::rows, diff::calc, diff::sch},
    {"Paramètres hybrides du bipolaire", "#g_m, r_π, r_o, h₁₁, h₂₁, h₂₂, C_π", "bipolaire parametres hybrides h11 h21 h22 modele pi gm transconductance early", 4, G_NONE, 0,
     hyb::def, hyb::rows, hyb::calc, hyb::sch},
};
extern const int g_calcAmpBjtN = sizeof(g_calcAmpBjt) / sizeof(g_calcAmpBjt[0]);

extern const CalcDef g_calcAmpFet[] = {
    AMP_DEF("Source commune : petits signaux", "#JFET ou MOSFET : A_v, Z_e, Z_s, Bode",
            "fet jfet mosfet source commune petits signaux dynamique gain impedance bode", 5, amp::sc),
    AMP_DEF("Drain commun : petits signaux", "#Source suiveuse : A_v ≈ 1, Z_s ≈ 1/g_m",
            "fet jfet mosfet drain commun source suiveuse petits signaux dynamique", 5, amp::dc),
    AMP_DEF("Grille commune : petits signaux", "#Z_e ≈ 1/g_m, gain non inverseur",
            "fet jfet mosfet grille commune petits signaux dynamique", 5, amp::gc),
};
extern const int g_calcAmpFetN = sizeof(g_calcAmpFet) / sizeof(g_calcAmpFet[0]);
