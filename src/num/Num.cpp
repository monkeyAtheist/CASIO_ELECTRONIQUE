#include "Num.hpp"

int scopy(char *dst, int cap, const char *src)
{
    int i = 0;
    for (; i < cap - 1 && src[i]; i++) dst[i] = src[i];
    dst[i] = 0;
    return i;
}

// --------------------------------------------------------------------------
// Unites et prefixes
// --------------------------------------------------------------------------
static const char *const s_unitSym[U_COUNT] = {
    "", "\xCE\xA9", "F", "H", "V", "A", "W", "Hz", "s", "S", "A/V\xC2\xB2", "rad/s", "C", "J", "Wb",
    "dB", "dBm", "dBW", "dBV", "dBu", "\xC2\xB0", "%", "\xC2\xB0" "C", "\xC2\xB0" "C/W",
    "m", "T", "N\xC2\xB7m", "bit/s", "Bd", "m/s", "V\xC2\xB7s/rad", "o", "VA", "var",
    "mm\xC2\xB2", "cm\xC2\xB2", "tr/min", "K", "A/mm\xC2\xB2",
    "dBi", "dBc", "dBm/Hz",
};

bool unitHasPrefix(uint8_t u)
{
    return (u >= U_OHM && u <= U_WB) || (u >= U_M && u <= U_VAR);
}

const char *unitSymbol(uint8_t u)
{
    return u < U_COUNT ? s_unitSym[u] : "";
}

static const char *const s_pfx[PFX_COUNT] = {"f", "p", "n", "\xC2\xB5", "m", "", "k", "M", "G", "T"};

const char *prefixSymbol(int p)
{
    return (p >= 0 && p < PFX_COUNT) ? s_pfx[p] : "";
}

int prefixExp(int p)
{
    return (p - PFX_1) * 3;
}

// --------------------------------------------------------------------------
// Ecriture des nombres
// --------------------------------------------------------------------------
struct Out {
    char *p;
    int cap, n;
    void c(char ch) { if (n < cap - 1) p[n++] = ch; }
    void s(const char *t) { while (*t) c(*t++); }
    void end() { p[n < cap ? n : cap - 1] = 0; }
};

static const char *const s_sup[10] = {
    "\xE2\x81\xB0", "\xC2\xB9", "\xC2\xB2", "\xC2\xB3", "\xE2\x81\xB4",
    "\xE2\x81\xB5", "\xE2\x81\xB6", "\xE2\x81\xB7", "\xE2\x81\xB8", "\xE2\x81\xB9",
};
static const char *MINUS = "\xE2\x88\x92";
static const char *SUP_MINUS = "\xE2\x81\xBB";

// Entier q ecrit avec 'dec' decimales (q = valeur * 10^dec), zeros de fin retires.
static void putFixed(Out &o, long long q, int dec, bool trim)
{
    char dig[24];
    int nd = 0;
    long long t = q;
    do { dig[nd++] = (char)('0' + t % 10); t /= 10; } while (t && nd < 22);
    while (nd <= dec) dig[nd++] = '0';      // zeros de tete (0,0xx)
    int last = 0;                           // nombre de decimales a ecrire
    if (dec > 0) {
        last = dec;
        if (trim) while (last > 0 && dig[dec - last] == '0') last--;
    }
    for (int i = nd - 1; i >= dec; i--) o.c(dig[i]);
    if (last > 0) {
        o.c(',');
        for (int i = dec - 1; i >= dec - last; i--) o.c(dig[i]);
    }
}

static double pow10i(int e)
{
    if (e >= -18 && e <= 18) return fm::p10(e);
    double r = 1;
    while (e > 18) { r *= 1e18; e -= 18; }
    while (e < -18) { r *= 1e-18; e += 18; }
    return r * fm::p10(e);
}

// floor(log10 a) + 1 pour a > 0 : 1 pour [1 ; 10[, 0 pour [0,1 ; 1[, -1 pour [0,01 ; 0,1[...
static int intDigits(double a)
{
    int e = 0;
    if (a >= 1) {
        while (e < 308 && a >= pow10i(e + 1)) e++;
    } else {
        while (e > -330 && a < pow10i(e)) e--;
    }
    return e + 1;
}

static void putSci(Out &o, double a, int sig)
{
    int e = intDigits(a) - 1;
    double m = a / pow10i(e);
    int dec = sig - 1;
    long long q = (long long)(m * pow10i(dec) + 0.5);
    if (q >= (long long)pow10i(sig)) { q /= 10; e++; }
    putFixed(o, q, dec, true);
    o.s("\xC3\x97" "10");   // ×10
    if (e < 0) { o.s(SUP_MINUS); e = -e; }
    char d[8];
    int n = 0;
    do { d[n++] = (char)(e % 10); e /= 10; } while (e && n < 7);
    while (n) o.s(s_sup[(int)d[--n]]);
}

// Partie numerique "fixe" de a (> 0) avec sig chiffres significatifs.
static void putSig(Out &o, double a, int sig)
{
    int nd = intDigits(a);
    int dec = sig - nd;
    if (dec < 0) dec = 0;
    long long q = (long long)(a * pow10i(dec) + 0.5);
    // arrondi qui ajoute un chiffre (9,9996 -> 10,00)
    if (dec > 0 && q >= (long long)pow10i(sig)) { q = (q + 5) / 10; dec--; }
    putFixed(o, q, dec, true);
}

static bool special(Out &o, double v)
{
    if (fm::isnan(v)) { o.s("\xE2\x80\x94"); return true; }   // tiret cadratin
    if (fm::isinf(v)) {
        if (v < 0) o.s(MINUS);
        o.s("\xE2\x88\x9E");
        return true;
    }
    return false;
}

int fmtNumber(char *out, int cap, double v, int sig)
{
    Out o{out, cap, 0};
    if (!special(o, v)) {
        double a = fm::fabs(v);
        if (a == 0) o.c('0');
        else {
            // valeur qui s'arrondit a zero : pas de signe
            if (v < 0) o.s(MINUS);
            if (a >= 1e-3 && a < 999999.5) putSig(o, a, sig);
            else putSci(o, a, sig);
        }
    }
    o.end();
    return o.n;
}

// Nombre avec un nombre fixe de decimales (dB, degres...), zeros retires.
static void putDec(Out &o, double v, int dec)
{
    double a = fm::fabs(v);
    if (a >= 1e9) { if (v < 0) o.s(MINUS); putSci(o, a, 4); return; }
    long long q = (long long)(a * pow10i(dec) + 0.5);
    if (q == 0) { o.c('0'); return; }
    if (v < 0) o.s(MINUS);
    putFixed(o, q, dec, true);
}

int fmtValue(char *out, int cap, double v, uint8_t unit, int sig)
{
    Out o{out, cap, 0};
    if (special(o, v)) {
        if (!fm::isnan(v) && unit != U_NONE) { o.c(' '); o.s(unitSymbol(unit)); }
        o.end();
        return o.n;
    }
    const char *sym = unitSymbol(unit);
    switch (unit) {
    case U_NONE:
        o.end();
        return fmtNumber(out, cap, v, sig);
    case U_MM2: case U_CM2: case U_RPM: case U_K: case U_AMM2: {
        o.end();
        int n = fmtNumber(out, cap, v, sig);
        Out o2{out, cap, n};
        o2.c(' ');
        o2.s(sym);
        o2.end();
        return o2.n;
    }
    case U_DB: case U_DBM: case U_DBW: case U_DBV: case U_DBU: case U_DBI: case U_DBC: case U_DBMHZ:
        putDec(o, v, 2);
        o.c(' ');
        o.s(sym);
        o.end();
        return o.n;
    case U_DEG:
        putDec(o, v, 1);
        o.s(sym);
        o.end();
        return o.n;
    case U_PCT: case U_DEGC:
        putDec(o, v, 1);
        o.c(' ');
        o.s(sym);
        o.end();
        return o.n;
    case U_CW:
        putDec(o, v, 2);
        o.c(' ');
        o.s(sym);
        o.end();
        return o.n;
    default:
        break;
    }
    double a = fm::fabs(v);
    if (a == 0) {
        o.s("0 ");
        o.s(sym);
        o.end();
        return o.n;
    }
    if (a < 0.9995e-15 || a >= 999.95e12) {
        if (v < 0) o.s(MINUS);
        putSci(o, a, sig);
        o.c(' ');
        o.s(sym);
        o.end();
        return o.n;
    }
    // prefixe : 1 <= m < 1000 apres arrondi a sig chiffres
    int p = PFX_1;
    while (p < PFX_T && a >= pow10i(prefixExp(p) + 3) * (1.0 - 0.5 * pow10i(-sig))) p++;
    while (p > PFX_F && a < pow10i(prefixExp(p)) * (1.0 - 0.5 * pow10i(-sig))) p--;
    double m = a / pow10i(prefixExp(p));
    if (v < 0) o.s(MINUS);
    putSig(o, m, sig);
    o.c(' ');
    o.s(prefixSymbol(p));
    o.s(sym);
    o.end();
    return o.n;
}

// --------------------------------------------------------------------------
// Saisie
// --------------------------------------------------------------------------
bool parseNumber(const char *s, double &v)
{
    bool neg = false;
    if (*s == '-') { neg = true; s++; }
    double mant = 0;
    int digits = 0, scale = 0, sigd = 0;
    bool point = false;
    for (; *s && *s != 'E'; s++) {
        if (*s == '.') {
            if (point) return false;
            point = true;
            continue;
        }
        if (*s < '0' || *s > '9') return false;
        digits++;
        if (sigd < 17) {
            mant = mant * 10 + (*s - '0');
            if (mant > 0) sigd++;
            if (point) scale--;
        } else if (!point) {
            scale++;
        }
    }
    if (!digits) return false;
    int ex = 0;
    if (*s == 'E') {
        s++;
        bool eneg = false;
        if (*s == '-') { eneg = true; s++; }
        int nd = 0;
        for (; *s; s++) {
            if (*s < '0' || *s > '9') return false;
            if (nd < 4) ex = ex * 10 + (*s - '0');
            nd++;
        }
        if (eneg) ex = -ex;
    }
    int e = ex + scale;
    if (e > 300) e = 300;
    if (e < -300) e = -300;
    double r = mant;
    if (e >= 0) r *= pow10i(e);
    else if (e >= -22) r /= pow10i(-e);   // division par une puissance exacte
    else r *= pow10i(e);
    v = neg ? -r : r;
    return true;
}

int fmtInput(char *out, int cap, const char *s)
{
    Out o{out, cap, 0};
    bool expo = false;
    for (; *s; s++) {
        char c = *s;
        if (expo) {
            if (c == '-') o.s(SUP_MINUS);
            else if (c >= '0' && c <= '9') o.s(s_sup[c - '0']);
            continue;
        }
        if (c == '.') o.c(',');
        else if (c == '-') o.s(MINUS);
        else if (c == 'E') { o.s("\xC3\x97" "10"); expo = true; }
        else o.c(c);
    }
    o.end();
    return o.n;
}

// --------------------------------------------------------------------------
// Series normalisees
// --------------------------------------------------------------------------
static const uint16_t s_e6[] = {100, 150, 220, 330, 470, 680};
static const uint16_t s_e12[] = {100, 120, 150, 180, 220, 270, 330, 390, 470, 560, 680, 820};
static const uint16_t s_e24[] = {100, 110, 120, 130, 150, 160, 180, 200, 220, 240, 270, 300,
                                 330, 360, 390, 430, 470, 510, 560, 620, 680, 750, 820, 910};
static const uint16_t s_e96[] = {
    100, 102, 105, 107, 110, 113, 115, 118, 121, 124, 127, 130, 133, 137, 140, 143,
    147, 150, 154, 158, 162, 165, 169, 174, 178, 182, 187, 191, 196, 200, 205, 210,
    215, 221, 226, 232, 237, 243, 249, 255, 261, 267, 274, 280, 287, 294, 301, 309,
    316, 324, 332, 340, 348, 357, 365, 374, 383, 392, 402, 412, 422, 432, 442, 453,
    464, 475, 487, 499, 511, 523, 536, 549, 562, 576, 590, 604, 619, 634, 649, 665,
    681, 698, 715, 732, 750, 768, 787, 806, 825, 845, 866, 887, 909, 931, 953, 976,
};

int eCount(int series)
{
    switch (series) {
    case E6: return 6;
    case E12: return 12;
    case E24: return 24;
    case E48: return 48;
    default: return 96;
    }
}

int eMantissa(int series, int i)
{
    switch (series) {
    case E6: return s_e6[i];
    case E12: return s_e12[i];
    case E24: return s_e24[i];
    case E48: return s_e96[2 * i];   // E48 = une valeur sur deux de E96
    default: return s_e96[i];
    }
}

// Decade d : valeurs mantisse * 10^(d-2), soit [10^d ; 10^(d+1)[.
static int decadeOf(double x)
{
    return intDigits(x) - 1;
}

static double eValue(int series, int i, int d)
{
    return eMantissa(series, i) * pow10i(d - 2);
}

double eNearest(int series, double x)
{
    if (!(x > 0) || !fm::isfinite(x)) return x;
    int d0 = decadeOf(x);
    double best = x, br = 1e300;
    for (int d = d0 - 1; d <= d0 + 1; d++) {
        for (int i = 0; i < eCount(series); i++) {
            double v = eValue(series, i, d);
            double r = v > x ? v / x : x / v;
            if (r < br) { br = r; best = v; }
        }
    }
    return best;
}

double eStep(int series, double x, int dir)
{
    if (!(x > 0) || !fm::isfinite(x)) return dir > 0 ? 1.0 : x;
    int d0 = decadeOf(x);
    double best = x;
    bool found = false;
    for (int d = d0 - 1; d <= d0 + 1; d++) {
        for (int i = 0; i < eCount(series); i++) {
            double v = eValue(series, i, d);
            if (dir > 0 ? v > x * (1 + 1e-9) : v < x * (1 - 1e-9)) {
                if (!found || (dir > 0 ? v < best : v > best)) { best = v; found = true; }
            }
        }
    }
    return best;
}

double eCeil(int series, double x)
{
    if (!(x > 0)) return x;
    double n = eNearest(series, x);
    if (n >= x * (1 - 1e-9)) return n;
    return eStep(series, n, 1);
}

double eFloor(int series, double x)
{
    if (!(x > 0)) return x;
    double n = eNearest(series, x);
    if (n <= x * (1 + 1e-9)) return n;
    return eStep(series, n, -1);
}

double step125(double x, int dir)
{
    static const int m[3] = {100, 200, 500};
    if (!(x > 0) || !fm::isfinite(x)) return dir > 0 ? 1.0 : x;
    int d0 = decadeOf(x);
    double best = x;
    bool found = false;
    for (int d = d0 - 1; d <= d0 + 1; d++) {
        for (int i = 0; i < 3; i++) {
            double v = m[i] * pow10i(d - 2);
            if (dir > 0 ? v > x * (1 + 1e-9) : v < x * (1 - 1e-9)) {
                if (!found || (dir > 0 ? v < best : v > best)) { best = v; found = true; }
            }
        }
    }
    return best;
}

// --------------------------------------------------------------------------
// Entiers et complexes
// --------------------------------------------------------------------------
int fmtInt(char *out, int cap, double v)
{
    Out o{out, cap, 0};
    if (!fm::isfinite(v) || fm::fabs(v) >= 1e15) {
        o.end();
        return fmtValue(out, cap, v, U_NONE);
    }
    long long q = (long long)(v < 0 ? -v + 0.5 : v + 0.5);
    if (v < 0 && q) o.s(MINUS);
    char d[24];
    int n = 0;
    do { d[n++] = (char)('0' + q % 10); q /= 10; } while (q && n < 22);
    for (int i = n - 1; i >= 0; i--) {
        o.c(d[i]);
        if (i && i % 3 == 0 && n > 4) o.s("\xC2\xA0");
    }
    o.end();
    return o.n;
}

int fmtComplex(char *out, int cap, double re, double im, uint8_t unit, bool polar, int sig)
{
    Out o{out, cap, 0};
    if (fm::isnan(re) || fm::isnan(im)) { o.s("\xE2\x80\x94"); o.end(); return o.n; }
    char a[40], b[40];
    double m = fm::hypot(re, im);
    if (polar) {
        fmtValue(a, sizeof a, m, unit, sig);
        o.s(a);
        if (m > 0 && fm::isfinite(m)) {
            o.s(" \xE2\x88\xA0");
            fmtValue(b, sizeof b, fm::deg(fm::atan2(im, re)), U_DEG);
            o.s(b);
        }
        o.end();
        return o.n;
    }
    bool hasRe = fm::fabs(re) > 1e-12 * m, hasIm = fm::fabs(im) > 1e-12 * m;
    if (!(m > 0)) hasRe = true;
    if (hasRe) {
        fmtValue(a, sizeof a, re, unit, hasIm ? 3 : sig);
        o.s(a);
    }
    if (hasIm) {
        if (hasRe) o.s(im < 0 ? " \xE2\x88\x92 j" : " + j");
        else o.s(im < 0 ? "\xE2\x88\x92j" : "j");
        fmtValue(b, sizeof b, fm::fabs(im), unit, hasRe ? 3 : sig);
        o.s(b);
    }
    o.end();
    return o.n;
}
