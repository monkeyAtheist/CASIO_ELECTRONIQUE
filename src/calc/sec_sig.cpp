// Section : traitement du signal (echantillonnage et repliement, CAN/CNA,
// acquisition et FFT, modulation d'amplitude, de frequence et de phase,
// modulations numeriques, valeurs moyenne et efficace, filtre numerique).
#include "CalcUtil.hpp"

using namespace cu;

namespace sig {

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

// erfc (approximation de Tchebychev, erreur relative < 1,2e-7).
static double erfc(double x)
{
    double z = fm::fabs(x), t = 1 / (1 + 0.5 * z);
    double r = t * fm::exp(-z * z - 1.26551223 +
                           t * (1.00002368 + t * (0.37409196 + t * (0.09678418 + t * (-0.18628806 + t * (0.27886807 +
                           t * (-1.13520398 + t * (1.48851587 + t * (-0.82215223 + t * 0.17087277)))))))));
    return x >= 0 ? r : 2 - r;
}
static double qfunc(double x) { return 0.5 * erfc(x / fm::sqrt(2.0)); }

// Fonctions de Bessel J_0..J_nmax(x) (recurrence descendante de Miller).
static void besselJ(double x, int nmax, double *J)
{
    for (int n = 0; n <= nmax; n++) J[n] = 0;
    if (x == 0) { J[0] = 1; return; }
    int m = 2 * ((int)(fm::fmax((double)nmax, x) + 15 + fm::sqrt(40 * fm::fmax((double)nmax, x))) / 2);
    if (m > 1600) m = 1600;
    double jp1 = 0, jn = 1e-30, sum = 0;
    for (int k = m; k >= 1; k--) {
        double jm1 = 2 * k / x * jn - jp1;
        if (k - 1 <= nmax) J[k - 1] = jm1;
        if (k <= nmax) J[k] = jn;
        if ((k - 1) % 2 == 0 && k - 1 > 0) sum += 2 * jm1;
        jp1 = jn;
        jn = jm1;
        if (fm::fabs(jn) > 1e200) {   // renormalisation
            for (int i = 0; i <= nmax; i++) J[i] *= 1e-200;
            jp1 *= 1e-200; jn *= 1e-200; sum *= 1e-200;
        }
    }
    double j0 = jn;
    sum += j0;
    for (int n = 0; n <= nmax; n++) J[n] /= sum;
}

static double sinc(double u)
{
    if (fm::fabs(u) < 1e-12) return 1;
    double a = fm::PI * u;
    return fm::sin(a) / a;
}

}  // namespace sig

using namespace sig;

// ==========================================================================
// Echantillonnage et repliement
// ==========================================================================
namespace samp {
enum { FS, F, ONYQ, OFA, OSPP, OK };
enum { T_FS = 1, T_F };
static void def(CalcState &s) { s.v[FS] = 1e3; s.v[F] = 900; }
static double alias(double f, double fs)
{
    double k = fm::floor(f / fs + 0.5);
    return fm::fabs(f - k * fs);
}
static void rows(const CalcState &, RowList &r)
{
    r.in("f_e", FS, U_HZ, ST_125, T_FS, "Fréquence d'échantillonnage");
    r.in("f", F, U_HZ, ST_125, T_F, "Fréquence du signal sinusoïdal", RF_NONNEG);
    r.head("Résultats");
    r.out("f_e/2", ONYQ, U_HZ, "Fréquence de Nyquist (maximum utilisable)");
    r.out("f_a", OFA, U_HZ, "Fréquence apparente après échantillonnage |f − k f_e|", true);
    r.out("k", OK, U_NONE, "Multiple de f_e le plus proche de f");
    r.out("f_e/f", OSPP, U_NONE, "Échantillons par période du signal (≥ 2 exigé)");
    r.msg(0);
}
static void calc(CalcState &s)
{
    s.v[ONYQ] = s.v[FS] / 2;
    s.v[OFA] = alias(s.v[F], s.v[FS]);
    s.v[OK] = fm::floor(s.v[F] / s.v[FS] + 0.5);
    s.v[OSPP] = s.v[F] > 0 ? s.v[FS] / s.v[F] : fm::inf();
    if (s.v[F] < s.v[ONYQ]) setMsg(s, 0, "Shannon respecté : f < f_e/2, pas de repliement", ML_OK);
    else if (s.v[F] == s.v[ONYQ]) setMsg(s, 0, "f = f_e/2 : cas limite (phase critique)", ML_WARN);
    else setMsg(s, 0, "Repliement : le signal apparaît à f_a (F4 : chronogramme)", ML_BAD);
}
static void spectrum(Canvas &cv, int X0, int X1, int Y, int H, const CalcState &s, uint16_t accent, uint16_t ink)
{
    double fs = s.v[FS], f = s.v[F];
    double fmax = fm::fmax(2 * fs, 1.15 * f);
    auto xOf = [&](double v) { return X0 + (int)(v / fmax * (X1 - X0)); };
    cv.fillRect(X0, Y, X1 - X0, 1, ink);
    int xn = xOf(fs / 2);
    for (int y = Y - H; y < Y; y += 3) cv.fillRect(xn, y, 1, 1, rgb(160, 166, 180));
    if (!(fs > 0) || !(fmax > 0) || !fm::isfinite(fmax)) return;
    for (int k = 1; k * fs <= fmax && k < 300; k++) {
        int x = xOf(k * fs);
        cv.fillRect(x, Y - 4, 1, 8, ink);
    }
    // images k f_e +/- f
    for (int k = 0; k * fs <= fmax + f && k < 300; k++)
        for (int sg = -1; sg <= 1; sg += 2) {
            double v = k * fs + sg * f;
            if (v <= 0 || v > fmax) continue;
            int x = xOf(v);
            cv.fillRect(x, Y - H / 2, 1, H / 2, rgb(190, 196, 208));
        }
    int xf = xOf(f), xa = xOf(alias(f, fs));
    cv.fillRect(xf - 1, Y - H, 3, H, f < fs / 2 ? accent : rgb(200, 40, 50));
    cv.fillRect(xa - 1, Y - H, 3, H, accent);
    cv.disc(xa, Y - H, 3, accent);
    font::drawRich(cv, SANS_12, xn - 10, Y + 14, "f_e/2", ink);
    font::drawRich(cv, SANS_12, xOf(fs) - 6, Y + 14, "f_e", ink);
}
static void sch(Sch &g, const CalcState &s)
{
    spectrum(*g.cv, g.ox + 10, g.ox + 170, g.oy + 80, 50, s, g.accent, g.ink);
    font::drawText(*g.cv, SANS_12, g.ox + 10, g.oy + 18, "spectre échantillonné", g.label);
    valText(g, 89, 132, "f_a", s.v[OFA], U_HZ, 1, true);
    valText(g, 89, 150, "f_e/2", s.v[ONYQ], U_HZ, 1);
}
static void graph(Canvas &cv, const CalcState &s, const GraphArea &a, uint16_t accent)
{
    double fs = s.v[FS], f = s.v[F], fa = alias(f, fs);
    const int ns = 24;
    double T = ns / fs;
    const int X0 = a.x + 12, X1 = a.x + a.w - 12, Y = a.y + 62, A = 40;
    cv.fillRect(X0, Y, X1 - X0, 1, rgb(170, 176, 190));
    // signal original (gris)
    int npts = 1200;
    int px = 0, py = 0;
    for (int i = 0; i <= npts; i++) {
        double t = T * i / npts;
        int x = X0 + (int)((t / T) * (X1 - X0));
        int y = Y - (int)(A * fm::sin(fm::TWO_PI * f * t));
        if (i) cv.line(px, py, x, y, rgb(185, 190, 202));
        px = x; py = y;
    }
    // sinusoide apparente : passe par les echantillons (signe selon le repliement)
    double k = fm::floor(f / fs + 0.5);
    double sgn = f - k * fs >= 0 ? 1 : -1;
    for (int i = 0; i <= 400; i++) {
        double t = T * i / 400;
        int x = X0 + (int)((t / T) * (X1 - X0));
        int y = Y - (int)(A * sgn * fm::sin(fm::TWO_PI * fa * t));
        if (i) { cv.line(px, py, x, y, accent); cv.line(px, py + 1, x, y + 1, accent); }
        px = x; py = y;
    }
    for (int n = 0; n <= ns; n++) {
        double t = n / fs;
        int x = X0 + (int)((t / T) * (X1 - X0));
        int y = Y - (int)(A * fm::sin(fm::TWO_PI * f * t));
        cv.fillRect(x, y < Y ? y : Y, 1, y < Y ? Y - y : y - Y, rgb(200, 40, 50));
        cv.disc(x, y, 3, rgb(200, 40, 50));
    }
    font::drawText(cv, SANS_12, X0, a.y + 14, "signal (gris), échantillons (rouge), signal apparent", col::inkSoft);
    spectrum(cv, X0 + 10, X1 - 10, a.y + a.h - 24, 44, s, accent, col::ink);
}
}  // namespace samp

// ==========================================================================
// Convertisseurs analogique-numerique et numerique-analogique
// ==========================================================================
namespace adc {
enum { N, VMIN, VMAX, VIN, CODE, FS, OQ, OCODE, OV, OERR, OEMAX, ONOISE, OSNR, ODR, ORATE };
enum { CDIR, CROUND };
enum { T_IN = 1, T_REF };
static char s_code[160];
static void def(CalcState &s) { s.v[N] = 10; s.v[VMIN] = 0; s.v[VMAX] = 5; s.v[VIN] = 3.3; s.v[CODE] = 512; s.v[FS] = 10e3; }
static void rows(const CalcState &s, RowList &r)
{
    bool cna = s.c[CDIR] == 1;
    r.choice("#Sens", CDIR, "CAN : tension → code|CNA : code → tension", 0);
    r.in("n", N, U_NONE, ST_INT, 0, "Résolution en bits", RF_POS | RF_INT);
    r.in("V_{ref-}", VMIN, U_V, ST_LIN01, T_REF, "Tension du code 0 (souvent 0 V)", 0);
    r.in("V_{ref+}", VMAX, U_V, ST_LIN01, T_REF, "Tension de pleine échelle", 0);
    if (!cna) {
        r.choice("#Quantification", CROUND, "troncature|arrondi", 0, "Code inférieur ou code le plus proche");
        r.in("V_e", VIN, U_V, ST_LIN01, T_IN, "Tension à convertir", 0);
    } else {
        r.in("N", CODE, U_NONE, ST_INT, T_IN, "Code numérique d'entrée", RF_NONNEG | RF_INT);
    }
    r.in("f_e", FS, U_HZ, ST_125, 0, "Fréquence d'échantillonnage (débit binaire)");
    r.head("Résultats");
    r.out("q", OQ, U_V, "Quantum (LSB) (V_ref+ − V_ref−)/2ⁿ", true);
    if (!cna) {
        r.outI("N", OCODE, "Code de sortie", true, T_IN);
        r.texOut("", 0, "Code en hexadécimal et en binaire", T_IN);
        r.out("V_N", OV, U_V, "Tension correspondant au code V_ref− + N q");
        r.out("\\varepsilon", OERR, U_V, "Erreur de quantification V_e − V_N");
    } else {
        r.texOut("", 0, "Code en hexadécimal et en binaire", T_IN);
        r.out("V_s", OV, U_V, "Tension de sortie V_ref− + N q", true);
    }
    r.out("\\varepsilon_{\\max}", OEMAX, U_V, "Erreur maximale : q (troncature) ou q/2 (arrondi)");
    r.out("\\sigma_q", ONOISE, U_V, "Bruit de quantification efficace q/√12");
    r.out("SNR", OSNR, U_DB, "Rapport signal/bruit idéal 6,02 n + 1,76 dB (sinus pleine échelle)");
    r.out("DR", ODR, U_DB, "Dynamique 20 log(2ⁿ)");
    r.out("D", ORATE, U_BPS, "Débit binaire n f_e");
    r.msg(1);
}
static void calc(CalcState &s)
{
    int n = (int)s.v[N];
    if (n > 32) { n = 32; s.v[N] = 32; }
    double full = 1;
    for (int k = 0; k < n; k++) full *= 2;
    double span = s.v[VMAX] - s.v[VMIN], q = span / full;
    s.v[OQ] = q;
    bool cna = s.c[CDIR] == 1;
    double code;
    if (!cna) {
        double x = (s.v[VIN] - s.v[VMIN]) / q;
        code = s.c[CROUND] ? fm::floor(x + 0.5) : fm::floor(x);
    } else code = s.v[CODE];
    bool clip = false;
    if (code < 0) { code = 0; clip = true; }
    if (code > full - 1) { code = full - 1; clip = true; }
    s.v[OCODE] = code;
    s.v[OV] = s.v[VMIN] + code * q;
    s.v[OERR] = s.v[VIN] - s.v[OV];
    s.v[OEMAX] = s.c[CROUND] && !cna ? q / 2 : q;
    s.v[ONOISE] = q / fm::sqrt(12.0);
    s.v[OSNR] = 6.0206 * n + 1.7609;
    s.v[ODR] = 20 * fm::log10(full);
    s.v[ORATE] = n * s.v[FS];
    // code en texte
    uint32_t c = (uint32_t)(long long)code;
    int k = scopy(s_code, sizeof s_code, "\\text{0x");
    int hd = (n + 3) / 4;
    for (int i = hd - 1; i >= 0; i--) s_code[k++] = "0123456789ABCDEF"[(c >> (4 * i)) & 15];
    k += scopy(s_code + k, (int)sizeof(s_code) - k, "}\\;\\;\\text{");
    for (int i = n - 1; i >= 0 && k < (int)sizeof(s_code) - 14; i--) {
        s_code[k++] = (char)('0' + ((c >> i) & 1));
        if (i && i % 4 == 0) k += scopy(s_code + k, (int)sizeof(s_code) - k, "}\\,\\text{");
    }
    s_code[k++] = '}';
    s_code[k] = 0;
    setMsg(s, 0, s_code, ML_INFO);
    if (!(span > 0)) setMsg(s, 1, "Il faut V_ref+ > V_ref−", ML_BAD);
    else if (clip) setMsg(s, 1, cna ? "Code hors limites (0 à 2ⁿ − 1)" : "Entrée hors plage : code saturé", ML_WARN);
    else setMsg(s, 1, "", ML_INFO);
}
static void sch(Sch &g, const CalcState &s)
{
    // caracteristique en escalier (3 bits si n > 4)
    int n = (int)s.v[N];
    int nb = n > 4 ? 3 : n;
    int steps = 1 << nb;
    const int X0 = 30, X1 = 168, Y0 = 18, Y1 = 128;
    g.line(X0, Y1, X1, Y1, g.ink);
    g.line(X0, Y0, X0, Y1, g.ink);
    double sw = (double)(X1 - X0 - 6) / steps, sh = (double)(Y1 - Y0 - 6) / steps;
    bool rnd = s.c[CROUND] && s.c[CDIR] == 0;
    int px = X0, py = Y1;
    for (int k = 0; k < steps; k++) {
        double x0 = X0 + (k - (rnd ? 0.5 : 0)) * sw, x1 = x0 + sw;
        if (x0 < X0) x0 = X0;
        int y = Y1 - (int)(k * sh);
        g.line(px, py, (int)x0, y, g.accent);
        g.line((int)x0, y, (int)x1, y, g.accent);
        px = (int)x1;
        py = y;
    }
    // droite ideale
    g.line(X0, Y1, X0 + (int)(steps * sw), Y1 - (int)(steps * sh), g.label);
    g.texSmall(X1, Y1 - 4, "V_e", tex::RIGHT, T_IN);
    g.texSmall(X0 - 3, Y0 + 6, "N", tex::RIGHT);
    if (n > 4) font::drawText(*g.cv, SANS_12, g.ox + 36, g.oy + 30, "(dessin sur 3 bits)", g.label);
    else {
        double frac = (s.v[OCODE]) / (double)steps;
        int y = Y1 - (int)(frac * steps * sh);
        g.line(X0 - 4, y, X0 + 4, y, g.volt);
    }
    valText(g, 6, 150, "q", s.v[OQ], U_V, 0, true);
    char buf[32];
    int k = scopy(buf, sizeof buf, "N = ");
    fmtInt(buf + k, (int)sizeof(buf) - k, s.v[OCODE]);
    font::drawText(*g.cv, BOLD_12, g.ox + 177 - font::width(BOLD_12, buf), g.oy + 150, buf, g.volt);
    valText(g, 6, 168, "SNR", s.v[OSNR], U_DB);
}
}  // namespace adc

// ==========================================================================
// Acquisition et FFT (fenetres de ponderation)
// ==========================================================================
namespace fft {
enum { FS, NPTS, F, BYTES, OT, ODF, ONYQ, OBIN, OBINN, OFBIN, OENBW, OENBWHZ, OCG, OSCAL, OW3, ONULL, OSIDE, OMEM };
enum { CWIN };
enum { T_W = 1 };
static const char *const WINS = "rectangulaire|Hann|Hamming|Blackman|Blackman-Harris|à toit plat";
static const double WC[6][5] = {
    {1, 0, 0, 0, 0}, {0.5, 0.5, 0, 0, 0}, {0.54, 0.46, 0, 0, 0}, {0.42, 0.5, 0.08, 0, 0},
    {0.35875, 0.48829, 0.14128, 0.01168, 0}, {0.21557895, 0.41663158, 0.277263158, 0.083578947, 0.006947368},
};
static double W(int w, double u)
{
    double v = WC[w][0] * sinc(u);
    for (int k = 1; k < 5; k++) if (WC[w][k] != 0) v += WC[w][k] / 2 * (sinc(u - k) + sinc(u + k));
    return v;
}
static void def(CalcState &s) { s.v[FS] = 48e3; s.v[NPTS] = 1024; s.v[F] = 1e3; s.v[BYTES] = 2; s.c[CWIN] = 1; }
static void rows(const CalcState &, RowList &r)
{
    r.in("f_e", FS, U_HZ, ST_125, 0, "Fréquence d'échantillonnage");
    r.in("N", NPTS, U_NONE, ST_POW2, 0, "Nombre de points de la FFT", RF_POS | RF_INT);
    r.in("f", F, U_HZ, ST_125, 0, "Fréquence d'une raie à analyser", RF_NONNEG);
    r.choice("#Fenêtre", CWIN, WINS, T_W, "Fenêtre de pondération");
    r.in("#octets", BYTES, U_NONE, ST_INT, 0, "Octets par échantillon (mémoire)", RF_POS | RF_INT);
    r.head("Acquisition");
    r.out("T_a", OT, U_S, "Durée d'acquisition N/f_e", true);
    r.out("\\Delta f", ODF, U_HZ, "Résolution fréquentielle f_e/N", true);
    r.out("f_e/2", ONYQ, U_HZ, "Fréquence maximale analysable");
    r.out("k", OBIN, U_NONE, "Position de la raie en canaux f/Δf");
    r.outI("k_0", OBINN, "Canal le plus proche");
    r.out("f_{k_0}", OFBIN, U_HZ, "Fréquence de ce canal");
    r.out("#mémoire", OMEM, U_BYTE, "Taille du bloc N × octets");
    r.head("Fenêtre");
    r.out("ENBW", OENBW, U_NONE, "Bande équivalente de bruit (en canaux)", false, T_W);
    r.out("ENBW", OENBWHZ, U_HZ, "Bande équivalente de bruit en Hz", false, T_W);
    r.out("G_c", OCG, U_NONE, "Gain cohérent (moyenne de la fenêtre)", false, T_W);
    r.out("#festonnage", OSCAL, U_DB, "Perte maximale entre deux canaux (raie à mi-canal)", false, T_W);
    r.out("B_{-3dB}", OW3, U_NONE, "Largeur du lobe principal à −3 dB (canaux)", false, T_W);
    r.out("#1er zéro", ONULL, U_NONE, "Demi-largeur du lobe principal (canaux)", false, T_W);
    r.out("#lobe secondaire", OSIDE, U_DB, "Niveau du plus haut lobe secondaire", true, T_W);
    r.msg(0);
}
static void calc(CalcState &s)
{
    double fs = s.v[FS], n = s.v[NPTS];
    s.v[OT] = n / fs;
    s.v[ODF] = fs / n;
    s.v[ONYQ] = fs / 2;
    s.v[OBIN] = s.v[F] / s.v[ODF];
    s.v[OBINN] = fm::floor(s.v[OBIN] + 0.5);
    s.v[OFBIN] = s.v[OBINN] * s.v[ODF];
    s.v[OMEM] = n * s.v[BYTES];
    int w = s.c[CWIN];
    double a0 = WC[w][0], sq = a0 * a0;
    for (int k = 1; k < 5; k++) sq += WC[w][k] * WC[w][k] / 2;
    s.v[OENBW] = sq / (a0 * a0);
    s.v[OENBWHZ] = s.v[OENBW] * s.v[ODF];
    s.v[OCG] = a0;
    double w0 = W(w, 0);
    s.v[OSCAL] = -20 * fm::log10(fm::fabs(W(w, 0.5)) / w0);
    // -3 dB
    double lo = 0, hi = 3;
    for (int it = 0; it < 60; it++) {
        double md = 0.5 * (lo + hi);
        if (fm::fabs(W(w, md)) / w0 > 1 / fm::sqrt(2.0)) lo = md;
        else hi = md;
    }
    s.v[OW3] = 2 * lo;
    // premier zero : le lobe principal s'annule au canal (nombre de termes)
    int nt = 1;
    for (int k = 1; k < 5; k++) if (WC[w][k] != 0) nt = k + 1;
    s.v[ONULL] = nt;
    // plus haut lobe secondaire
    double mx = 0, um = nt;
    for (double u = nt; u <= nt + 20; u += 0.02) {
        double v = fm::fabs(W(w, u));
        if (v > mx) { mx = v; um = u; }
    }
    for (double u = um - 0.02; u <= um + 0.02; u += 0.0005) {
        double v = fm::fabs(W(w, u));
        if (v > mx) mx = v;
    }
    s.v[OSIDE] = 20 * fm::log10(mx / w0);
    if (s.v[F] >= s.v[ONYQ]) setMsg(s, 0, "f ≥ f_e/2 : repliement (filtrer avant le CAN)", ML_BAD);
    else if (fm::fabs(s.v[OBIN] - s.v[OBINN]) > 1e-9) setMsg(s, 0, "Raie entre deux canaux : fuite spectrale (F4)", ML_WARN);
    else setMsg(s, 0, "Raie centrée sur un canal (cohérence)", ML_OK);
}
static void sch(Sch &g, const CalcState &s)
{
    // forme de la fenetre
    const int X0 = 14, X1 = 166, Y0 = 26, Y1 = 110;
    int w = s.c[CWIN];
    g.line(X0, Y1, X1, Y1, g.ink);
    int px = 0, py = 0;
    for (int i = 0; i <= 120; i++) {
        double t = i / 120.0;
        double v = WC[w][0];
        for (int k = 1; k < 5; k++) v += (k & 1 ? -1 : 1) * WC[w][k] * fm::cos(fm::TWO_PI * k * t);
        int x = X0 + (int)(t * (X1 - X0)), y = Y1 - (int)(v * (Y1 - Y0));
        if (i) { g.line(px, py, x, y, g.col(T_W)); g.line(px, py + 1, x, y + 1, g.col(T_W)); }
        px = x; py = y;
    }
    g.texSmall(X0, Y0 - 6, "w(n)", tex::LEFT, T_W);
    g.texSmall(X0, Y1 + 12, "0", tex::LEFT);
    g.texSmall(X1, Y1 + 12, "N-1", tex::RIGHT);
    valText(g, 6, 136, "\xCE\x94" "f", s.v[ODF], U_HZ, 0, true);
    valText(g, 6, 151, "T_a", s.v[OT], U_S, 0, true);
    valText(g, 6, 166, "lobe", s.v[OSIDE], U_DB, 0);
}
static void graph(Canvas &cv, const CalcState &s, const GraphArea &a, uint16_t accent)
{
    // module du spectre de la fenetre (dB) autour d'une raie
    int w = s.c[CWIN];
    double w0 = W(w, 0);
    const int X0 = a.x + 40, X1 = a.x + a.w - 10, Y0 = a.y + 22, Y1 = a.y + a.h - 26;
    const double U = 12, bot = -120;
    uint16_t grid = rgb(226, 230, 238);
    char buf[16];
    for (int d = 0; d >= bot; d -= 20) {
        int y = Y0 + (int)(-d / -bot * (Y1 - Y0));
        cv.fillRect(X0, y, X1 - X0, 1, grid);
        fmtNumber(buf, sizeof buf, d, 3);
        font::drawText(cv, SANS_12, X0 - 4 - font::width(SANS_12, buf), y + 4, buf, col::inkSoft);
    }
    for (int k = -12; k <= 12; k += 2) {
        int x = X0 + (int)((k + U) / (2 * U) * (X1 - X0));
        cv.fillRect(x, Y0, 1, Y1 - Y0, grid);
        fmtNumber(buf, sizeof buf, k, 3);
        font::drawText(cv, SANS_12, x - font::width(SANS_12, buf) / 2, Y1 + 14, buf, col::inkSoft);
    }
    int px = 0, py = 0;
    for (int i = 0; i <= 700; i++) {
        double u = -U + 2 * U * i / 700.0;
        double v = fm::fabs(W(w, u)) / w0;
        double d = v > 1e-6 ? 20 * fm::log10(v) : bot;
        if (d < bot) d = bot;
        int x = X0 + (int)((u + U) / (2 * U) * (X1 - X0)), y = Y0 + (int)(-d / -bot * (Y1 - Y0));
        if (i) cv.line(px, py, x, y, accent);
        px = x; py = y;
    }
    font::drawText(cv, SANS_12, X0 + 4, Y0 + 12, "dB", col::inkSoft);
    font::drawText(cv, SANS_12, X1 - 60, Y0 + 12, "canaux", col::inkSoft);
}
}  // namespace fft

// ==========================================================================
// Modulation d'amplitude
// ==========================================================================
namespace am {
enum { FC, FM, VC, M, R, RD, OVMAX, OVMIN, OB, OVSB, OPC, OPSB, OPT, OETA, OTC, ORCMAX, OCMAX, OCMIN };
enum { CTYPE };
enum { T_C = 1, T_M };
static void def(CalcState &s) { s.v[FC] = 100e3; s.v[FM] = 5e3; s.v[VC] = 1; s.v[M] = 0.6; s.v[R] = 50; s.v[RD] = 10e3; }
static void rows(const CalcState &s, RowList &r)
{
    int t = s.c[CTYPE];
    r.choice("#Modulation", CTYPE, "AM (avec porteuse)|DBSP (porteuse supprimée)|BLU (bande latérale unique)", 0);
    r.in("f_p", FC, U_HZ, ST_125, T_C, "Fréquence de la porteuse");
    r.in("f_m", FM, U_HZ, ST_125, T_M, "Fréquence du signal modulant (sinus)");
    r.in("V_p", VC, U_V, ST_LIN01, T_C, "Amplitude de la porteuse non modulée");
    r.in("m", M, U_NONE, ST_NONE, T_M, "Indice de modulation (0 à 1 en AM)", RF_NONNEG);
    r.in("R", R, U_OHM, ST_E24, 0, "Charge (calcul des puissances)");
    r.head("Signal modulé");
    if (t == 0) {
        r.out("V_{\\max}", OVMAX, U_V, "V_p (1 + m)");
        r.out("V_{\\min}", OVMIN, U_V, "V_p (1 − m)");
    }
    r.out("B", OB, U_HZ, t == 2 ? "Encombrement f_m" : "Encombrement spectral 2 f_m", true);
    r.out("V_{BL}", OVSB, U_V, "Amplitude d'une raie latérale m V_p/2");
    if (t == 0) r.out("P_p", OPC, U_W, "Puissance de la porteuse V_p²/(2R)");
    r.out("P_{BL}", OPSB, U_W, "Puissance d'une bande latérale m² V_p²/(8R)");
    r.out("P_t", OPT, U_W, "Puissance totale", true);
    r.out("\\eta", OETA, U_PCT, "Part de la puissance portant l'information");
    if (t == 0) {
        r.head("Détecteur d'enveloppe");
        r.in("R_d", RD, U_OHM, ST_E24, 0, "Résistance du détecteur");
        r.out("T_p", OTC, U_S, "Période de la porteuse : il faut R C ≫ T_p");
        r.out("RC_{\\max}", ORCMAX, U_S, "√(1 − m²)/(2π f_m m) : au-delà, l'enveloppe décroche");
        r.out("C_{\\min}", OCMIN, U_F, "≈ 10/(2π f_p R_d) : ondulation faible");
        r.out("C_{\\max}", OCMAX, U_F, "RC_max/R_d", true);
    }
    r.msg(0);
}
static void calc(CalcState &s)
{
    int t = s.c[CTYPE];
    double vc = s.v[VC], m = s.v[M], r = s.v[R];
    s.v[OVMAX] = vc * (1 + m);
    s.v[OVMIN] = vc * (1 - m);
    s.v[OB] = t == 2 ? s.v[FM] : 2 * s.v[FM];
    s.v[OVSB] = m * vc / 2;
    s.v[OPC] = t == 0 ? vc * vc / (2 * r) : 0;
    s.v[OPSB] = m * m * vc * vc / (8 * r);
    s.v[OPT] = s.v[OPC] + (t == 2 ? 1 : 2) * s.v[OPSB];
    s.v[OETA] = (s.v[OPT] - s.v[OPC]) / s.v[OPT] * 100;
    s.v[OTC] = 1 / s.v[FC];
    s.v[ORCMAX] = m > 0 && m < 1 ? fm::sqrt(1 - m * m) / (fm::TWO_PI * s.v[FM] * m) : NaN();
    s.v[OCMAX] = s.v[ORCMAX] / s.v[RD];
    s.v[OCMIN] = 10 / (fm::TWO_PI * s.v[FC] * s.v[RD]);
    if (t == 0 && m > 1) setMsg(s, 0, "Surmodulation (m > 1) : enveloppe déformée", ML_BAD);
    else if (t == 0 && s.v[OCMIN] > s.v[OCMAX]) setMsg(s, 0, "f_p trop proche de f_m : détecteur impossible", ML_WARN);
    else setMsg(s, 0, "F4 : signal modulé et spectre", ML_INFO);
}
static void lines(Canvas &cv, int X0, int X1, int Y, int H, const CalcState &s, uint16_t accent, uint16_t ink)
{
    int t = s.c[CTYPE];
    double vc = s.v[VC], m = s.v[M];
    double amax = vc > m * vc / 2 ? vc : m * vc / 2;
    int xc = (X0 + X1) / 2, dx = (X1 - X0) / 4;
    cv.fillRect(X0, Y, X1 - X0, 1, ink);
    auto line = [&](int x, double a, uint16_t c) {
        int h = (int)(a / amax * H);
        cv.fillRect(x - 1, Y - h, 3, h, c);
        cv.disc(x, Y - h, 2, c);
    };
    if (t == 0) line(xc, vc, ink);
    else for (int y = Y - 10; y < Y; y += 3) cv.fillRect(xc, y, 1, 1, ink);
    if (t != 2) line(xc - dx, m * vc / 2, accent);
    line(xc + dx, m * vc / 2, accent);
    font::drawRich(cv, SANS_12, xc - 6, Y + 14, "f_p", ink);
    font::drawRich(cv, SANS_12, xc - dx - 18, Y + 14, "f_p−f_m", accent);
    font::drawRich(cv, SANS_12, xc + dx - 18, Y + 14, "f_p+f_m", accent);
}
static void sch(Sch &g, const CalcState &s)
{
    font::drawText(*g.cv, SANS_12, g.ox + 8, g.oy + 18, "spectre", g.label);
    lines(*g.cv, g.ox + 8, g.ox + 172, g.oy + 104, 70, s, g.accent, g.ink);
    valText(g, 6, 148, "B", s.v[OB], U_HZ, 0, true);
    valText(g, 177, 148, "P_t", s.v[OPT], U_W, 2, true);
    valText(g, 6, 166, "\xCE\xB7", s.v[OETA], U_PCT);
}
static void graph(Canvas &cv, const CalcState &s, const GraphArea &a, uint16_t accent)
{
    int t = s.c[CTYPE];
    double vc = s.v[VC], m = s.v[M], fc = s.v[FC], fmd = s.v[FM];
    double ratio = fc / fmd;
    bool dil = ratio > 30;
    double fcd = dil ? 30 * fmd : fc;
    double T = 2 / fmd;
    const int X0 = a.x + 10, X1 = a.x + a.w - 10, Y = a.y + 64, A = 22;
    double amax = vc * (1 + m);
    if (t == 2) amax = m * vc / 2;
    if (!(amax > 0)) return;
    cv.fillRect(X0, Y, X1 - X0, 1, rgb(170, 176, 190));
    int px = 0, py = 0;
    for (int i = 0; i <= 900; i++) {
        double tt = T * i / 900, wm = fm::TWO_PI * fmd * tt, wc = fm::TWO_PI * fcd * tt;
        double v;
        if (t == 0) v = vc * (1 + m * fm::cos(wm)) * fm::cos(wc);
        else if (t == 1) v = vc * m * fm::cos(wm) * fm::cos(wc);
        else v = m * vc / 2 * fm::cos(wc + wm);
        int x = X0 + (int)((tt / T) * (X1 - X0)), y = Y - (int)(v / amax * A * 1.8);
        if (i) cv.line(px, py, x, y, accent);
        px = x; py = y;
    }
    if (t != 2) {
        for (int sg = -1; sg <= 1; sg += 2) {
            for (int i = 0; i <= 200; i++) {
                double tt = T * i / 200;
                double env = t == 0 ? vc * (1 + m * fm::cos(fm::TWO_PI * fmd * tt)) : vc * m * fm::cos(fm::TWO_PI * fmd * tt);
                int x = X0 + (int)((tt / T) * (X1 - X0)), y = Y - (int)(sg * env / amax * A * 1.8);
                if (i) cv.line(px, py, x, y, rgb(200, 40, 50));
                px = x; py = y;
            }
        }
    }
    if (dil) font::drawText(cv, SANS_12, X0, a.y + 14, "porteuse dessinée à 30 f_m (échelle dilatée)", col::inkSoft);
    lines(cv, X0 + 40, X1 - 40, a.y + a.h - 22, 46, s, accent, col::ink);
}
}  // namespace am

// ==========================================================================
// Modulation de frequence et de phase
// ==========================================================================
namespace fmm {
enum { NJ = 600 };
static const double BMAX = 500;
enum { FC, FMOD, DF, BETA, OBETA, ODF, OBC, ONS, OB1, OJ0, OJ1, OJ2, OJ3, OPC };
enum { CTYPE };
enum { T_M = 1 };
static void def(CalcState &s) { s.v[FC] = 100e6; s.v[FMOD] = 15e3; s.v[DF] = 75e3; s.v[BETA] = 2; }
static double betaOf(const CalcState &s) { return s.c[CTYPE] == 0 ? s.v[DF] / s.v[FMOD] : s.v[BETA]; }
static void rows(const CalcState &s, RowList &r)
{
    bool fmm = s.c[CTYPE] == 0;
    r.choice("#Modulation", CTYPE, "fréquence (FM)|phase (PM)", 0);
    r.in("f_p", FC, U_HZ, ST_125, 0, "Fréquence de la porteuse");
    r.in("f_m", FMOD, U_HZ, ST_125, T_M, "Fréquence du signal modulant (sinus)");
    if (fmm) r.in("\\Delta f", DF, U_HZ, ST_125, T_M, "Excursion de fréquence (75 kHz en radio FM)", RF_NONNEG);
    else r.in("\\Delta\\varphi", BETA, U_NONE, ST_NONE, T_M, "Excursion de phase en radians (indice β)", RF_NONNEG);
    r.head("Résultats");
    r.out("\\beta", OBETA, U_NONE, fmm ? "Indice de modulation Δf/f_m" : "Indice de modulation = Δφ", true);
    if (!fmm) r.out("\\Delta f", ODF, U_HZ, "Excursion de fréquence équivalente β f_m");
    r.out("B_C", OBC, U_HZ, "Bande de Carson 2(Δf + f_m) = 2 f_m(β + 1)", true);
    r.out("n", ONS, U_NONE, "Paires de raies significatives (|J_n| ≥ 1 %)");
    r.out("B_{1\\%}", OB1, U_HZ, "Encombrement à 1 % : 2 n f_m");
    r.out("J_0", OJ0, U_NONE, "Amplitude relative de la porteuse J₀(β)");
    r.out("J_1", OJ1, U_NONE, "Raies f_p ± f_m");
    r.out("J_2", OJ2, U_NONE, "Raies f_p ± 2 f_m");
    r.out("J_3", OJ3, U_NONE, "Raies f_p ± 3 f_m");
    r.out("P_p/P_t", OPC, U_PCT, "Part de puissance restant sur la porteuse J₀²");
    r.msg(0);
}
static void calc(CalcState &s)
{
    double b = betaOf(s);
    s.v[OBETA] = b;
    s.v[ODF] = b * s.v[FMOD];
    s.v[OBC] = 2 * s.v[FMOD] * (b + 1);
    static double J[NJ + 1];
    if (b > BMAX) {
        for (int k = OBC + 1; k <= OPC; k++) s.v[k] = NaN();
        setMsg(s, 0, "β > 500 : raies non calculées (Carson reste valable)", ML_WARN);
        return;
    }
    besselJ(b, NJ, J);
    int n = 0;
    for (int k = 1; k <= NJ; k++) if (fm::fabs(J[k]) >= 0.01) n = k;
    s.v[ONS] = n;
    s.v[OB1] = 2 * n * s.v[FMOD];
    s.v[OJ0] = J[0]; s.v[OJ1] = J[1]; s.v[OJ2] = J[2]; s.v[OJ3] = J[3];
    s.v[OPC] = J[0] * J[0] * 100;
    if (b < 0.3) setMsg(s, 0, "β < 0,3 : bande étroite (comme l'AM, B ≈ 2 f_m)", ML_INFO);
    else if (fm::fabs(J[0]) < 0.02) setMsg(s, 0, "Porteuse presque éteinte (zéro de J₀ : β ≈ 2,405…)", ML_INFO);
    else setMsg(s, 0, "F4 : spectre de raies (fonctions de Bessel)", ML_INFO);
}
static void spectrumBessel(Canvas &cv, int X0, int X1, int Y, int H, double b, uint16_t accent, uint16_t ink, int nmax)
{
    static double J[NJ + 1];
    if (!(b <= BMAX)) return;
    besselJ(b, NJ, J);
    int n = 0;
    for (int k = 1; k <= NJ; k++) if (fm::fabs(J[k]) >= 0.01) n = k;
    int N = n + 2;
    if (N > nmax) N = nmax;
    cv.fillRect(X0, Y, X1 - X0, 1, ink);
    int xc = (X0 + X1) / 2;
    double dx = (double)(X1 - X0 - 8) / (2 * N);
    for (int k = -N; k <= N; k++) {
        double a = fm::fabs(J[k < 0 ? -k : k]);
        int x = xc + (int)(k * dx);
        int h = (int)(a * H);
        uint16_t c = k == 0 ? ink : accent;
        if (h > 0) cv.fillRect(x - 1, Y - h, dx > 6 ? 3 : 1, h, c);
    }
    font::drawRich(cv, SANS_12, xc - 6, Y + 14, "f_p", ink);
}
static void sch(Sch &g, const CalcState &s)
{
    font::drawRich(*g.cv, SANS_12, g.ox + 8, g.oy + 18, "raies |J_n(\xCE\xB2)|", g.label);
    spectrumBessel(*g.cv, g.ox + 8, g.ox + 172, g.oy + 110, 80, s.v[OBETA], g.accent, g.ink, 20);
    valText(g, 6, 148, "\xCE\xB2", s.v[OBETA], U_NONE, 0, true);
    valText(g, 177, 148, "B_C", s.v[OBC], U_HZ, 2, true);
}
static void graph(Canvas &cv, const CalcState &s, const GraphArea &a, uint16_t accent)
{
    double b = s.v[OBETA];
    const int X0 = a.x + 10, X1 = a.x + a.w - 10, Y = a.y + 56, A = 28;
    double fmd = s.v[FMOD], fcd = fmd * 12;
    double T = 2 / fmd;
    cv.fillRect(X0, Y, X1 - X0, 1, rgb(170, 176, 190));
    int px = 0, py = 0;
    for (int i = 0; i <= 900; i++) {
        double t = T * i / 900;
        double v = fm::cos(fm::TWO_PI * fcd * t + b * fm::sin(fm::TWO_PI * fmd * t));
        int x = X0 + (int)((t / T) * (X1 - X0)), y = Y - (int)(v * A);
        if (i) cv.line(px, py, x, y, accent);
        px = x; py = y;
    }
    font::drawText(cv, SANS_12, X0, a.y + 14, "signal (porteuse dessinée à 12 f_m)", col::inkSoft);
    spectrumBessel(cv, X0 + 20, X1 - 20, a.y + a.h - 22, 70, b, accent, col::ink, 40);
}
}  // namespace fmm

// ==========================================================================
// Modulations numeriques
// ==========================================================================
namespace dig {
enum { D, ALPHA, EBN0, OM, OK, ORS, OB, OEFF, OSNR, OBER, OEB6 };
enum { CTYPE };
enum { T_C = 1 };
static const char *const TYPES = "OOK (tout ou rien)|BPSK|QPSK|8-PSK|16-QAM|64-QAM|256-QAM|FSK 2 (non cohérente)";
static const int MS[8] = {2, 2, 4, 8, 16, 64, 256, 2};
static void def(CalcState &s) { s.v[D] = 1e6; s.v[ALPHA] = 0.35; s.v[EBN0] = 10; s.c[CTYPE] = 2; }
static double ber(int t, double ebn0db)
{
    double g = fm::pow10(ebn0db / 10);
    int M = MS[t];
    double k = fm::ln((double)M) / fm::ln(2.0);
    switch (t) {
    case 0: return qfunc(fm::sqrt(g));
    case 1: case 2: return qfunc(fm::sqrt(2 * g));
    case 3: return 2 / k * qfunc(fm::sqrt(2 * k * g) * fm::sin(fm::PI / M));
    case 7: return 0.5 * fm::exp(-g / 2);
    default: {
        double sm = fm::sqrt((double)M);
        double p = 4 / k * (1 - 1 / sm) * qfunc(fm::sqrt(3 * k * g / (M - 1)));
        return p > 0.5 ? 0.5 : p;
    }
    }
}
static void rows(const CalcState &, RowList &r)
{
    r.choice("#Modulation", CTYPE, TYPES, T_C);
    r.in("D", D, U_BPS, ST_125, 0, "Débit binaire");
    r.in("\\alpha", ALPHA, U_NONE, ST_NONE, 0, "Facteur de retombée du filtre de Nyquist (0 à 1)", RF_NONNEG);
    r.in("E_b/N_0", EBN0, U_DB, ST_LIN1, 0, "Énergie par bit sur densité de bruit", 0);
    r.head("Résultats");
    r.out("M", OM, U_NONE, "Nombre d'états (symboles)");
    r.out("k", OK, U_NONE, "Bits par symbole log₂ M");
    r.out("R", ORS, U_BAUD, "Rapidité de modulation D/k", true);
    r.out("B", OB, U_HZ, "Bande occupée R (1 + α)", true);
    r.out("\\eta", OEFF, U_NONE, "Efficacité spectrale D/B (bit/s/Hz)");
    r.out("SNR", OSNR, U_DB, "Rapport signal/bruit E_b/N_0 + 10 log(D/B)");
    r.out("TEB", OBER, U_NONE, "Taux d'erreur binaire théorique (codage de Gray)", true);
    r.out("E_b/N_0\\ (10^{-6})", OEB6, U_DB, "E_b/N_0 nécessaire pour un TEB de 10⁻⁶");
}
static void calc(CalcState &s)
{
    int t = s.c[CTYPE];
    int M = MS[t];
    double k = fm::ln((double)M) / fm::ln(2.0);
    s.v[OM] = M;
    s.v[OK] = k;
    s.v[ORS] = s.v[D] / k;
    s.v[OB] = s.v[ORS] * (1 + s.v[ALPHA]);
    if (t == 7) s.v[OB] = s.v[ORS] * (1 + s.v[ALPHA]) + s.v[ORS];   // FSK : ecart de frequence ~ R
    s.v[OEFF] = s.v[D] / s.v[OB];
    s.v[OSNR] = s.v[EBN0] + 10 * fm::log10(s.v[D] / s.v[OB]);
    s.v[OBER] = ber(t, s.v[EBN0]);
    double lo = -10, hi = 60;
    for (int it = 0; it < 80; it++) {
        double md = 0.5 * (lo + hi);
        if (ber(t, md) > 1e-6) lo = md;
        else hi = md;
    }
    s.v[OEB6] = 0.5 * (lo + hi);
}
static void sch(Sch &g, const CalcState &s)
{
    int t = s.c[CTYPE];
    const int cx0 = 89, cy0 = 70, R = 52;
    uint16_t axc = mix16(g.paper, g.ink, 6);
    g.line(cx0 - R - 8, cy0, cx0 + R + 8, cy0, axc);
    g.line(cx0, cy0 - R - 8, cx0, cy0 + R + 8, axc);
    g.texSmall(cx0 + R + 6, cy0 - 4, "I", tex::RIGHT);
    g.texSmall(cx0 + 4, cy0 - R - 2, "Q", tex::LEFT);
    uint16_t c = g.col(T_C) == g.ink ? g.accent : g.accent;
    auto pt = [&](double x, double y, const char *lab) {
        int px = cx0 + fm::iround(x * R), py = cy0 - fm::iround(y * R);
        g.cv->disc(g.ox + px, g.oy + py, t >= 5 && t <= 6 ? 1 : 3, c);
        if (lab) font::drawText(*g.cv, SANS_12, g.ox + px + 4, g.oy + py - 3, lab, g.label);
    };
    static const char *const g2[2] = {"0", "1"};
    static const char *const g4[4] = {"00", "01", "11", "10"};
    static const char *const g8[8] = {"000", "001", "011", "010", "110", "111", "101", "100"};
    switch (t) {
    case 0: pt(0, 0, "0"); pt(0.9, 0, "1"); break;
    case 1: pt(-0.9, 0, "0"); pt(0.9, 0, "1"); break;
    case 2: for (int k = 0; k < 4; k++) { double a = fm::PI / 4 + k * fm::PI / 2; pt(0.9 * fm::cos(a), 0.9 * fm::sin(a), g4[k]); } break;
    case 3: for (int k = 0; k < 8; k++) { double a = k * fm::PI / 4; pt(0.9 * fm::cos(a), 0.9 * fm::sin(a), g8[k]); } break;
    case 7: pt(0.9, 0, nullptr); pt(0, 0.9, nullptr);
        font::drawText(*g.cv, SANS_12, g.ox + 8, g.oy + 18, "2 fréquences f\xE2\x82\x80, f\xE2\x82\x81", g.label);
        break;
    default: {
        int m = (int)fm::sqrt((double)MS[t]);
        for (int i = 0; i < m; i++)
            for (int j = 0; j < m; j++) {
                double x = -0.9 + 1.8 * i / (m - 1), y = -0.9 + 1.8 * j / (m - 1);
                pt(x, y, nullptr);
            }
    }
    }
    (void)g2;
    valText(g, 6, 140, "R", s.v[ORS], U_BAUD, 0, true);
    char buf[40];
    int k = scopy(buf, sizeof buf, "TEB = ");
    fmtNumber(buf + k, (int)sizeof(buf) - k, s.v[OBER], 3);
    font::drawRich(*g.cv, BOLD_12, g.ox + 6, g.oy + 155, buf, g.volt);
    valText(g, 6, 170, "B", s.v[OB], U_HZ);
}
}  // namespace dig

// ==========================================================================
// Valeurs moyenne et efficace
// ==========================================================================
namespace rms {
enum { A, V0, DUTY, OMOY, OEFF, OEFFAC, OCREST, OFORM, OMAX, OMIN };
enum { CWAVE };
enum { T_W = 1 };
static const char *const WAVES = "sinus|carré ±A|rectangle (0 à A)|triangle|dent de scie|sinus redressé simple|sinus redressé double";
static void def(CalcState &s) { s.v[A] = 10; s.v[V0] = 0; s.v[DUTY] = 25; }
static void rows(const CalcState &s, RowList &r)
{
    r.choice("#Forme", CWAVE, WAVES, T_W);
    r.in("A", A, U_V, ST_LIN1, T_W, "Amplitude (valeur crête)", RF_NONNEG);
    r.in("V_0", V0, U_V, ST_LIN1, T_W, "Composante continue ajoutée (décalage)", 0);
    if (s.c[CWAVE] == 2) r.in("\\alpha", DUTY, U_PCT, ST_LIN1, T_W, "Rapport cyclique", RF_NONNEG);
    r.head("Résultats");
    r.out("V_{moy}", OMOY, U_V, "Valeur moyenne", true);
    r.out("V_{eff}", OEFF, U_V, "Valeur efficace (RMS) totale", true);
    r.out("V_{eff,AC}", OEFFAC, U_V, "Valeur efficace de la composante alternative √(V_eff² − V_moy²)");
    r.out("V_{\\max}", OMAX, U_V, "Valeur maximale");
    r.out("V_{\\min}", OMIN, U_V, "Valeur minimale");
    r.out("F_c", OCREST, U_NONE, "Facteur de crête max|v| / V_eff");
    r.out("F_f", OFORM, U_NONE, "Facteur de forme V_eff / |V_moy|");
}
static double waveAt(int w, double A, double V0, double al, double t)
{
    double x = t - fm::floor(t);
    switch (w) {
    case 0: return V0 + A * fm::sin(fm::TWO_PI * x);
    case 1: return V0 + (x < 0.5 ? A : -A);
    case 2: return V0 + (x < al ? A : 0);
    case 3: return V0 + (x < 0.25 ? 4 * A * x : x < 0.75 ? A * (2 - 4 * x) : A * (4 * x - 4));
    case 4: return V0 + A * (2 * x - 1);
    case 5: { double v = fm::sin(fm::TWO_PI * x); return V0 + (v > 0 ? A * v : 0); }
    default: return V0 + A * fm::fabs(fm::sin(fm::TWO_PI * x));
    }
}
static void calc(CalcState &s)
{
    int w = s.c[CWAVE];
    double a = s.v[A], v0 = s.v[V0], al = s.v[DUTY] / 100;
    if (al > 1) al = 1;
    double m, m2, mx, mn;
    const double PI = fm::PI;
    switch (w) {
    case 0: m = v0; m2 = v0 * v0 + a * a / 2; mx = v0 + a; mn = v0 - a; break;
    case 1: m = v0; m2 = v0 * v0 + a * a; mx = v0 + a; mn = v0 - a; break;
    case 2: m = v0 + al * a; m2 = (1 - al) * v0 * v0 + al * (v0 + a) * (v0 + a); mx = v0 + (al > 0 ? a : 0); mn = v0 + (al < 1 ? 0 : a); break;
    case 3: case 4: m = v0; m2 = v0 * v0 + a * a / 3; mx = v0 + a; mn = v0 - a; break;
    case 5: m = v0 + a / PI; m2 = v0 * v0 + 2 * v0 * a / PI + a * a / 4; mx = v0 + a; mn = v0; break;
    default: m = v0 + 2 * a / PI; m2 = v0 * v0 + 4 * v0 * a / PI + a * a / 2; mx = v0 + a; mn = v0; break;
    }
    double eff = fm::sqrt(m2), ac = m2 - m * m;
    s.v[OMOY] = m;
    s.v[OEFF] = eff;
    s.v[OEFFAC] = fm::sqrt(ac > 0 ? ac : 0);
    s.v[OMAX] = mx;
    s.v[OMIN] = mn;
    double pk = fm::fmax(fm::fabs(mx), fm::fabs(mn));
    s.v[OCREST] = eff > 0 ? pk / eff : NaN();
    s.v[OFORM] = fm::fabs(m) > 1e-12 * (fm::fabs(a) + fm::fabs(v0)) ? eff / fm::fabs(m) : NaN();
}
static void sch(Sch &g, const CalcState &s)
{
    int w = s.c[CWAVE];
    double a = s.v[A], v0 = s.v[V0], al = s.v[DUTY] / 100;
    double span = fm::fmax(fm::fabs(s.v[OMAX]), fm::fabs(s.v[OMIN]));
    if (!(span > 0)) span = 1;
    const int X0 = 12, X1 = 168, Y = 74, H = 46;
    auto yOf = [&](double v) { return Y - (int)(v / span * H); };
    g.line(X0, Y, X1, Y, g.label);
    int px = 0, py = 0;
    for (int i = 0; i <= 312; i++) {
        double t = 2.0 * i / 312;
        int x = X0 + i / 2, y = yOf(waveAt(w, a, v0, al, t));
        if (i) { g.line(px, py, x, y, g.col(T_W)); }
        px = x; py = y;
    }
    // valeur moyenne et efficace
    int ym = yOf(s.v[OMOY]), ye = yOf(s.v[OEFF]);
    for (int x = X0; x < X1; x += 4) { g.rect(x, ym, 2, 1, rgb(200, 40, 50)); g.rect(x, ye, 2, 1, rgb(30, 110, 220)); }
    font::drawText(*g.cv, SANS_12, g.ox + X1 - 30, g.oy + ym - 2, "moy", rgb(200, 40, 50));
    font::drawText(*g.cv, SANS_12, g.ox + X1 - 30, g.oy + ye - 2, "eff", rgb(30, 110, 220));
    valText(g, 6, 148, "V_moy", s.v[OMOY], U_V, 0, true);
    valText(g, 6, 166, "V_eff", s.v[OEFF], U_V, 0, true);
}
}  // namespace rms

// ==========================================================================
// Filtre numerique du premier ordre / moyenne glissante
// ==========================================================================
namespace dfilt {
enum { FS, FC, N, OALPHA, OTAU, OF3, OF0, ODELAY };
enum { CTYPE };
static char s_eq[96];
static void def(CalcState &s) { s.v[FS] = 1e3; s.v[FC] = 10; s.v[N] = 8; }
static void rows(const CalcState &s, RowList &r)
{
    bool ema = s.c[CTYPE] == 0;
    r.choice("#Filtre", CTYPE, "moyenne exponentielle (RC numérique)|moyenne glissante sur N points", 0);
    r.in("f_e", FS, U_HZ, ST_125, 0, "Fréquence d'échantillonnage");
    if (ema) r.in("f_c", FC, U_HZ, ST_125, 0, "Fréquence de coupure visée (équivalent RC)");
    else r.in("N", N, U_NONE, ST_INT, 0, "Nombre de points moyennés", RF_POS | RF_INT);
    r.head("Résultats");
    r.texOut("", 0, "Équation de récurrence à programmer");
    if (ema) {
        r.out("\\alpha", OALPHA, U_NONE, "Coefficient 1 − exp(−2π f_c/f_e)", true);
        r.out("\\tau", OTAU, U_S, "Constante de temps équivalente");
    } else {
        r.out("f_0", OF0, U_HZ, "Premier zéro de transmission f_e/N");
        r.out("\\tau_g", ODELAY, U_S, "Retard (N − 1)/(2 f_e)");
    }
    r.out("f_{-3dB}", OF3, U_HZ, "Coupure exacte à −3 dB (calculée sur H(z))", true);
    r.msg(1);
}
static bool H(const CalcState &s, double f, Cx &h)
{
    double fs = s.v[FS];
    if (f >= fs / 2) return false;
    double w = fm::TWO_PI * f / fs;
    Cx z1 = cx(fm::cos(w), -fm::sin(w));   // e^{-jw}
    if (s.c[CTYPE] == 0) {
        double a = s.v[OALPHA];
        h = cx(a) / (cx(1) - (1 - a) * z1);
    } else {
        int n = (int)s.v[N];
        if (n < 1) n = 1;
        if (fm::fabs(w) < 1e-9) { h = cx(1); return true; }
        Cx zn = cx(fm::cos(n * w), -fm::sin(n * w));
        h = (cx(1) - zn) / (n * (cx(1) - z1));
    }
    return !cbad(h);
}
static void calc(CalcState &s)
{
    double fs = s.v[FS];
    int k;
    if (s.c[CTYPE] == 0) {
        double a = 1 - fm::exp(-fm::TWO_PI * s.v[FC] / fs);
        s.v[OALPHA] = a;
        s.v[OTAU] = -1 / (fs * fm::ln(1 - a));
        scopy(s_eq, sizeof s_eq, "y_n=y_{n-1}+\\alpha\\,(x_n-y_{n-1})");
        (void)k;
    } else {
        int n = (int)s.v[N];
        s.v[OF0] = fs / n;
        s.v[ODELAY] = (n - 1) / (2 * fs);
        scopy(s_eq, sizeof s_eq, "y_n=\\frac{1}{N}\\sum_{k=0}^{N-1}x_{n-k}");
        (void)k;
    }
    setMsg(s, 0, s_eq, ML_INFO);
    // -3 dB par dichotomie (gain decroissant au debut)
    double lo = 0, hi = fs / 2 * 0.999999;
    Cx h;
    if (H(s, hi, h) && cabs(h) > 1 / fm::sqrt(2.0)) {
        s.v[OF3] = NaN();
        setMsg(s, 1, "Le gain ne descend pas à −3 dB avant f_e/2", ML_WARN);
        return;
    }
    if (s.c[CTYPE] == 1) hi = fm::fmin(hi, fs / s.v[N]);
    for (int it = 0; it < 70; it++) {
        double md = 0.5 * (lo + hi);
        H(s, md, h);
        if (cabs(h) > 1 / fm::sqrt(2.0)) lo = md;
        else hi = md;
    }
    s.v[OF3] = 0.5 * (lo + hi);
    setMsg(s, 1, "F4 : réponse en fréquence jusqu'à f_e/2", ML_INFO);
}
static bool tf(const CalcState &s, double f, Cx &h) { return H(s, f, h); }
static double fc(const CalcState &s) { return ok(s.v[OF3]) ? s.v[OF3] : s.v[FS] / 10; }
static void sch(Sch &g, const CalcState &s)
{
    // structure : x -> (+) -> retard z^-1 -> y
    bool ema = s.c[CTYPE] == 0;
    const int y = 70;
    g.term(10, y);
    g.texSmall(10, y - 8, "x_n", tex::CENTER);
    if (ema) {
        g.wire(10, y, 50, y);
        g.cv->circle(g.ox + 58, g.oy + y, 8, g.ink);
        g.texSmall(58, y + 4, "#+", tex::CENTER);
        g.wire(66, y, 96, y);
        g.chip(96, y - 11, 26, 22, "\\alpha", 0);
        g.wire(122, y, 168, y);
        g.term(168, y);
        g.texSmall(168, y - 8, "y_n", tex::CENTER);
        g.wire(140, y, 140, 112);
        g.dot(140, y);
        g.chip(100, 101, 30, 22, "z^{-1}", 0);
        g.wire(140, 112, 130, 112);
        g.wire(100, 112, 58, 112);
        g.wire(58, 112, 58, y + 8);
        g.cv->arrowHead(g.ox + 58, g.oy + y + 9, 0, -1, 4, g.ink);
        g.texSmall(30, 100, "-y_{n-1}", tex::CENTER);
    } else {
        for (int k = 0; k < 4; k++) g.chip(30 + 34 * k, 52, 24, 20, "z^{-1}", 0);
        g.wire(10, 62, 30, 62);
        for (int k = 0; k < 3; k++) g.wire(54 + 34 * k, 62, 64 + 34 * k, 62);
        for (int k = 0; k < 5; k++) {
            int x = 20 + 34 * k;
            g.wire(x, 62, x, 100);
            g.dot(x, 62);
        }
        g.wire(20, 100, 156, 100);
        g.cv->circle(g.ox + 160, g.oy + 100, 8, g.ink);
        g.texSmall(160, 104, "#Σ", tex::CENTER);
        g.texSmall(89, 124, "\\times\\frac{1}{N}", tex::CENTER);
    }
    if (ema) valText(g, 6, 152, "\xCE\xB1", s.v[OALPHA], U_NONE, 0, true);
    else valText(g, 6, 152, "f_0", s.v[OF0], U_HZ, 0, true);
    valText(g, 6, 168, "f_-3dB", s.v[OF3], U_HZ);
}
}  // namespace dfilt

extern const CalcDef g_calcSig[] = {
    {"Échantillonnage et repliement", "#Shannon, fréquence apparente, chronogramme", "echantillonnage repliement aliasing shannon nyquist frequence apparente", 16, G_CUSTOM, 0,
     samp::def, samp::rows, samp::calc, samp::sch, nullptr, nullptr, samp::graph, "Chrono."},
    {"Convertisseur analogique-numérique", "#Quantum, code, bruit de quantification, SNR", "can cna adc dac convertisseur quantification quantum lsb code snr resolution bits", 16, G_NONE, 0,
     adc::def, adc::rows, adc::calc, adc::sch},
    {"Acquisition et FFT", "#Résolution, durée, fenêtres de pondération", "fft acquisition resolution frequentielle fenetre hann hamming blackman fuite spectrale enbw", 16, G_CUSTOM, 0,
     fft::def, fft::rows, fft::calc, fft::sch, nullptr, nullptr, fft::graph, "Fenêtre"},
    {"Modulation d'amplitude", "#AM, DBSP, BLU ; détecteur d'enveloppe", "modulation amplitude am dbsp blu bande laterale indice demodulation detecteur enveloppe", 16, G_CUSTOM, 0,
     am::def, am::rows, am::calc, am::sch, nullptr, nullptr, am::graph, "Signal"},
    {"Modulation de fréquence et de phase", "#Indice, bande de Carson, raies de Bessel", "modulation frequence phase fm pm indice carson bessel excursion", 16, G_CUSTOM, 0,
     fmm::def, fmm::rows, fmm::calc, fmm::sch, nullptr, nullptr, fmm::graph, "Spectre"},
    {"Modulations numériques", "#ASK, PSK, QAM, FSK : débit, bande, TEB", "modulation numerique ask ook psk bpsk qpsk qam fsk debit rapidite teb ber eb n0 constellation", 16, G_NONE, 0,
     dig::def, dig::rows, dig::calc, dig::sch},
    {"Valeurs moyenne et efficace", "#Sinus, carré, triangle, redressé…", "valeur moyenne efficace rms facteur crete forme sinus carre triangle redresse", 16, G_NONE, 0,
     rms::def, rms::rows, rms::calc, rms::sch},
    {"Filtre numérique du premier ordre", "#Moyenne exponentielle ou glissante, coupure", "filtre numerique moyenne glissante exponentielle iir fir rc coupure recurrence", 16, G_BODE, 0,
     dfilt::def, dfilt::rows, dfilt::calc, dfilt::sch, dfilt::tf, dfilt::fc, nullptr, "Bode", 0},
};
extern const int g_calcSigN = sizeof(g_calcSig) / sizeof(g_calcSig[0]);
