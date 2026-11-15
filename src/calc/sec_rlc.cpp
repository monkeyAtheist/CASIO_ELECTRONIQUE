// Section : circuits RC, RL, LC et RLC (regime sinusoidal et transitoire).
#include "CalcUtil.hpp"

using namespace cu;

static const char *const DESC_U = "Amplitude (ou valeur efficace) de la tension appliquée";

// Boucle serie : source a gauche (x = 30), composants en haut (y = 44) et a
// droite (x = 136), retour en bas (y = 132).
enum { LX = 30, RX = 136, TY = 44, BY = 132 };
static void loopFrame(Sch &g, int srcKind, int tagSrc, const char *srcLab)
{
    g.vsrc(LX, TY, BY, srcKind, tagSrc);
    g.varrow(14, BY - 14, TY + 14, srcLab, -1, tagSrc);
    g.wire(LX, BY, RX, BY);
}

// ==========================================================================
// Circuit RC serie
// ==========================================================================
namespace rc {
enum { R, C, F, U, OTAU, OFC, O5T, OX, OZ, OPHI, OI, OUR, OUC };
enum { T_R = 1, T_C, T_U };
static void def(CalcState &s) { s.v[R] = 1e3; s.v[C] = 100e-9; s.v[F] = 1e3; s.v[U] = 5; }
static void rows(const CalcState &, RowList &r)
{
    r.in("R", R, U_OHM, ST_E24, T_R);
    r.in("C", C, U_F, ST_E12, T_C);
    r.in("f", F, U_HZ, ST_125, 0, "Fréquence de la tension sinusoïdale");
    r.in("U", U, U_V, ST_LIN1, T_U, DESC_U);
    r.head("Résultats");
    r.out("\\tau", OTAU, U_S, "Constante de temps τ = RC", true);
    r.out("f_c", OFC, U_HZ, "Fréquence de coupure 1/(2π RC) : X_C = R", true);
    r.out("5\\tau", O5T, U_S, "Durée de charge à 99 %");
    r.out("X_C", OX, U_OHM, "Réactance du condensateur 1/(Cω) à f");
    r.out("|Z|", OZ, U_OHM, "Impédance du circuit série √(R² + X_C²)");
    r.out("\\varphi", OPHI, U_DEG, "Déphasage de la tension sur le courant (négatif : capacitif)");
    r.out("I", OI, U_A, "Courant I = U/|Z|");
    r.out("U_R", OUR, U_V, "Tension aux bornes de R", false, T_R);
    r.out("U_C", OUC, U_V, "Tension aux bornes de C", false, T_C);
}
static void calc(CalcState &s)
{
    double w = w2pi(s.v[F]);
    s.v[OTAU] = s.v[R] * s.v[C];
    s.v[OFC] = 1.0 / (fm::TWO_PI * s.v[OTAU]);
    s.v[O5T] = 5 * s.v[OTAU];
    s.v[OX] = 1.0 / (w * s.v[C]);
    s.v[OZ] = fm::hypot(s.v[R], s.v[OX]);
    s.v[OPHI] = -fm::deg(fm::atan2(s.v[OX], s.v[R]));
    s.v[OI] = s.v[U] / s.v[OZ];
    s.v[OUR] = s.v[R] * s.v[OI];
    s.v[OUC] = s.v[OX] * s.v[OI];
}
static void sch(Sch &g, const CalcState &)
{
    loopFrame(g, 1, T_U, "u");
    g.res(LX, TY, RX, TY, T_R);
    g.tex(83, TY - 11, "R", tex::CENTER, T_R);
    g.harrow(104, 62, TY + 14, "u_R", 1, T_R);
    g.cap(RX, TY, RX, BY, T_C);
    g.tex(RX - 10, 93, "C", tex::RIGHT, T_C);
    g.varrow(154, BY - 14, TY + 14, "u_C", 1, T_C);
    g.iarrow(RX, 70, 0, 1, "i", RX - 12, 74);
}
}  // namespace rc

// ==========================================================================
// Circuit RL serie
// ==========================================================================
namespace rl {
enum { R, L, F, U, OTAU, OFC, O5T, OX, OZ, OPHI, OI, OUR, OUL };
enum { T_R = 1, T_L, T_U };
static void def(CalcState &s) { s.v[R] = 100; s.v[L] = 10e-3; s.v[F] = 1e3; s.v[U] = 5; }
static void rows(const CalcState &, RowList &r)
{
    r.in("R", R, U_OHM, ST_E24, T_R);
    r.in("L", L, U_H, ST_E12, T_L);
    r.in("f", F, U_HZ, ST_125, 0, "Fréquence de la tension sinusoïdale");
    r.in("U", U, U_V, ST_LIN1, T_U, DESC_U);
    r.head("Résultats");
    r.out("\\tau", OTAU, U_S, "Constante de temps τ = L/R", true);
    r.out("f_c", OFC, U_HZ, "Fréquence de coupure R/(2π L) : X_L = R", true);
    r.out("5\\tau", O5T, U_S, "Établissement du courant à 99 %");
    r.out("X_L", OX, U_OHM, "Réactance de la bobine Lω à f");
    r.out("|Z|", OZ, U_OHM, "Impédance du circuit série √(R² + X_L²)");
    r.out("\\varphi", OPHI, U_DEG, "Déphasage de la tension sur le courant (positif : inductif)");
    r.out("I", OI, U_A, "Courant I = U/|Z|");
    r.out("U_R", OUR, U_V, "Tension aux bornes de R", false, T_R);
    r.out("U_L", OUL, U_V, "Tension aux bornes de L", false, T_L);
}
static void calc(CalcState &s)
{
    double w = w2pi(s.v[F]);
    s.v[OTAU] = s.v[L] / s.v[R];
    s.v[OFC] = s.v[R] / (fm::TWO_PI * s.v[L]);
    s.v[O5T] = 5 * s.v[OTAU];
    s.v[OX] = w * s.v[L];
    s.v[OZ] = fm::hypot(s.v[R], s.v[OX]);
    s.v[OPHI] = fm::deg(fm::atan2(s.v[OX], s.v[R]));
    s.v[OI] = s.v[U] / s.v[OZ];
    s.v[OUR] = s.v[R] * s.v[OI];
    s.v[OUL] = s.v[OX] * s.v[OI];
}
static void sch(Sch &g, const CalcState &)
{
    loopFrame(g, 1, T_U, "u");
    g.res(LX, TY, RX, TY, T_R);
    g.tex(83, TY - 11, "R", tex::CENTER, T_R);
    g.harrow(104, 62, TY + 14, "u_R", 1, T_R);
    g.ind(RX, TY, RX, BY, T_L);
    g.tex(RX - 8, 93, "L", tex::RIGHT, T_L);
    g.varrow(154, BY - 14, TY + 14, "u_L", 1, T_L);
    g.iarrow(RX, 70, 0, 1, "i", RX - 12, 74);
}
}  // namespace rl

// ==========================================================================
// Circuit LC : frequence de resonance
// ==========================================================================
namespace lc {
enum { L, C, F0, OW0, OT0, OZC };
enum { CMODE };
enum { T_L = 1, T_C };
static void def(CalcState &s) { s.v[L] = 10e-3; s.v[C] = 100e-9; s.v[F0] = 5e3; }
static void rows(const CalcState &s, RowList &r)
{
    r.choice("#Calculer", CMODE, "f₀|C|L", 0, "Grandeur à calculer à partir des deux autres");
    int m = s.c[CMODE];
    if (m != 2) r.in("L", L, U_H, ST_E12, T_L);
    if (m != 1) r.in("C", C, U_F, ST_E12, T_C);
    if (m != 0) r.in("f_0", F0, U_HZ, ST_125, 0, "Fréquence de résonance voulue");
    r.head("Résultats");
    if (m == 0) r.out("f_0", F0, U_HZ, "f₀ = 1/(2π √(LC))", true);
    if (m == 1) r.out("C", C, U_F, "C = 1/(4π² f₀² L)", true, T_C);
    if (m == 2) r.out("L", L, U_H, "L = 1/(4π² f₀² C)", true, T_L);
    r.out("\\omega_0", OW0, U_RADS, "Pulsation propre ω₀ = 1/√(LC)");
    r.out("T_0", OT0, U_S, "Période propre 2π √(LC)");
    r.out("\\sqrt{L/C}", OZC, U_OHM, "Impédance caractéristique : X_L = X_C à la résonance");
    r.msg(0);
}
static void calc(CalcState &s)
{
    int m = s.c[CMODE];
    double w0;
    if (m == 0) s.v[F0] = 1.0 / (fm::TWO_PI * fm::sqrt(s.v[L] * s.v[C]));
    w0 = w2pi(s.v[F0]);
    if (m == 1) s.v[C] = 1.0 / (w0 * w0 * s.v[L]);
    if (m == 2) s.v[L] = 1.0 / (w0 * w0 * s.v[C]);
    s.v[OW0] = w0;
    s.v[OT0] = 1.0 / s.v[F0];
    s.v[OZC] = fm::sqrt(s.v[L] / s.v[C]);
    if (m == 1) setMsg(s, 0, "E12 la plus proche : voir ← → sur C", ML_INFO);
    else setMsg(s, 0, "À f₀ : X_L = X_C (échanges d'énergie L ↔ C)", ML_INFO);
}
static void sch(Sch &g, const CalcState &)
{
    g.wire(30, 50, 150, 50);
    g.wire(30, 130, 150, 130);
    g.wire(30, 50, 30, 130);
    g.wire(150, 50, 150, 130);
    g.ind(66, 50, 66, 130, T_L);
    g.cap(116, 50, 116, 130, T_C);
    g.dot(66, 50);
    g.dot(66, 130);
    g.dot(116, 50);
    g.dot(116, 130);
    g.tex(58, 95, "L", tex::RIGHT, T_L);
    g.tex(127, 95, "C", tex::LEFT, T_C);
    g.tex(90, 160, "f_0=\\frac{1}{2\\pi\\sqrt{LC}}", tex::CENTER);
}
}  // namespace lc

// ==========================================================================
// RLC serie
// ==========================================================================
namespace rlcs {
enum { R, L, C, F, U, OF0, OQ, OXI, OB, OF1, OF2, OZ, OPHI, OI, OUC, OI0, OUC0 };
enum { T_R = 1, T_L, T_C, T_U };
static void def(CalcState &s) { s.v[R] = 47; s.v[L] = 10e-3; s.v[C] = 100e-9; s.v[F] = 5e3; s.v[U] = 1; }
static void rows(const CalcState &, RowList &r)
{
    r.in("R", R, U_OHM, ST_E24, T_R);
    r.in("L", L, U_H, ST_E12, T_L);
    r.in("C", C, U_F, ST_E12, T_C);
    r.in("f", F, U_HZ, ST_125, 0, "Fréquence de la tension sinusoïdale");
    r.in("U", U, U_V, ST_LIN1, T_U, DESC_U);
    r.head("Résonance");
    r.out("f_0", OF0, U_HZ, "f₀ = 1/(2π √(LC)) : Z = R minimale, courant maximal", true);
    r.out("Q", OQ, U_NONE, "Facteur de qualité Q = (1/R) √(L/C) = L ω₀/R", true);
    r.out("\\xi", OXI, U_NONE, "Coefficient d'amortissement ξ = 1/(2Q)");
    r.out("B", OB, U_HZ, "Bande passante à −3 dB : B = f₀/Q");
    r.out("f_1", OF1, U_HZ, "Fréquence de coupure basse");
    r.out("f_2", OF2, U_HZ, "Fréquence de coupure haute");
    r.out("I_0", OI0, U_A, "Courant à la résonance U/R");
    r.out("U_{C0}", OUC0, U_V, "Surtension aux bornes de C à f₀ : Q·U");
    r.msg(0);
    r.head("À la fréquence f");
    r.out("|Z|", OZ, U_OHM, "|Z| = √(R² + (Lω − 1/(Cω))²)");
    r.out("\\varphi", OPHI, U_DEG, "Déphasage de u sur i");
    r.out("I", OI, U_A, "Courant I = U/|Z|");
    r.out("U_C", OUC, U_V, "Tension aux bornes de C", false, T_C);
}
static Cx Z(const CalcState &s, double f)
{
    double w = w2pi(f);
    return cx(s.v[R], w * s.v[L] - 1.0 / (w * s.v[C]));
}
static void calc(CalcState &s)
{
    double r = s.v[R], l = s.v[L], c = s.v[C];
    s.v[OF0] = 1.0 / (fm::TWO_PI * fm::sqrt(l * c));
    double q = fm::sqrt(l / c) / r;
    s.v[OQ] = q;
    s.v[OXI] = 1 / (2 * q);
    s.v[OB] = s.v[OF0] / q;
    double k = fm::sqrt(1 + 1 / (4 * q * q));
    s.v[OF1] = s.v[OF0] * (k - 1 / (2 * q));
    s.v[OF2] = s.v[OF0] * (k + 1 / (2 * q));
    s.v[OI0] = s.v[U] / r;
    s.v[OUC0] = q * s.v[U];
    Cx z = Z(s, s.v[F]);
    s.v[OZ] = cabs(z);
    s.v[OPHI] = fm::deg(carg(z));
    s.v[OI] = s.v[U] / s.v[OZ];
    s.v[OUC] = s.v[OI] / (w2pi(s.v[F]) * c);
    if (q > 0.5 + 1e-9) setMsg(s, 0, "Régime libre pseudo-périodique (Q > 1/2)", ML_INFO);
    else if (q < 0.5 - 1e-9) setMsg(s, 0, "Régime libre apériodique (Q < 1/2)", ML_INFO);
    else setMsg(s, 0, "Régime libre critique (Q = 1/2)", ML_INFO);
}
static bool tf(const CalcState &s, double f, Cx &h) { h = cx(s.v[R]) / Z(s, f); return true; }
static double fc(const CalcState &s) { return 1.0 / (fm::TWO_PI * fm::sqrt(s.v[L] * s.v[C])); }
static void sch(Sch &g, const CalcState &)
{
    loopFrame(g, 1, T_U, "u");
    g.res(LX, TY, 82, TY, T_R);
    g.tex(56, TY - 11, "R", tex::CENTER, T_R);
    g.ind(82, TY, RX, TY, T_L);
    g.tex(109, TY - 10, "L", tex::CENTER, T_L);
    g.cap(RX, TY, RX, BY, T_C);
    g.tex(RX - 10, 93, "C", tex::RIGHT, T_C);
    g.varrow(154, BY - 14, TY + 14, "u_C", 1, T_C);
    g.iarrow(RX, 70, 0, 1, "i", RX - 12, 74);
}
}  // namespace rlcs

// ==========================================================================
// RLC parallele
// ==========================================================================
namespace rlcp {
enum { R, L, C, F, I, OF0, OQ, OB, OZ0, OZ, OPHI, OU };
enum { T_R = 1, T_L, T_C, T_I };
static void def(CalcState &s) { s.v[R] = 10e3; s.v[L] = 10e-3; s.v[C] = 100e-9; s.v[F] = 5e3; s.v[I] = 1e-3; }
static void rows(const CalcState &, RowList &r)
{
    r.in("R", R, U_OHM, ST_E24, T_R);
    r.in("L", L, U_H, ST_E12, T_L);
    r.in("C", C, U_F, ST_E12, T_C);
    r.in("f", F, U_HZ, ST_125, 0, "Fréquence du courant sinusoïdal");
    r.in("I", I, U_A, ST_125, T_I, "Courant fourni par la source");
    r.head("Résonance (anti-résonance)");
    r.out("f_0", OF0, U_HZ, "f₀ = 1/(2π √(LC)) : Z = R maximale", true);
    r.out("Q", OQ, U_NONE, "Facteur de qualité Q = R √(C/L) = R/(L ω₀)", true);
    r.out("B", OB, U_HZ, "Bande passante B = f₀/Q");
    r.out("Z_0", OZ0, U_OHM, "Impédance à la résonance : R");
    r.head("À la fréquence f");
    r.out("|Z|", OZ, U_OHM, "1/|Z| = √(1/R² + (Cω − 1/(Lω))²)");
    r.out("\\varphi", OPHI, U_DEG, "Déphasage de u sur i");
    r.out("U", OU, U_V, "Tension aux bornes : |Z|·I");
}
static Cx Z(const CalcState &s, double f)
{
    double w = w2pi(f);
    return 1.0 / cx(1.0 / s.v[R], w * s.v[C] - 1.0 / (w * s.v[L]));
}
static void calc(CalcState &s)
{
    s.v[OF0] = 1.0 / (fm::TWO_PI * fm::sqrt(s.v[L] * s.v[C]));
    s.v[OQ] = s.v[R] * fm::sqrt(s.v[C] / s.v[L]);
    s.v[OB] = s.v[OF0] / s.v[OQ];
    s.v[OZ0] = s.v[R];
    Cx z = Z(s, s.v[F]);
    s.v[OZ] = cabs(z);
    s.v[OPHI] = fm::deg(carg(z));
    s.v[OU] = s.v[OZ] * s.v[I];
}
static bool tf(const CalcState &s, double f, Cx &h) { h = Z(s, f) / s.v[R]; return true; }
static double fc(const CalcState &s) { return 1.0 / (fm::TWO_PI * fm::sqrt(s.v[L] * s.v[C])); }
static void sch(Sch &g, const CalcState &)
{
    g.vsrc(22, 44, 132, 2, T_I);
    g.tex(34, 98, "I", tex::LEFT, T_I);
    g.wire(22, 44, 146, 44);
    g.wire(22, 132, 146, 132);
    g.res(62, 44, 62, 132, T_R);
    g.ind(104, 44, 104, 132, T_L);
    g.cap(146, 44, 146, 132, T_C);
    g.dot(62, 44);
    g.dot(62, 132);
    g.dot(104, 44);
    g.dot(104, 132);
    g.tex(53, 93, "R", tex::RIGHT, T_R);
    g.tex(98, 93, "L", tex::RIGHT, T_L);
    g.tex(136, 93, "C", tex::RIGHT, T_C);
    g.varrow(164, 124, 52, "u", 1);
    g.tex(89, 160, "Q=R\\sqrt{C/L}", tex::CENTER);
}
}  // namespace rlcp

// ==========================================================================
// Charge et decharge d'un condensateur
// ==========================================================================
namespace charge {
enum { E, U0, R, C, T, UC, OTAU, O5T, OUT, OIT, OTC, OW };
enum { CMODE };
enum { T_E = 1, T_R, T_C };
static void def(CalcState &s) { s.v[E] = 12; s.v[U0] = 0; s.v[R] = 10e3; s.v[C] = 100e-6; s.v[T] = 1; s.v[UC] = 9; }
static double uf(const CalcState &s) { return s.c[CMODE] ? 0.0 : s.v[E]; }
static void rows(const CalcState &s, RowList &r)
{
    r.choice("#Phase", CMODE, "Charge vers E|Décharge vers 0", 0, "Charge sous la tension E ou décharge dans R");
    if (!s.c[CMODE]) r.in("E", E, U_V, ST_LIN1, T_E, "Tension du générateur", 0);
    r.in("U_0", U0, U_V, ST_LIN1, T_C, "Tension initiale du condensateur", 0);
    r.in("R", R, U_OHM, ST_E24, T_R);
    r.in("C", C, U_F, ST_E12, T_C);
    r.in("t", T, U_S, ST_125, 0, "Instant étudié", RF_NONNEG);
    r.in("U_{\\text{cible}}", UC, U_V, ST_LIN1, T_C, "Tension dont on cherche l'instant d'atteinte", 0);
    r.head("Résultats");
    r.out("\\tau", OTAU, U_S, "τ = RC : 63 % du chemin parcouru", true);
    r.out("5\\tau", O5T, U_S, "Charge ou décharge à 99 %");
    r.out("u_C(t)", OUT, U_V, "u(t) = U_f + (U₀ − U_f) e^(−t/τ)", true);
    r.out("i(t)", OIT, U_A, "Courant i = C du/dt");
    r.out("t_{\\text{cible}}", OTC, U_S, "t = τ ln((U₀ − U_f)/(U − U_f))", true);
    r.out("q(t)", OW, U_COUL, "Charge du condensateur q = C u(t)");
    r.msg(0);
}
static void calc(CalcState &s)
{
    double tau = s.v[R] * s.v[C], f = uf(s), u0 = s.v[U0];
    s.v[OTAU] = tau;
    s.v[O5T] = 5 * tau;
    double e = fm::exp(-s.v[T] / tau);
    s.v[OUT] = f + (u0 - f) * e;
    s.v[OIT] = (f - u0) / s.v[R] * e;
    s.v[OW] = s.v[C] * s.v[OUT];
    double k = (s.v[UC] - f) / (u0 - f);
    if (k > 0 && k <= 1) {
        s.v[OTC] = tau * fm::ln(1 / k);
        setMsg(s, 0, "Graphe : u_C(t) de 0 à 5τ", ML_INFO);
    } else {
        s.v[OTC] = NaN();
        setMsg(s, 0, "U cible hors du chemin U₀ → U_f", ML_WARN);
    }
}
static void sch(Sch &g, const CalcState &s)
{
    if (!s.c[CMODE]) {
        g.vsrc(LX, TY, BY, 0, T_E);
        g.varrow(14, BY - 14, TY + 14, "E", -1, T_E);
    } else {
        g.wire(LX, TY, LX, BY);
    }
    g.wire(LX, BY, RX, BY);
    g.sw(LX, TY, 72, 0);
    g.tex(51, TY - 12, "K", tex::CENTER);
    g.res(72, TY, RX, TY, T_R);
    g.tex(104, TY - 11, "R", tex::CENTER, T_R);
    g.cap(RX, TY, RX, BY, T_C);
    g.tex(RX - 10, 93, "C", tex::RIGHT, T_C);
    g.varrow(154, BY - 14, TY + 14, "u_C", 1, T_C);
    g.iarrow(RX, 70, 0, 1, "i", RX - 12, 74);
}
static void graph(Canvas &cv, const CalcState &s, const GraphArea &a, uint16_t accent)
{
    double tau = s.v[OTAU], f = uf(s), u0 = s.v[U0];
    double tmax = 5 * tau;
    if (ok(s.v[OTC]) && s.v[OTC] * 1.15 > tmax) tmax = s.v[OTC] * 1.15;
    if (s.v[T] * 1.1 > tmax && s.v[T] < 20 * tau) tmax = s.v[T] * 1.1;
    double lo = fm::fmin(0, fm::fmin(f, u0)), hi = fm::fmax(0, fm::fmax(f, u0));
    if (hi - lo < 1e-9) hi = lo + 1;
    double m = (hi - lo) * 0.1;
    lo -= m;
    hi += m;
    int X0 = a.x + 44, X1 = a.x + a.w - 10, Y0 = a.y + 12, Y1 = a.y + a.h - 24;
    auto px = [&](double t) { return X0 + fm::iround(fm::clampc((t / tmax) * (X1 - X0))); };
    auto pyf = [&](double v) { return fm::clampc(Y1 - (v - lo) / (hi - lo) * (Y1 - Y0)); };
    uint16_t axc = rgb(150, 156, 170), grid = rgb(226, 230, 238);
    // graduations en tau
    char buf[32];
    for (int k = 1; k * tau <= tmax * 1.0001; k++) {
        int x = px(k * tau);
        cv.fillRect(x, Y0, 1, Y1 - Y0, grid);
        int n = fmtNumber(buf, sizeof buf, k, 2);
        scopy(buf + n, (int)sizeof(buf) - n, "τ");
        font::drawText(cv, SANS_12, x - font::width(SANS_12, buf) / 2, Y1 + 14, buf, col::inkSoft);
    }
    // niveaux U0, Uf et 63 %
    double lv[3] = {u0, f, f + (u0 - f) * 0.36788};
    for (int i = 0; i < 3; i++) {
        int y = (int)pyf(lv[i]);
        for (int x = X0; x < X1; x += 3) cv.fillRect(x, y, 1, 1, axc);
        fmtValue(buf, sizeof buf, lv[i], U_V, 3);
        int w = font::width(SANS_12, buf);
        font::drawText(cv, SANS_12, X0 - 4 - w, y + 4, buf, i == 2 ? col::note : col::inkSoft);
    }
    cv.fillRect(X0, Y0, 1, Y1 - Y0 + 1, axc);
    cv.fillRect(X0, (int)pyf(0), X1 - X0, 1, axc);
    // courbe
    int prevX = 0, prevY = 0;
    for (int i = 0; i <= 200; i++) {
        double t = tmax * i / 200;
        double v = f + (u0 - f) * fm::exp(-t / tau);
        int x = (int)(fm::clampc(X0 + (t / tmax) * (X1 - X0)) * 16), y = (int)(pyf(v) * 16);
        if (i) {
            cv.lineSub(prevX, prevY, x, y, accent);
            cv.lineSub(prevX, prevY + 10, x, y + 10, accent);
        }
        prevX = x;
        prevY = y;
    }
    // point a l'instant t
    if (s.v[T] <= tmax) {
        int x = px(s.v[T]), y = (int)pyf(s.v[OUT]);
        cv.disc(x, y, 3, col::red);
        int n = scopy(buf, sizeof buf, "u(t) = ");
        fmtValue(buf + n, (int)sizeof(buf) - n, s.v[OUT], U_V);
        font::drawText(cv, BOLD_12, X1 - font::width(BOLD_12, buf), Y0 + 30, buf, col::red);
    }
    if (ok(s.v[OTC]) && s.v[OTC] <= tmax) {
        int x = px(s.v[OTC]);
        for (int y = Y0; y < Y1; y += 3) cv.fillRect(x, y, 1, 2, accent);
    }
    tex::draw(cv, "u_C", X0 + 6, Y0 + 10, col::inkSoft, tex::LEFT, 1);
}
}  // namespace charge

extern const CalcDef g_calcRlc[] = {
    {"Circuit RC", "\\tau=RC,\\ f_c=1/(2\\pi RC)", "rc constante temps coupure reactance condensateur", 1, G_NONE, 0,
     rc::def, rc::rows, rc::calc, rc::sch, nullptr, nullptr, nullptr, nullptr, 0},
    {"Circuit RL", "\\tau=L/R,\\ f_c=R/(2\\pi L)", "rl constante temps coupure reactance bobine", 1, G_NONE, 0,
     rl::def, rl::rows, rl::calc, rl::sch, nullptr, nullptr, nullptr, nullptr, 0},
    {"Circuit LC (résonance)", "f_0=1/(2\\pi\\sqrt{LC})", "lc resonance frequence propre accord", 1, G_NONE, 0,
     lc::def, lc::rows, lc::calc, lc::sch, nullptr, nullptr, nullptr, nullptr, 0},
    {"Circuit RLC série", "f_0,\\ Q,\\ B,\\ |Z|", "rlc serie resonance facteur qualite bande passante", 1, G_BODE, 0,
     rlcs::def, rlcs::rows, rlcs::calc, rlcs::sch, rlcs::tf, rlcs::fc, nullptr, "U_R/U", rlcs::F + 1},
    {"Circuit RLC parallèle", "f_0,\\ Q,\\ |Z|", "rlc parallele bouchon anti resonance", 1, G_BODE, 0,
     rlcp::def, rlcp::rows, rlcp::calc, rlcp::sch, rlcp::tf, rlcp::fc, nullptr, "Z/R", rlcp::F + 1},
    {"Charge et décharge RC", "u_C(t)=U_f+(U_0-U_f)\\,e^{-t/\\tau}", "charge decharge condensateur transitoire temps", 1, G_CUSTOM, 0,
     charge::def, charge::rows, charge::calc, charge::sch, nullptr, nullptr, charge::graph, "Courbe", 0},
};
extern const int g_calcRlcN = sizeof(g_calcRlc) / sizeof(g_calcRlc[0]);
