// Icones des sections (tuiles de l'accueil), centrees en (cx, cy).
#include "Sch.hpp"
#include "../gfx/Tex.hpp"
#include "../gfx/Color.hpp"
#include "../calc/Calc.hpp"

static Sch sch(Canvas &cv, uint16_t ink, uint16_t bg)
{
    Sch g;
    g.cv = &cv;
    g.ox = 0;
    g.oy = 0;
    g.ink = ink;
    g.accent = ink;
    g.light = bg;
    g.paper = bg;
    g.label = ink;
    g.volt = ink;
    g.hilite = 0;
    return g;
}

void iconAssoc(Canvas &cv, int cx, int cy, uint16_t ink, uint16_t bg)
{
    Sch g = sch(cv, ink, bg);
    g.wire(cx - 32, cy, cx - 20, cy);
    g.wire(cx + 20, cy, cx + 32, cy);
    g.wire(cx - 20, cy - 8, cx - 20, cy + 8);
    g.wire(cx + 20, cy - 8, cx + 20, cy + 8);
    g.res(cx - 20, cy - 8, cx + 20, cy - 8);
    g.res(cx - 20, cy + 8, cx + 20, cy + 8);
    g.dot(cx - 20, cy);
    g.dot(cx + 20, cy);
}

void iconRlc(Canvas &cv, int cx, int cy, uint16_t ink, uint16_t bg)
{
    Sch g = sch(cv, ink, bg);
    g.res(cx - 36, cy, cx - 12, cy);
    g.ind(cx - 12, cy, cx + 14, cy);
    g.cap(cx + 14, cy, cx + 36, cy);
}

void iconFilter(Canvas &cv, int cx, int cy, uint16_t ink, uint16_t bg)
{
    (void)bg;
    // axes
    cv.fillRect(cx - 30, cy - 14, 1, 28, ink);
    cv.fillRect(cx - 30, cy + 13, 62, 1, ink);
    // gain d'un passe-bas en echelle logarithmique
    static const int8_t pts[13] = {0, 0, 0, 0, 1, 2, 4, 7, 10, 13, 16, 19, 22};
    int px = 0, py = 0;
    for (int i = 0; i < 13; i++) {
        int x = (cx - 28 + i * 5) * 16, y = (cy - 9 + pts[i]) * 16;
        if (i) {
            cv.lineSub(px, py, x, y, ink);
            cv.lineSub(px, py + 10, x, y + 10, ink);
        }
        px = x;
        py = y;
    }
}

void iconAop(Canvas &cv, int cx, int cy, uint16_t ink, uint16_t bg)
{
    Sch g = sch(cv, ink, bg);
    int x = cx - 14;
    g.opamp(x, cy, 28, true, 0);
    int d = Sch::inDy(28);
    g.wire(x - 10, cy - d, x, cy - d);
    g.wire(x - 10, cy + d, x, cy + d);
    g.wire(x + 28, cy, x + 38, cy);
}

void iconBjt(Canvas &cv, int cx, int cy, uint16_t ink, uint16_t bg)
{
    Sch g = sch(cv, ink, bg);
    int x = cx - 14;
    g.wire(x - 10, cy, x, cy);
    g.bjt(x, cy, true, 0);
}

void iconFet(Canvas &cv, int cx, int cy, uint16_t ink, uint16_t bg)
{
    Sch g = sch(cv, ink, bg);
    int x = cx - 10;
    g.wire(x - 12, cy, x, cy);
    g.mosfet(x, cy, true, 0);
}

void iconOhm(Canvas &cv, int cx, int cy, uint16_t ink, uint16_t bg)
{
    (void)bg;
    tex::draw(cv, "U=R\\,I", cx, cy + 6, ink, tex::CENTER, 1);
}

void iconAlim(Canvas &cv, int cx, int cy, uint16_t ink, uint16_t bg)
{
    (void)bg;
    // redressement double alternance (fin) et tension filtree (epais)
    uint16_t thin = mix16(ink, bg, 9);
    int px = 0, py = 0;
    for (int i = 0; i <= 60; i++) {
        double t = i / 60.0 * 2 * fm::PI * 2;
        double v = fm::fabs(fm::sin(t));
        int x = (cx - 30 + i) * 16, y = (int)((cy + 12 - v * 22) * 16);
        if (i) cv.lineSub(px, py, x, y, thin);
        px = x;
        py = y;
    }
    // ondulation : decharge lineaire puis recharge
    int x0 = cx - 30 + 15;
    cv.lineSub((cx - 30) * 16, (cy + 12) * 16, x0 * 16, (cy - 10) * 16, ink);
    for (int k = 0; k < 2; k++) {
        int xa = x0 + 30 * k, xb = xa + 26;
        cv.lineSub(xa * 16, (cy - 10) * 16, xb * 16, (cy - 6) * 16, ink);
        cv.lineSub(xa * 16, (cy - 10) * 16 + 10, xb * 16, (cy - 6) * 16 + 10, ink);
        if (k == 0) cv.lineSub(xb * 16, (cy - 6) * 16, (xa + 30) * 16, (cy - 10) * 16, ink);
    }
    cv.fillRect(cx - 31, cy + 13, 62, 1, ink);
}

void iconCode(Canvas &cv, int cx, int cy, uint16_t ink, uint16_t bg)
{
    (void)bg;
    cv.fillRect(cx - 34, cy, 10, 1, ink);
    cv.fillRect(cx + 25, cy, 10, 1, ink);
    cv.roundRect(cx - 25, cy - 8, 50, 17, 4, rgb(222, 196, 150), true, mix16(ink, col::black, 6), true);
    static const uint16_t bands[4] = {rgb(130, 70, 20), rgb(20, 20, 20), rgb(220, 30, 30), rgb(200, 160, 40)};
    static const int bx[4] = {-17, -9, -1, 13};
    for (int i = 0; i < 4; i++) cv.fillRect(cx + bx[i], cy - 7, 4, 15, bands[i]);
}

void iconOsc(Canvas &cv, int cx, int cy, uint16_t ink, uint16_t bg)
{
    (void)bg;
    int x = cx - 32, hi = cy - 9, lo = cy + 9;
    for (int k = 0; k < 4; k++) {
        int y = (k & 1) ? lo : hi;
        cv.fillRect(x, y, 16, 2, ink);
        if (k < 3) cv.fillRect(x + 15, hi, 2, lo - hi + 2, ink);
        x += 16;
    }
}

void iconQuad(Canvas &cv, int cx, int cy, uint16_t ink, uint16_t bg)
{
    Sch g = sch(cv, ink, bg);
    g.frame(cx - 14, cy - 13, 29, 27, ink);
    g.wire(cx - 30, cy - 8, cx - 14, cy - 8);
    g.wire(cx - 30, cy + 8, cx - 14, cy + 8);
    g.wire(cx + 14, cy - 8, cx + 30, cy - 8);
    g.wire(cx + 14, cy + 8, cx + 30, cy + 8);
    g.term(cx - 30, cy - 8); g.term(cx - 30, cy + 8); g.term(cx + 30, cy - 8); g.term(cx + 30, cy + 8);
    tex::draw(cv, "#Q", cx, cy + 5, ink, tex::CENTER, 1);
}

void iconMag(Canvas &cv, int cx, int cy, uint16_t ink, uint16_t bg)
{
    (void)bg;
    for (int k = 0; k < 4; k++) {
        cv.halfCircle(cx - 10, cy - 12 + 4 + 8 * k - 4, 4, 1, ink);
        cv.halfCircle(cx + 10, cy - 12 + 4 + 8 * k - 4, 4, 3, ink);
    }
    cv.fillRect(cx - 2, cy - 15, 1, 30, ink);
    cv.fillRect(cx + 1, cy - 15, 1, 30, ink);
    cv.fillRect(cx - 24, cy - 16, 14, 1, ink);
    cv.fillRect(cx - 24, cy + 16, 14, 1, ink);
    cv.fillRect(cx + 11, cy - 16, 14, 1, ink);
    cv.fillRect(cx + 11, cy + 16, 14, 1, ink);
    cv.fillRect(cx - 10, cy - 16, 1, 3, ink);
    cv.fillRect(cx - 10, cy + 13, 1, 4, ink);
    cv.fillRect(cx + 10, cy - 16, 1, 3, ink);
    cv.fillRect(cx + 10, cy + 13, 1, 4, ink);
}

void iconLogic(Canvas &cv, int cx, int cy, uint16_t ink, uint16_t bg)
{
    (void)bg;
    // porte ET (forme americaine)
    int x0 = cx - 12, h = 13;
    cv.fillRect(x0, cy - h, 1, 2 * h + 1, ink);
    cv.fillRect(x0, cy - h, 12, 1, ink);
    cv.fillRect(x0, cy + h, 12, 1, ink);
    cv.halfCircle(x0 + 12, cy, h, 1, ink);
    cv.fillRect(x0 - 14, cy - 7, 14, 1, ink);
    cv.fillRect(x0 - 14, cy + 7, 14, 1, ink);
    cv.fillRect(x0 + 25, cy, 14, 1, ink);
}

void iconMcu(Canvas &cv, int cx, int cy, uint16_t ink, uint16_t bg)
{
    (void)bg;
    cv.roundRect(cx - 16, cy - 12, 33, 25, 2, ink, false, ink, true);
    for (int k = 0; k < 4; k++) {
        int x = cx - 11 + 7 * k;
        cv.fillRect(x, cy - 18, 2, 6, ink);
        cv.fillRect(x, cy + 13, 2, 6, ink);
    }
    // creneaux d'horloge
    static const int8_t sq[] = {0, 0, 1, 1, 0, 0, 1, 1, 0};
    for (int k = 0; k < 8; k++) {
        int x = cx - 10 + 3 * k, y = cy + (sq[k] ? -4 : 4);
        cv.fillRect(x, y, 3, 1, ink);
        if (sq[k] != sq[k + 1]) cv.fillRect(x + 3, cy - 4, 1, 9, ink);
    }
}

void iconMotor(Canvas &cv, int cx, int cy, uint16_t ink, uint16_t bg)
{
    cv.circle(cx, cy, 14, ink);
    cv.circle(cx, cy, 13, ink);
    tex::draw(cv, "#M", cx, cy + 6, ink, tex::CENTER, 1);
    cv.fillRect(cx - 30, cy, 16, 1, ink);
    cv.fillRect(cx + 15, cy, 16, 1, ink);
    (void)bg;
}

void iconRf(Canvas &cv, int cx, int cy, uint16_t ink, uint16_t bg)
{
    (void)bg;
    cv.line(cx - 12, cy - 16, cx, cy - 2, ink);
    cv.line(cx + 12, cy - 16, cx, cy - 2, ink);
    cv.fillRect(cx - 12, cy - 16, 25, 1, ink);
    cv.fillRect(cx, cy - 2, 1, 18, ink);
    for (int r = 8; r <= 16; r += 8) {
        cv.halfCircle(cx + 14, cy - 12, r / 2, 1, ink);
        cv.halfCircle(cx - 14, cy - 12, r / 2, 3, ink);
    }
    cv.fillRect(cx - 10, cy + 16, 21, 1, ink);
}

void iconSig(Canvas &cv, int cx, int cy, uint16_t ink, uint16_t bg)
{
    (void)bg;
    cv.fillRect(cx - 30, cy, 61, 1, mix16(bg, ink, 6));
    int px = 0, py = 0;
    for (int i = 0; i <= 60; i++) {
        double a = fm::TWO_PI * i / 40.0;
        int x = (cx - 30 + i) * 16, y = (int)((cy - 12 * fm::sin(a)) * 16);
        if (i) { cv.lineSub(px, py, x, y, ink); cv.lineSub(px, py + 8, x, y + 8, ink); }
        px = x; py = y;
    }
    for (int i = 0; i <= 60; i += 6) {
        double a = fm::TWO_PI * i / 40.0;
        int y = cy - fm::iround(12 * fm::sin(a));
        int y0 = y < cy ? y : cy, h = y < cy ? cy - y : y - cy;
        cv.fillRect(cx - 30 + i, y0, 1, h + 1, ink);
        cv.disc(cx - 30 + i, y, 2, ink);
    }
}
