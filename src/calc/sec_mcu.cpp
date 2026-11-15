// Section : timers et microcontroleurs (delai d'un timer avec diviseurs,
// registre, offset et plafond ; reglage d'un delai ; PWM ; liaison serie).
#include "CalcUtil.hpp"

using namespace cu;

namespace mcu {

static double pow2(int n) { double p = 1; for (int k = 0; k < n; k++) p *= 2; return p; }

// Texte d'un entier en hexadecimal et binaire pour un registre de n bits.
static void regText(char *out, int cap, double v, int n)
{
    int k = 0;
    if (!(v >= 0) || v >= pow2(n > 32 ? 32 : n)) { scopy(out, cap, "\\text{---}"); return; }
    uint32_t x = (uint32_t)(long long)(v + 0.5);
    k += scopy(out + k, cap - k, "\\text{0x");
    int hd = (n + 3) / 4;
    for (int i = hd - 1; i >= 0 && k < cap - 2; i--) out[k++] = "0123456789ABCDEF"[(x >> (4 * i)) & 15];
    k += scopy(out + k, cap - k, "}\\;\\;\\text{");
    for (int i = n - 1; i >= 0 && k < cap - 12; i--) {
        out[k++] = (char)('0' + ((x >> i) & 1));
        if (i && i % 4 == 0 && n > 4) k += scopy(out + k, cap - k, "}\\,\\text{");
    }
    if (k < cap - 1) out[k++] = '}';
    out[k] = 0;
}

static void valText(Sch &g, int x, int y, const char *name, double v, uint8_t unit, int align = 0, bool bold = false,
                    bool integer = false)
{
    char buf[48];
    int k = 0;
    buf[0] = 0;
    if (name) { k = scopy(buf, sizeof buf, name); k += scopy(buf + k, (int)sizeof(buf) - k, " = "); }
    if (integer) fmtInt(buf + k, (int)sizeof(buf) - k, v);
    else fmtValue(buf + k, (int)sizeof(buf) - k, v, unit, 4);
    int f = bold ? BOLD_12 : SANS_12;
    int w = font::richWidth(f, buf);
    if (align == 1) x -= w / 2;
    else if (align == 2) x -= w;
    if (x + w > 177) x = 177 - w;
    if (x < 2) x = 2;
    font::drawRich(*g.cv, f, g.ox + x, g.oy + y, buf, g.volt);
}

// Petit signal carre (horloge) dans un rectangle.
static void clockBox(Sch &g, int x, int y, int w, int h, int tag)
{
    g.chip(x, y, w, h, nullptr, tag);
    uint16_t c = g.col(tag);
    int n = 3, sw = (w - 6) / (2 * n);
    int yl = y + h - 5, yh = y + 5;
    int px = x + 3;
    for (int k = 0; k < n; k++) {
        g.line(px, yl, px, yh, c);
        g.line(px, yh, px + sw, yh, c);
        g.line(px + sw, yh, px + sw, yl, c);
        g.line(px + sw, yl, px + 2 * sw, yl, c);
        px += 2 * sw;
    }
}
static void arrowR(Sch &g, int x0, int x1, int y, uint16_t c)
{
    g.line(x0, y, x1 - 3, y, c);
    g.cv->arrowHead(g.ox + x1, g.oy + y, 1, 0, 4, c);
}
static void arrowD(Sch &g, int x, int y0, int y1, uint16_t c)
{
    g.line(x, y0, x, y1 - 3, c);
    g.cv->arrowHead(g.ox + x, g.oy + y1, 0, 1, 4, c);
}

// Chaine horloge -> diviseurs -> registre -> interruption.
struct Chain {
    double fclk, div[3];
    int ndiv, bits;
    double offset, top, ticks;
    int tClk, tDiv[3], tReg, tOff, tTop;
};
static void drawChain(Sch &g, const Chain &c)
{
    const int y = 22;
    clockBox(g, 4, y - 10, 30, 20, c.tClk);
    g.texSmall(19, y + 24, "f_{clk}", tex::CENTER, c.tClk);
    int x = 34;
    for (int k = 0; k < c.ndiv; k++) {
        arrowR(g, x, x + 10, y, g.ink);
        x += 10;
        bool one = !(c.div[k] > 1);
        char t[16] = "\xC3\xB7";
        fmtInt(t + 2, 14, c.div[k]);
        int w = font::width(BOLD_12, t) + 8;
        if (w < 26) w = 26;
        g.chip(x, y - 10, w, 20, nullptr, c.tDiv[k]);
        font::drawText(*g.cv, BOLD_12, g.ox + x + (w - font::width(BOLD_12, t)) / 2, g.oy + y + 5, t,
                       one ? mix16(g.paper, g.ink, 6) : g.col(c.tDiv[k]));
        x += w;
    }
    arrowR(g, x, x + 10, y, g.ink);
    g.texSmall(x + 12, y + 4, "f_t", tex::LEFT);
    // registre
    int n = c.bits;
    int shown = n > 16 ? 16 : n;
    int cw = shown > 8 ? 160 / shown : 18;
    if (cw > 18) cw = 18;
    int W = shown * cw, rx = (179 - W) / 2, ry = 64;
    g.line(x + 12, y + 8, x + 12, y + 18, g.ink);
    g.line(x + 12, y + 18, rx + W / 2, y + 18, g.ink);
    arrowD(g, rx + W / 2, y + 18, ry - 2, g.ink);
    uint32_t off = (uint32_t)(long long)(c.offset >= 0 ? c.offset + 0.5 : 0);
    for (int i = 0; i < shown; i++) {
        int bit = shown - 1 - i;
        bool one = (off >> bit) & 1;
        int xx = rx + i * cw;
        uint16_t bg = one ? (g.on(c.tOff) ? g.accent : mix16(g.paper, g.accent, 9)) : g.paper;
        g.rect(xx, ry, cw - 1, 16, bg);
        g.frame(xx, ry, cw, 17, g.col(c.tReg));
        char t[2] = {(char)(one ? '1' : '0'), 0};
        font::drawText(*g.cv, SANS_12, g.ox + xx + (cw - font::width(SANS_12, t)) / 2, g.oy + ry + 13, t,
                       one ? col::white : g.ink);
    }
    char rl[32];
    int k = scopy(rl, sizeof rl, "registre ");
    k += fmtInt(rl + k, (int)sizeof(rl) - k, n);
    scopy(rl + k, (int)sizeof(rl) - k, n > 16 ? " bits (16 de poids faible)" : " bits");
    font::drawText(*g.cv, SANS_12, g.ox + rx, g.oy + ry + 30, rl, g.col(c.tReg));
    // interruption
    int iy = 126;
    arrowD(g, rx + W / 2, ry + 18, iy - 9, g.ink);
    g.chip(rx + W / 2 - 22, iy - 9, 44, 18, nullptr, 0);
    font::drawText(*g.cv, BOLD_12, g.ox + rx + W / 2 - font::width(BOLD_12, "IRQ") / 2, g.oy + iy + 4, "IRQ", rgb(200, 40, 40));
}

}  // namespace mcu

using namespace mcu;

// ==========================================================================
// Timer : delai entre interruptions
// ==========================================================================
namespace tim {
enum { FCLK, D1, D2, D3, BITS, OFF, TOP, K, OFT, OTT, OTICK, OT, OF, OTOT, OTMAX };
enum { CMODE };
enum { T_CLK = 1, T_D1, T_D2, T_D3, T_REG, T_OFF, T_TOP, T_K };
static char s_off[160], s_top[160];
static void def(CalcState &s)
{
    s.v[FCLK] = 16e6; s.v[D1] = 2; s.v[D2] = 4; s.v[D3] = 1; s.v[BITS] = 8; s.v[OFF] = 57; s.v[TOP] = 255; s.v[K] = 1;
}
static void rows(const CalcState &s, RowList &r)
{
    bool cmp = s.c[CMODE] == 1;
    r.in("f_{clk}", FCLK, U_HZ, ST_125, T_CLK, "Fréquence de l'horloge d'entrée (quartz, horloge système)");
    r.in("\\div_1", D1, U_NONE, ST_POW2, T_D1, "Premier diviseur (1 : aucun)", RF_POS | RF_INT);
    r.in("\\div_2", D2, U_NONE, ST_POW2, T_D2, "Deuxième diviseur (prédiviseur du timer…)", RF_POS | RF_INT);
    r.in("\\div_3", D3, U_NONE, ST_POW2, T_D3, "Troisième diviseur (1 : aucun)", RF_POS | RF_INT);
    r.in("n", BITS, U_NONE, ST_INT, T_REG, "Taille du registre de comptage (8, 16, 32 bits…)", RF_POS | RF_INT);
    r.choice("#Mode", CMODE, "débordement|comparaison (plafond)", T_REG, "Débordement : compte jusqu'à 2ⁿ − 1 ; comparaison : jusqu'au plafond");
    r.in("#offset", OFF, U_NONE, ST_INT, T_OFF, "Valeur de départ chargée dans le registre (préchargement)", RF_NONNEG | RF_INT);
    if (cmp) r.in("#plafond", TOP, U_NONE, ST_INT, T_TOP, "Valeur de comparaison (OCR, ARR) : remise à l'offset après", RF_NONNEG | RF_INT);
    r.in("k", K, U_NONE, ST_INT, T_K, "Nombre d'interruptions comptées par logiciel (1 : aucune)", RF_POS | RF_INT);
    r.head("Résultats");
    r.out("f_t", OFT, U_HZ, "Fréquence de comptage f_clk / (÷₁ ÷₂ ÷₃)");
    r.out("T_{tick}", OTT, U_S, "Durée d'un pas de comptage (résolution)");
    r.outI("#ticks", OTICK, cmp ? "Pas par période : plafond − offset + 1" : "Pas avant débordement : 2ⁿ − offset", true);
    r.out("T", OT, U_S, "Délai entre deux interruptions", true, T_REG);
    r.out("f", OF, U_HZ, "Fréquence des interruptions 1/T");
    r.out("k\\,T", OTOT, U_S, "Délai avec k interruptions comptées", false, T_K);
    r.out("T_{\\max}", OTMAX, U_S, "Délai maximal (offset 0, registre complet)");
    r.texOut("#offset", 0, "Offset en hexadécimal et en binaire", T_OFF);
    if (cmp) r.texOut("#plafond", 1, "Plafond en hexadécimal et en binaire", T_TOP);
    r.msg(2);
}
static void calc(CalcState &s)
{
    int n = (int)s.v[BITS];
    if (n > 32) { n = 32; s.v[BITS] = 32; }
    double full = pow2(n);
    double div = s.v[D1] * s.v[D2] * s.v[D3];
    s.v[OFT] = s.v[FCLK] / div;
    s.v[OTT] = 1 / s.v[OFT];
    bool cmp = s.c[CMODE] == 1;
    double ticks = cmp ? s.v[TOP] - s.v[OFF] + 1 : full - s.v[OFF];
    s.v[OTICK] = ticks;
    s.v[OT] = ticks * s.v[OTT];
    s.v[OF] = 1 / s.v[OT];
    s.v[OTOT] = s.v[K] * s.v[OT];
    s.v[OTMAX] = full * s.v[OTT];
    regText(s_off, sizeof s_off, s.v[OFF], n);
    regText(s_top, sizeof s_top, s.v[TOP], n);
    setMsg(s, 0, s_off, ML_INFO);
    setMsg(s, 1, s_top, ML_INFO);
    if (s.v[OFF] >= full) { setMsg(s, 2, "Offset trop grand pour le registre", ML_BAD); s.v[OTICK] = s.v[OT] = s.v[OF] = s.v[OTOT] = NaN(); }
    else if (cmp && s.v[TOP] >= full) { setMsg(s, 2, "Plafond trop grand pour le registre", ML_BAD); s.v[OTICK] = s.v[OT] = s.v[OF] = s.v[OTOT] = NaN(); }
    else if (cmp && s.v[TOP] < s.v[OFF]) { setMsg(s, 2, "Il faut plafond ≥ offset", ML_BAD); s.v[OTICK] = s.v[OT] = s.v[OF] = s.v[OTOT] = NaN(); }
    else if (!cmp && s.v[OFF] > 0) setMsg(s, 2, "Recharger l'offset dans la routine d'interruption", ML_INFO);
    else setMsg(s, 2, "F4 : chronogramme du compteur", ML_INFO);
}
static void sch(Sch &g, const CalcState &s)
{
    Chain c{s.v[FCLK], {s.v[D1], s.v[D2], s.v[D3]}, 3, (int)s.v[BITS], s.v[OFF], s.v[TOP], s.v[OTICK],
            T_CLK, {T_D1, T_D2, T_D3}, T_REG, T_OFF, T_TOP};
    drawChain(g, c);
    valText(g, 6, 152, "T", s.v[OT], U_S, 0, true);
    valText(g, 177, 152, "f", s.v[OF], U_HZ, 2);
    valText(g, 6, 168, "ticks", s.v[OTICK], U_NONE, 0, false, true);
    valText(g, 177, 168, "f_t", s.v[OFT], U_HZ, 2);
}
static void graph(Canvas &cv, const CalcState &s, const GraphArea &a, uint16_t accent)
{
    double ticks = s.v[OTICK];
    if (!(ticks >= 1)) return;
    int n = (int)s.v[BITS];
    double full = pow2(n);
    bool cmp = s.c[CMODE] == 1;
    double top = cmp ? s.v[TOP] : full - 1, off = s.v[OFF];
    const int X0 = a.x + 56, X1 = a.x + a.w - 12, Y0 = a.y + 22, Y1 = a.y + 120;
    const int periods = 3;
    double tot = ticks * periods;
    uint16_t axc = rgb(150, 156, 170), grid = rgb(226, 230, 238);
    cv.fillRect(X0, Y0, 1, Y1 - Y0 + 1, axc);
    cv.fillRect(X0, Y1, X1 - X0, 1, axc);
    auto py = [&](double v) { return Y1 - (int)((v / (full - 1 > 0 ? full - 1 : 1)) * (Y1 - Y0)); };
    // niveaux
    char buf[32];
    for (int k = 0; k < 2; k++) {
        double v = k ? top : off;
        int y = py(v);
        for (int x = X0; x < X1; x += 3) cv.fillRect(x, y, 1, 1, grid);
        fmtInt(buf, sizeof buf, v);
        font::drawText(cv, SANS_12, X0 - 4 - font::width(SANS_12, buf), y + 4, buf, col::inkSoft);
    }
    tex::draw(cv, "#compteur", X0 + 4, Y0 + 10, col::inkSoft, tex::LEFT, 2);
    // dents de scie (marches si peu de pas)
    int px = X0, pyv = py(off);
    for (int p = 0; p < periods; p++) {
        double t0 = p * ticks;
        int xs = X0 + (int)((t0 / tot) * (X1 - X0)), xe = X0 + (int)(((t0 + ticks) / tot) * (X1 - X0));
        if (ticks <= 24) {
            for (int k = 0; k < (int)ticks; k++) {
                int xa = X0 + (int)(((t0 + k) / tot) * (X1 - X0)), xb = X0 + (int)(((t0 + k + 1) / tot) * (X1 - X0));
                int y = py(off + k);
                cv.line(px, pyv, xa, y, accent);
                cv.line(xa, y, xb, y, accent);
                px = xb;
                pyv = y;
            }
        } else {
            cv.line(px, pyv, xs, py(off), accent);
            cv.line(xs, py(off), xe, py(top), accent);
            cv.line(xs, py(off) + 1, xe, py(top) + 1, accent);
            px = xe;
            pyv = py(top);
        }
        // impulsion d'interruption
        int yi0 = a.y + 150, yi1 = a.y + 132;
        cv.fillRect(xe, yi1, 2, yi0 - yi1, rgb(200, 40, 40));
    }
    cv.fillRect(X0, a.y + 150, X1 - X0, 1, axc);
    tex::draw(cv, "#IRQ", X0 - 6, a.y + 150, rgb(200, 40, 40), tex::RIGHT, 2);
    // cote T
    int xa = X0 + (int)((ticks / tot) * (X1 - X0)), xb = X0 + (int)((2 * ticks / tot) * (X1 - X0));
    cv.fillRect(xa, a.y + 162, xb - xa, 1, accent);
    cv.arrowHead(xa, a.y + 162, -1, 0, 4, accent);
    cv.arrowHead(xb, a.y + 162, 1, 0, 4, accent);
    int k = scopy(buf, sizeof buf, "T = ");
    fmtValue(buf + k, (int)sizeof(buf) - k, s.v[OT], U_S);
    font::drawText(cv, BOLD_12, (xa + xb) / 2 - font::width(BOLD_12, buf) / 2, a.y + 178, buf, accent);
}
}  // namespace tim

// ==========================================================================
// Timer : reglage d'un delai voulu
// ==========================================================================
namespace tset {
enum { FCLK, T, BITS, OP, OPSC, OTICK, OREG, OTR, OERR, OK, OFT };
enum { CFAM, CMODE };
enum { T_CLK = 1, T_D, T_REG };
static char s_reg[160], s_msg[80];
static const char *const FAMS = "AVR : 1, 8, 64, 256, 1024|AVR timer 2 : 1 à 1024|PIC : 1 à 256 (×2)|STM32 : 1 à 65 536|libre : puissances de 2";
static const int AVR[] = {1, 8, 64, 256, 1024};
static const int AVR2[] = {1, 8, 32, 64, 128, 256, 1024};
static void def(CalcState &s) { s.v[FCLK] = 16e6; s.v[T] = 1e-3; s.v[BITS] = 8; }
static void rows(const CalcState &s, RowList &r)
{
    bool cmp = s.c[CMODE] == 1;
    r.in("f_{clk}", FCLK, U_HZ, ST_125, T_CLK, "Fréquence d'horloge du timer");
    r.in("T", T, U_S, ST_125, 0, "Délai voulu entre deux interruptions");
    r.in("n", BITS, U_NONE, ST_INT, T_REG, "Taille du registre de comptage", RF_POS | RF_INT);
    r.choice("#Prédiviseurs", CFAM, FAMS, T_D, "Valeurs de prédiviseur disponibles");
    r.choice("#Mode", CMODE, "débordement (offset)|comparaison (plafond)", T_REG);
    r.head("Réglage");
    r.out("N", OP, U_NONE, "Prédiviseur choisi (le plus petit qui convient : meilleure résolution)", true, T_D);
    if (s.c[CFAM] == 3) r.outI("PSC", OPSC, "Registre du prédiviseur STM32 = N − 1");
    r.outI("#ticks", OTICK, "Nombre de pas par période", false);
    r.outI(cmp ? (s.c[CFAM] == 3 ? "ARR" : "#plafond") : "#offset", OREG,
           cmp ? "Valeur de comparaison = ticks − 1 (comptage depuis 0)" : "Préchargement = 2ⁿ − ticks", true, T_REG);
    r.texOut("", 0, "Registre en hexadécimal et binaire", T_REG);
    r.out("k", OK, U_NONE, "Interruptions à compter par logiciel (délai trop long)");
    r.head("Délai obtenu");
    r.out("T", OTR, U_S, "Délai réel", true);
    r.out("\\varepsilon", OERR, U_PCT, "Erreur relative sur le délai");
    r.out("f_t", OFT, U_HZ, "Fréquence de comptage f_clk / N");
    r.msg(1);
}
static int family(int fam, int *list)
{
    switch (fam) {
    case 0: for (int i = 0; i < 5; i++) list[i] = AVR[i]; return 5;
    case 1: for (int i = 0; i < 7; i++) list[i] = AVR2[i]; return 7;
    case 2: for (int i = 0; i < 9; i++) list[i] = 1 << i; return 9;
    case 4: for (int i = 0; i < 11; i++) list[i] = 1 << i; return 11;
    default: return 0;
    }
}
static void calc(CalcState &s)
{
    int n = (int)s.v[BITS];
    if (n > 32) { n = 32; s.v[BITS] = 32; }
    double full = pow2(n);
    double Nt = s.v[T] * s.v[FCLK];   // pas d'horloge voulus
    double p = 0, ticks = 0, k = 1;
    if (s.c[CFAM] == 3) {
        double pmin = fm::ceil(Nt / full - 1e-12);
        if (pmin < 1) pmin = 1;
        if (pmin > 65536) {
            k = fm::ceil(Nt / (65536 * full) - 1e-12);
            pmin = fm::ceil(Nt / k / full - 1e-12);
            if (pmin > 65536) pmin = 65536;
        }
        // prediviseur entier quelconque : le plus petit donnant l'erreur minimale
        double nk = Nt / k, best = 1e300;
        for (double q = pmin; q <= 65536 && q <= pmin + 2000; q += 1) {
            double t = fm::floor(nk / q + 0.5);
            if (t < 1 || t > full) continue;
            double e = fm::fabs(t * q - nk);
            if (e < best - 1e-9 * nk) { best = e; p = q; ticks = t; }
            if (e <= 1e-9 * nk) break;
        }
        if (!(p >= 1)) { p = pmin; ticks = fm::floor(nk / p + 0.5); }
    } else {
        int list[12];
        int cnt = family(s.c[CFAM], list);
        bool found = false;
        for (int i = 0; i < cnt; i++) {
            double t = fm::floor(Nt / list[i] + 0.5);
            if (t >= 1 && t <= full) { p = list[i]; ticks = t; found = true; break; }
        }
        if (!found) {
            p = list[cnt - 1];
            if (Nt / p < 1) ticks = 1;
            else {
                k = fm::ceil(Nt / (p * full) - 1e-12);
                ticks = fm::floor(Nt / k / p + 0.5);
            }
        }
    }
    if (ticks > full) ticks = full;
    if (ticks < 1) ticks = 1;
    s.v[OP] = p;
    s.v[OPSC] = p - 1;
    s.v[OTICK] = ticks;
    s.v[OREG] = s.c[CMODE] == 1 ? ticks - 1 : full - ticks;
    s.v[OK] = k;
    s.v[OFT] = s.v[FCLK] / p;
    s.v[OTR] = ticks * p / s.v[FCLK];
    double tot = s.v[OTR] * k;
    s.v[OERR] = (tot - s.v[T]) / s.v[T] * 100;
    regText(s_reg, sizeof s_reg, s.v[OREG], n);
    setMsg(s, 0, s_reg, ML_INFO);
    if (k > 1) {
        int m = scopy(s_msg, sizeof s_msg, "Délai trop long : compter ");
        m += fmtInt(s_msg + m, (int)sizeof(s_msg) - m, k);
        scopy(s_msg + m, (int)sizeof(s_msg) - m, " interruptions de T");
        setMsg(s, 1, s_msg, ML_WARN);
    } else if (fm::fabs(s.v[OERR]) > 1) setMsg(s, 1, "Erreur > 1 % : changer f_clk ou le prédiviseur", ML_WARN);
    else setMsg(s, 1, "Réglage exact ou très proche", ML_OK);
}
static void sch(Sch &g, const CalcState &s)
{
    Chain c{s.v[FCLK], {s.v[OP], 1, 1}, 1, (int)s.v[BITS], s.c[CMODE] == 1 ? 0 : s.v[OREG], s.v[OREG], s.v[OTICK],
            T_CLK, {T_D, 0, 0}, T_REG, T_REG, T_REG};
    drawChain(g, c);
    valText(g, 6, 152, "T", s.v[OTR], U_S, 0, true);
    valText(g, 177, 152, "\xCE\xB5", s.v[OERR], U_PCT, 2);
    valText(g, 6, 168, s.c[CMODE] == 1 ? "plafond" : "offset", s.v[OREG], U_NONE, 0, false, true);
}
}  // namespace tset

// ==========================================================================
// PWM
// ==========================================================================
namespace pwm {
enum { FCLK, P, TOP, DUTY, VCC, OF, OOCR, ODUTY, ORES, OVM, OT };
enum { CMODE };
enum { T_CLK = 1, T_P, T_TOP, T_OCR };
static void def(CalcState &s) { s.v[FCLK] = 16e6; s.v[P] = 8; s.v[TOP] = 255; s.v[DUTY] = 25; s.v[VCC] = 5; }
static void rows(const CalcState &, RowList &r)
{
    r.in("f_{clk}", FCLK, U_HZ, ST_125, T_CLK, "Fréquence d'horloge");
    r.in("N", P, U_NONE, ST_POW2, T_P, "Prédiviseur", RF_POS | RF_INT);
    r.choice("#Mode", CMODE, "rapide (dent de scie)|phase correcte (triangle)", T_TOP, "Comptage montant seul ou montant puis descendant");
    r.in("#TOP", TOP, U_NONE, ST_INT, T_TOP, "Valeur maximale du compteur (255 : 8 bits)", RF_POS | RF_INT);
    r.in("\\alpha", DUTY, U_PCT, ST_LIN1, T_OCR, "Rapport cyclique voulu", RF_NONNEG);
    r.in("V_{CC}", VCC, U_V, ST_LIN01, 0, "Tension de la sortie à l'état haut");
    r.head("Résultats");
    r.out("f_{PWM}", OF, U_HZ, "Fréquence du signal", true);
    r.out("T", OT, U_S, "Période");
    r.outI("#OCR", OOCR, "Valeur de comparaison à programmer", true, T_OCR);
    r.out("\\alpha", ODUTY, U_PCT, "Rapport cyclique obtenu");
    r.out("n", ORES, U_NONE, "Résolution en bits log₂(TOP + 1)");
    r.out("V_{moy}", OVM, U_V, "Valeur moyenne α V_CC (après filtrage)");
}
static void calc(CalcState &s)
{
    double top = s.v[TOP], p = s.v[P], a = s.v[DUTY] / 100;
    if (a > 1) a = 1;
    bool fast = s.c[CMODE] == 0;
    s.v[OF] = fast ? s.v[FCLK] / (p * (top + 1)) : s.v[FCLK] / (2 * p * top);
    s.v[OT] = 1 / s.v[OF];
    double ocr = fast ? fm::floor(a * (top + 1) + 0.5) - 1 : fm::floor(a * top + 0.5);
    if (ocr < (fast ? -1 : 0)) ocr = fast ? -1 : 0;
    if (ocr > top) ocr = top;
    s.v[OOCR] = ocr < 0 ? 0 : ocr;
    s.v[ODUTY] = fast ? (ocr + 1) / (top + 1) * 100 : ocr / top * 100;
    s.v[ORES] = fm::ln(top + 1) / fm::ln(2.0);
    s.v[OVM] = s.v[ODUTY] / 100 * s.v[VCC];
}
static void sch(Sch &g, const CalcState &s)
{
    // compteur et seuil, sortie
    const int X0 = 14, X1 = 168, Y0 = 26, Y1 = 92, YO0 = 112, YO1 = 140;
    bool fast = s.c[CMODE] == 0;
    double top = s.v[TOP], ocr = s.v[OOCR];
    uint16_t c = g.col(T_TOP), co = g.col(T_OCR);
    g.line(X0, Y1, X1, Y1, g.ink);
    g.line(X0, Y0 - 4, X0, Y1, g.ink);
    int yO = Y1 - (int)(ocr / top * (Y1 - Y0));
    for (int x = X0; x < X1; x += 3) g.rect(x, yO, 1, 1, co);
    const int per = 3;
    double w = (double)(X1 - X0) / per;
    for (int p = 0; p < per; p++) {
        int xa = X0 + (int)(p * w), xb = X0 + (int)((p + 1) * w), xm = (xa + xb) / 2;
        if (fast) {
            g.line(xa, Y1, xb, Y0, c);
            g.line(xb, Y0, xb, Y1, c);
        } else {
            g.line(xa, Y1, xm, Y0, c);
            g.line(xm, Y0, xb, Y1, c);
        }
        // sortie haute tant que compteur <= OCR
        double fr = fast ? (ocr + 1) / (top + 1) : ocr / top;
        int hx0, hx1;
        if (fast) { hx0 = xa; hx1 = xa + (int)(fr * (xb - xa)); }
        else { hx0 = xm - (int)(fr * (xm - xa)); hx1 = xm + (int)(fr * (xb - xm)); }
        if (fast) {
            g.line(hx0, YO0, hx1, YO0, g.accent);
            g.line(hx1, YO0, hx1, YO1, g.accent);
            g.line(hx1, YO1, xb, YO1, g.accent);
            g.line(xb, YO1, xb, YO0, g.accent);
        } else {
            g.line(xa, YO1, hx0, YO1, g.accent);
            g.line(hx0, YO1, hx0, YO0, g.accent);
            g.line(hx0, YO0, hx1, YO0, g.accent);
            g.line(hx1, YO0, hx1, YO1, g.accent);
            g.line(hx1, YO1, xb, YO1, g.accent);
        }
    }
    g.texSmall(X0 + 3, Y0 + 4, "#TOP", tex::LEFT, T_TOP);
    g.texSmall(X1, yO - 3, "#OCR", tex::RIGHT, T_OCR);
    g.texSmall(X0 + 3, YO0 - 4, "#sortie", tex::LEFT);
    mcu::valText(g, 6, 162, "f", s.v[OF], U_HZ, 0, true);
    mcu::valText(g, 177, 162, "\xCE\xB1", s.v[ODUTY], U_PCT, 2, true);
}
}  // namespace pwm

// ==========================================================================
// Liaison serie (UART) : registre de debit et erreur
// ==========================================================================
namespace uart {
enum { FCLK, BAUD, DATA, OUBRR, OBAUD, OERR, OBITS, OTB, OTBYTE, ORATE };
enum { COVS, CPAR, CSTOP };
enum { T_START = 1, T_DATA, T_PAR, T_STOP };
static const double BAUDS[] = {300, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 250000, 460800, 500000, 921600, 1e6};
static void def(CalcState &s) { s.v[FCLK] = 16e6; s.v[BAUD] = 9600; s.v[DATA] = 8; }
static void rows(const CalcState &, RowList &r)
{
    r.in("f_{clk}", FCLK, U_HZ, ST_125, 0, "Fréquence d'horloge du périphérique");
    r.in("#débit", BAUD, U_BAUD, ST_NONE, 0, "Débit voulu (9600, 115 200…)");
    r.choice("#Suréchantillonnage", COVS, "16|8", 0, "16 (normal) ou 8 (double vitesse, U2X)");
    r.in("#bits", DATA, U_NONE, ST_INT, T_DATA, "Bits de données (5 à 9)", RF_POS | RF_INT);
    r.choice("#Parité", CPAR, "aucune|paire|impaire", T_PAR);
    r.choice("#Stop", CSTOP, "1 bit|2 bits", T_STOP);
    r.head("Résultats");
    r.outI("#UBRR", OUBRR, "Registre de débit f_clk / (k débit) − 1, arrondi", true);
    r.out("#débit réel", OBAUD, U_BAUD, "f_clk / (k (UBRR + 1))");
    r.out("\\varepsilon", OERR, U_PCT, "Erreur de débit (≤ 2 % conseillé)", true);
    r.outI("#trame", OBITS, "Bits par trame : départ + données + parité + stop");
    r.out("T_b", OTB, U_S, "Durée d'un bit");
    r.out("T_{trame}", OTBYTE, U_S, "Durée d'une trame");
    r.out("#octets/s", ORATE, U_NONE, "Débit utile maximal (trames par seconde)");
    r.msg(0);
}
static void calc(CalcState &s)
{
    double k = s.c[COVS] ? 8 : 16;
    double u = fm::floor(s.v[FCLK] / (k * s.v[BAUD]) - 1 + 0.5);
    if (u < 0) u = 0;
    s.v[OUBRR] = u;
    s.v[OBAUD] = s.v[FCLK] / (k * (u + 1));
    s.v[OERR] = (s.v[OBAUD] - s.v[BAUD]) / s.v[BAUD] * 100;
    s.v[OBITS] = 1 + s.v[DATA] + (s.c[CPAR] ? 1 : 0) + (s.c[CSTOP] ? 2 : 1);
    s.v[OTB] = 1 / s.v[OBAUD];
    s.v[OTBYTE] = s.v[OBITS] * s.v[OTB];
    s.v[ORATE] = 1 / s.v[OTBYTE];
    double e = fm::fabs(s.v[OERR]);
    if (e <= 0.5) setMsg(s, 0, "Erreur très faible : liaison fiable", ML_OK);
    else if (e <= 2) setMsg(s, 0, "Erreur acceptable (≤ 2 %)", ML_OK);
    else setMsg(s, 0, "Erreur trop grande : changer de quartz ou de débit", ML_BAD);
    (void)BAUDS;
}
static void sch(Sch &g, const CalcState &s)
{
    // trame : repos haut, depart, donnees LSB d'abord, parite, stop
    int nd = (int)s.v[DATA];
    if (nd > 9) nd = 9;
    int nb = 1 + nd + (s.c[CPAR] ? 1 : 0) + (s.c[CSTOP] ? 2 : 1);
    const int X0 = 12, X1 = 172, yh = 46, yl = 70;
    double w = (double)(X1 - X0 - 16) / nb;
    static const int data[9] = {1, 0, 1, 0, 0, 1, 1, 0, 1};   // exemple : 0x65
    int x = X0, lvl = 1;
    g.line(x, yh, x + 8, yh, g.ink);
    x += 8;
    int ones = 0;
    for (int i = 0; i < nb; i++) {
        int v, tag;
        const char *lab;
        char tl[3] = {0, 0, 0};
        if (i == 0) { v = 0; tag = T_START; lab = "D"; }
        else if (i <= nd) { v = data[i - 1]; ones += v; tag = T_DATA; tl[0] = (char)('0' + i - 1); lab = tl; }
        else if (s.c[CPAR] && i == nd + 1) { v = (ones & 1) ^ (s.c[CPAR] == 2); tag = T_PAR; lab = "P"; }
        else { v = 1; tag = T_STOP; lab = "S"; }
        int xa = X0 + 8 + (int)(i * w), xb = X0 + 8 + (int)((i + 1) * w);
        uint16_t c = g.col(tag);
        if (v != lvl) g.line(xa, yh, xa, yl, c);
        g.line(xa, v ? yh : yl, xb, v ? yh : yl, c);
        g.line(xa, yl + 6, xa, yl + 10, g.label);
        int tw = font::width(SANS_12, lab);
        font::drawText(*g.cv, SANS_12, g.ox + (xa + xb) / 2 - tw / 2, g.oy + yl + 20, lab, c);
        lvl = v;
    }
    g.line(X1 - 8, yh, X1, yh, g.ink);
    g.texSmall(X0, yh - 6, "#repos", tex::LEFT);
    mcu::valText(g, 89, 120, "UBRR", s.v[OUBRR], U_NONE, 1, true, true);
    mcu::valText(g, 89, 138, "\xCE\xB5", s.v[OERR], U_PCT, 1, true);
    mcu::valText(g, 89, 156, "T_b", s.v[OTB], U_S, 1);
}
}  // namespace uart

extern const CalcDef g_calcMcu[] = {
    {"Timer : délai entre interruptions", "#Horloge, diviseurs, registre, offset, plafond", "timer compteur interruption delai prediviseur prescaler registre offset plafond debordement overflow ctc tick horloge", 13, G_CUSTOM, 0,
     tim::def, tim::rows, tim::calc, tim::sch, nullptr, nullptr, tim::graph, "Chrono."},
    {"Timer : réglage d'un délai", "#Prédiviseur et registre pour un délai voulu", "timer reglage delai prediviseur prescaler avr pic stm32 psc arr ocr", 13, G_NONE, 0,
     tset::def, tset::rows, tset::calc, tset::sch},
    {"PWM", "#Fréquence, rapport cyclique, valeur OCR", "pwm mli modulation largeur impulsion rapport cyclique ocr top frequence", 13, G_NONE, 0,
     pwm::def, pwm::rows, pwm::calc, pwm::sch},
    {"Liaison série (UART)", "#Registre de débit, erreur, durée d'une trame", "uart usart serie baud debit ubrr rs232 trame", 13, G_NONE, 0,
     uart::def, uart::rows, uart::calc, uart::sch},
};
extern const int g_calcMcuN = sizeof(g_calcMcu) / sizeof(g_calcMcu[0]);
