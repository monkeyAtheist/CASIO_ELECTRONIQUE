// Verifie les fonctions mathematiques maison (contre libm) et le formatage.
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "../../src/num/FMath.hpp"
#include "../../src/num/Num.hpp"
#include "../../src/num/Cx.hpp"

static int s_fail = 0;
static double s_worst[16];
static const char *s_names[16] = {"sqrt","exp","ln","log10","sin","cos","atan","atan2","pow10","hypot"};

static void cmp(int k, double got, double ref)
{
    double err = fabs(got - ref) / (fabs(ref) > 1e-300 ? fabs(ref) : 1.0);
    if (ref == 0) err = fabs(got);
    if (err > s_worst[k]) s_worst[k] = err;
}

static void fmt(double v, int u, const char *expect, int sig = 4)
{
    char b[64];
    fmtValue(b, sizeof b, v, (uint8_t)u, sig);
    if (strcmp(b, expect)) { printf("FORMAT %.17g -> [%s] attendu [%s]\n", v, b, expect); s_fail++; }
}

int main()
{
    unsigned rng = 12345;
    auto rnd = [&]() { rng = rng * 1103515245u + 12345u; return (rng >> 8) / 16777216.0; };
    for (int i = 0; i < 200000; i++) {
        double e = (rnd() - 0.5) * 60;              // 1e-30 .. 1e30
        double x = pow(10.0, e) * (0.5 + rnd());
        cmp(0, fm::sqrt(x), sqrt(x));
        cmp(2, fm::ln(x), log(x));
        cmp(3, fm::log10(x), log10(x));
        double y = (rnd() - 0.5) * 80;
        cmp(1, fm::exp(y), exp(y));
        double a = (rnd() - 0.5) * 200;
        // sin/cos : erreur absolue (les zeros rendent l'erreur relative inutile)
        double es = fabs(fm::sin(a) - sin(a)), ec = fabs(fm::cos(a) - cos(a));
        if (es > s_worst[4]) s_worst[4] = es;
        if (ec > s_worst[5]) s_worst[5] = ec;
        double u = (rnd() - 0.5) * 10, v = (rnd() - 0.5) * 10;
        double ea = fabs(fm::atan2(u, v) - atan2(u, v));
        if (ea > s_worst[7]) s_worst[7] = ea;
        double p = (rnd() - 0.5) * 36;
        cmp(8, fm::pow10(p), pow(10.0, p));
        cmp(9, fm::hypot(u * x, v), hypot(u * x, v));
    }
    // atan
    for (int i = 0; i < 100000; i++) {
        double x = pow(10.0, (rnd() - 0.5) * 12) * (rnd() < 0.5 ? -1 : 1);
        cmp(6, fm::atan(x), atan(x));
    }
    for (int k = 0; k < 10; k++) {
        double lim = (k == 4 || k == 5) ? 1e-13 : (k == 1 || k == 8) ? 3e-14 : 2e-15;
        printf("%-6s erreur max %.3g %s\n", s_names[k], s_worst[k], s_worst[k] > lim ? "  <-- TROP" : "");
        if (s_worst[k] > lim) s_fail++;
    }
    // formatage
    fmt(4700, U_OHM, "4,7 k\xCE\xA9");
    fmt(10e3, U_OHM, "10 k\xCE\xA9");
    fmt(159.15494, U_HZ, "159,2 Hz");
    fmt(1591.5494, U_HZ, "1,592 kHz");
    fmt(999.96, U_OHM, "1 k\xCE\xA9");
    fmt(999.94, U_OHM, "999,9 \xCE\xA9");
    fmt(0.99997, U_V, "1 V");
    fmt(0.9999, U_V, "999,9 mV");
    fmt(100e-9, U_F, "100 nF");
    fmt(4.7e-12, U_F, "4,7 pF");
    fmt(2.2e-6, U_F, "2,2 \xC2\xB5" "F");
    fmt(-15, U_V, "\xE2\x88\x92" "15 V");
    fmt(0, U_V, "0 V");
    fmt(-3.0103, U_DB, "\xE2\x88\x92" "3,01 dB");
    fmt(-45.0, U_DEG, "\xE2\x88\x92" "45\xC2\xB0");
    fmt(-0.00001, U_DB, "0 dB");
    fmt(12.5, U_NONE, "12,5");
    fmt(-10, U_NONE, "\xE2\x88\x92" "10");
    fmt(123456789, U_NONE, "1,235\xC3\x97" "10\xE2\x81\xB8");
    fmt(0.000012, U_NONE, "1,2\xC3\x97" "10\xE2\x81\xBB\xE2\x81\xB5");
    fmt(0.0123, U_NONE, "0,0123");
    fmt(fm::nan(), U_V, "\xE2\x80\x94");
    fmt(1e-18, U_F, "1\xC3\x97" "10\xE2\x81\xBB\xC2\xB9\xE2\x81\xB8 F");
    fmt(2.5e9, U_HZ, "2,5 GHz");
    fmt(0.035, U_A, "35 mA");
    fmt(50, U_PCT, "50 %");
    // saisie
    struct { const char *s; double v; } P[] = {
        {"4.7", 4.7}, {"-15", -15}, {"1E3", 1000}, {"2.2E-6", 2.2e-6}, {".5", 0.5},
        {"100", 100}, {"0.000001", 1e-6}, {"12345678901234567890", 1.2345678901234567e19},
    };
    for (auto &t : P) {
        double v;
        if (!parseNumber(t.s, v) || fabs(v - t.v) > 1e-15 * fabs(t.v)) { printf("PARSE %s -> %.17g\n", t.s, v); s_fail++; }
    }
    double dummy;
    const char *bad[] = {"", "-", ".", "1.2.3", "E5"};
    for (auto b : bad) if (parseNumber(b, dummy)) { printf("PARSE accepte [%s]\n", b); s_fail++; }
    // series E
    if (eNearest(E24, 4800) != 4700) { printf("E24 4800\n"); s_fail++; }
    if (fabs(eStep(E24, 4700, 1) - 5100) > 1e-9) { printf("E24 step\n"); s_fail++; }
    if (fabs(eStep(E24, 1000, -1) - 910) > 1e-9) { printf("E24 step-\n"); s_fail++; }
    if (fabs(eStep(E12, 82e-9, 1) - 100e-9) > 1e-18) { printf("E12 step\n"); s_fail++; }
    if (fabs(eCeil(E24, 433) - 470) > 1e-9 || fabs(eFloor(E24, 433) - 430) > 1e-9) { printf("E24 ceil/floor\n"); s_fail++; }
    if (fabs(step125(1000, 1) - 2000) > 1e-9 || fabs(step125(1000, -1) - 500) > 1e-9) { printf("125\n"); s_fail++; }
    for (int s = 0; s < ESERIES_COUNT; s++)
        for (int i = 1; i < eCount(s); i++)
            if (eMantissa(s, i) <= eMantissa(s, i - 1)) { printf("serie %d non croissante\n", s); s_fail++; }
    // E96 : ecart a 10^(i/96) < 1 %
    for (int i = 0; i < 96; i++) {
        double ideal = 100 * pow(10.0, i / 96.0);
        if (fabs(eMantissa(E96, i) - ideal) / ideal > 0.01) { printf("E96[%d]\n", i); s_fail++; }
    }
    // complexes
    Cx z = cx(3, 4) / cx(1, -2);
    if (fabs(z.re + 1) > 1e-15 || fabs(z.im - 2) > 1e-15) { printf("Cx div\n"); s_fail++; }
    printf(s_fail ? "%d ECHEC(S)\n" : "OK\n", s_fail);
    return s_fail != 0;
}
