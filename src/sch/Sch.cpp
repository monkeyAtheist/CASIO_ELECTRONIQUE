#include "Sch.hpp"
#include "../gfx/Tex.hpp"
#include "../calc/Calc.hpp"

static inline int iabs(int a) { return a < 0 ? -a : a; }
static inline int imin(int a, int b) { return a < b ? a : b; }
static inline int imax(int a, int b) { return a > b ? a : b; }
static inline int sgn(int a) { return a > 0 ? 1 : a < 0 ? -1 : 0; }

// --------------------------------------------------------------------------
// Primitives
// --------------------------------------------------------------------------
void Sch::line(int x0, int y0, int x1, int y1, uint16_t c)
{
    cv->line(ox + x0, oy + y0, ox + x1, oy + y1, c);
}

void Sch::rect(int x, int y, int w, int h, uint16_t c)
{
    cv->fillRect(ox + x, oy + y, w, h, c);
}

void Sch::frame(int x, int y, int w, int h, uint16_t c)
{
    cv->frame(ox + x, oy + y, w, h, c);
}

void Sch::wire(int x0, int y0, int x1, int y1, int tag)
{
    line(x0, y0, x1, y1, col(tag));
}

void Sch::wireHV(int x0, int y0, int x1, int y1, int tag)
{
    wire(x0, y0, x1, y0, tag);
    wire(x1, y0, x1, y1, tag);
}

void Sch::wireVH(int x0, int y0, int x1, int y1, int tag)
{
    wire(x0, y0, x0, y1, tag);
    wire(x0, y1, x1, y1, tag);
}

void Sch::dot(int x, int y)
{
    cv->disc(ox + x, oy + y, 2, ink);
}

void Sch::term(int x, int y)
{
    cv->disc(ox + x, oy + y, 2, paper);
    cv->circle(ox + x, oy + y, 2, ink);
}

void Sch::gnd(int x, int y, int tag)
{
    uint16_t c = col(tag);
    wire(x, y, x, y + 4, tag);
    rect(x - 7, y + 4, 15, 1, c);
    rect(x - 4, y + 7, 9, 1, c);
    rect(x - 1, y + 10, 3, 1, c);
}

void Sch::rail(int x, int y, const char *s, int tag)
{
    uint16_t c = col(tag);
    rect(x - 7, y - 1, 15, 2, c);
    if (s) tex(x, y - 4, s, tex::CENTER, tag);
}

void Sch::railDown(int x, int y, const char *s, int tag)
{
    uint16_t c = col(tag);
    rect(x - 7, y, 15, 2, c);
    if (s) tex(x, y + 15, s, tex::CENTER, tag);
}

void Sch::tex(int x, int base, const char *s, int align, int tag)
{
    tex::draw(*cv, s, ox + x, oy + base, on(tag) ? accent : label, align, 1);
}

void Sch::texSmall(int x, int base, const char *s, int align, int tag)
{
    tex::draw(*cv, s, ox + x, oy + base, on(tag) ? accent : label, align, 2);
}

// --------------------------------------------------------------------------
// Dipoles
// --------------------------------------------------------------------------
// Prepare un dipole : longueur, milieu ; dessine les fils jusqu'au corps de
// longueur 'body' centre. Renvoie la position du debut du corps (axe).
struct Span {
    bool h;
    int a0, a1;    // extremites sur l'axe (a0 < a1)
    int t;         // coordonnee transverse
    int mid;
};

static Span spanOf(int x0, int y0, int x1, int y1)
{
    Span s;
    s.h = (y0 == y1);
    if (s.h) { s.a0 = imin(x0, x1); s.a1 = imax(x0, x1); s.t = y0; }
    else { s.a0 = imin(y0, y1); s.a1 = imax(y0, y1); s.t = x0; }
    s.mid = (s.a0 + s.a1) / 2;
    return s;
}

// Fils de part et d'autre d'un corps [b0, b1].
static void leads(Sch &g, const Span &s, int b0, int b1, int tag)
{
    if (s.h) {
        if (b0 > s.a0) g.wire(s.a0, s.t, b0, s.t, tag);
        if (b1 < s.a1) g.wire(b1, s.t, s.a1, s.t, tag);
    } else {
        if (b0 > s.a0) g.wire(s.t, s.a0, s.t, b0, tag);
        if (b1 < s.a1) g.wire(s.t, b1, s.t, s.a1, tag);
    }
}

void Sch::res(int x0, int y0, int x1, int y1, int tag)
{
    Span s = spanOf(x0, y0, x1, y1);
    int len = s.a1 - s.a0;
    int bl = len >= 22 ? 16 : imax(8, len - 6);
    int b0 = s.mid - bl / 2, b1 = b0 + bl;
    leads(*this, s, b0, b1, tag);
    uint16_t c = col(tag);
    uint16_t f = on(tag) ? light : paper;
    if (s.h) {
        rect(b0, s.t - 3, bl, 7, f);
        frame(b0, s.t - 3, bl + 1, 7, c);
    } else {
        rect(s.t - 3, b0, 7, bl, f);
        frame(s.t - 3, b0, 7, bl + 1, c);
    }
}

void Sch::cap(int x0, int y0, int x1, int y1, int tag)
{
    Span s = spanOf(x0, y0, x1, y1);
    int b0 = s.mid - 2, b1 = s.mid + 3;   // armatures (2 px) separees de 3 px
    leads(*this, s, b0, b1, tag);
    uint16_t c = col(tag);
    if (on(tag)) {
        if (s.h) rect(b0, s.t - 5, 6, 11, light);
        else rect(s.t - 5, b0, 11, 6, light);
    }
    if (s.h) {
        rect(b0, s.t - 5, 2, 11, c);
        rect(b1 - 1, s.t - 5, 2, 11, c);
    } else {
        rect(s.t - 5, b0, 11, 2, c);
        rect(s.t - 5, b1 - 1, 11, 2, c);
    }
}

void Sch::ind(int x0, int y0, int x1, int y1, int tag)
{
    Span s = spanOf(x0, y0, x1, y1);
    const int n = 3, r = 3;
    int bl = 2 * r * n;
    int b0 = s.mid - bl / 2, b1 = b0 + bl;
    leads(*this, s, b0, b1, tag);
    uint16_t c = col(tag);
    if (on(tag)) {
        if (s.h) rect(b0, s.t - r - 1, bl + 1, r + 2, light);
        else rect(s.t, b0, r + 2, bl + 1, light);
    }
    for (int k = 0; k < n; k++) {
        int cc = b0 + r + 2 * r * k;
        if (s.h) cv->halfCircle(ox + cc, oy + s.t, r, 0, c);
        else cv->halfCircle(ox + s.t, oy + cc, r, 1, c);
    }
}

void Sch::zdip(int x0, int y0, int x1, int y1, int t, int tag)
{
    Span s = spanOf(x0, y0, x1, y1);
    switch (t) {
    case ZT_R: res(x0, y0, x1, y1, tag); return;
    case ZT_C: cap(x0, y0, x1, y1, tag); return;
    case ZT_L: ind(x0, y0, x1, y1, tag); return;
    case ZT_RC_S:
    case ZT_RL_S: {
        int m = s.mid;
        if (s.h) {
            res(s.a0, s.t, m, s.t, tag);
            if (t == ZT_RC_S) cap(m, s.t, s.a1, s.t, tag);
            else ind(m, s.t, s.a1, s.t, tag);
        } else {
            res(s.t, s.a0, s.t, m, tag);
            if (t == ZT_RC_S) cap(s.t, m, s.t, s.a1, tag);
            else ind(s.t, m, s.t, s.a1, tag);
        }
        return;
    }
    case ZT_RC_P:
    case ZT_RL_P: {
        const int d = 7;
        int p0 = s.a0 + 4, p1 = s.a1 - 4;
        if (s.h) {
            wire(s.a0, s.t, p0, s.t, tag);
            wire(p1, s.t, s.a1, s.t, tag);
            wire(p0, s.t - d, p0, s.t + d, tag);
            wire(p1, s.t - d, p1, s.t + d, tag);
            res(p0, s.t - d, p1, s.t - d, tag);
            if (t == ZT_RC_P) cap(p0, s.t + d, p1, s.t + d, tag);
            else ind(p0, s.t + d, p1, s.t + d, tag);
            dot(p0, s.t);
            dot(p1, s.t);
        } else {
            wire(s.t, s.a0, s.t, p0, tag);
            wire(s.t, p1, s.t, s.a1, tag);
            wire(s.t - d, p0, s.t + d, p0, tag);
            wire(s.t - d, p1, s.t + d, p1, tag);
            res(s.t - d, p0, s.t - d, p1, tag);
            if (t == ZT_RC_P) cap(s.t + d, p0, s.t + d, p1, tag);
            else ind(s.t + d, p0, s.t + d, p1, tag);
            dot(s.t, p0);
            dot(s.t, p1);
        }
        return;
    }
    default:
        res(x0, y0, x1, y1, tag);
    }
}

void Sch::diode(int x0, int y0, int x1, int y1, int kind, int tag)
{
    Span s = spanOf(x0, y0, x1, y1);
    int dir = s.h ? sgn(x1 - x0) : sgn(y1 - y0);   // sens anode -> cathode
    const int L = 8, W = 5;
    int b0 = s.mid - L / 2, b1 = b0 + L;
    leads(*this, s, b0, b1, tag);
    uint16_t c = col(tag);
    int base = dir > 0 ? b0 : b1, tip = dir > 0 ? b1 : b0;
    if (s.h) {
        cv->fillTri(ox + base, oy + s.t - W, ox + base, oy + s.t + W, ox + tip, oy + s.t, c);
        rect(tip - (dir > 0 ? 0 : 1), s.t - W, 2, 2 * W + 1, c);
        if (kind == 2) {   // Zener : crochets
            rect(tip - (dir > 0 ? 2 : -1), s.t - W, 2, 1, c);
            rect(tip + (dir > 0 ? 1 : -2), s.t + W, 2, 1, c);
        }
    } else {
        cv->fillTri(ox + s.t - W, oy + base, ox + s.t + W, oy + base, ox + s.t, oy + tip, c);
        rect(s.t - W, tip - (dir > 0 ? 0 : 1), 2 * W + 1, 2, c);
        if (kind == 2) {
            rect(s.t - W, tip - (dir > 0 ? 2 : -1), 1, 2, c);
            rect(s.t + W, tip + (dir > 0 ? 1 : -2), 1, 2, c);
        }
    }
    if (kind == 1) {   // LED : deux fleches vers l'exterieur
        for (int k = 0; k < 2; k++) {
            int ax, ay;
            if (s.h) { ax = s.mid - 2 + 5 * k; ay = s.t - W - 2; }
            else { ax = s.t + W + 2; ay = s.mid - 2 + 5 * k; }
            int ex = ax + (s.h ? 5 : 5), ey = ay - 5;
            line(ax, ay, ex, ey, c);
            cv->arrowHead(ox + ex, oy + ey, 1, -1, 4, c);
        }
    }
}

void Sch::vsrc(int x, int ytop, int ybot, int kind, int tag)
{
    int m = (ytop + ybot) / 2;
    const int r = 9;
    uint16_t c = col(tag);
    wire(x, ytop, x, m - r, tag);
    wire(x, m + r, x, ybot, tag);
    cv->disc(ox + x, oy + m, r, on(tag) ? light : paper);
    cv->circle(ox + x, oy + m, r, c);
    if (kind == 0) {
        // continue : + en haut, - en bas
        rect(x - 2, m - 5, 5, 1, c);
        rect(x, m - 7, 1, 5, c);
        rect(x - 2, m + 5, 5, 1, c);
    } else if (kind == 1) {
        // alternative : sinusoide
        int px = 0, py = 0;
        for (int i = 0; i <= 12; i++) {
            // i de 0 a 12 -> une periode sur 12 px
            static const int8_t sv[13] = {0, 2, 3, 4, 3, 2, 0, -2, -3, -4, -3, -2, 0};
            int X = (x - 6 + i) * 16, Y = (m - sv[i]) * 16;
            if (i) cv->lineSub(ox * 16 + px, oy * 16 + py, ox * 16 + X, oy * 16 + Y, c);
            px = X;
            py = Y;
        }
    } else {
        // courant : fleche vers le haut
        rect(x, m - 4, 1, 10, c);
        cv->arrowHead(ox + x, oy + m - 6, 0, -1, 4, c);
    }
}

void Sch::isrc(int x0, int y0, int x1, int y1, int tag)
{
    Span s = spanOf(x0, y0, x1, y1);
    const int r = 8;
    leads(*this, s, s.mid - r, s.mid + r, tag);
    uint16_t c = col(tag);
    int cx = s.h ? s.mid : s.t, cy = s.h ? s.t : s.mid;
    cv->disc(ox + cx, oy + cy, r, on(tag) ? light : paper);
    cv->circle(ox + cx, oy + cy, r, c);
    int d = s.h ? sgn(x1 - x0) : sgn(y1 - y0);
    if (s.h) {
        rect(cx - 4, cy, 8, 1, c);
        cv->arrowHead(ox + cx + 5 * d, oy + cy, d, 0, 4, c);
    } else {
        rect(cx, cy - 4, 1, 8, c);
        cv->arrowHead(ox + cx, oy + cy + 5 * d, 0, d, 4, c);
    }
}

void Sch::vcsrc(int x0, int y0, int x1, int y1, int tag)
{
    Span s = spanOf(x0, y0, x1, y1);
    const int r = 8;
    leads(*this, s, s.mid - r, s.mid + r, tag);
    uint16_t c = col(tag);
    int cx = s.h ? s.mid : s.t, cy = s.h ? s.t : s.mid;
    int X = ox + cx, Y = oy + cy;
    uint16_t f = on(tag) ? light : paper;
    cv->fillTri(X - r, Y, X, Y - r, X + r, Y, f);
    cv->fillTri(X - r, Y, X, Y + r, X + r, Y, f);
    cv->line(X - r, Y, X, Y - r, c);
    cv->line(X, Y - r, X + r, Y, c);
    cv->line(X + r, Y, X, Y + r, c);
    cv->line(X, Y + r, X - r, Y, c);
    bool plusFirst = s.h ? x0 < x1 : y0 < y1;
    if (s.h) {
        int px = plusFirst ? cx - 4 : cx + 4, mx = plusFirst ? cx + 4 : cx - 4;
        rect(px - 1, cy, 3, 1, c); rect(px, cy - 1, 1, 3, c);
        rect(mx - 1, cy, 3, 1, c);
    } else {
        int py = plusFirst ? cy - 4 : cy + 4, my = plusFirst ? cy + 4 : cy - 4;
        rect(cx - 1, py, 3, 1, c); rect(cx, py - 1, 1, 3, c);
        rect(cx - 1, my, 3, 1, c);
    }
}

void Sch::acH(int x0, int x1, int y, int tag)
{
    int m = (x0 + x1) / 2;
    const int r = 9;
    uint16_t c = col(tag);
    wire(x0, y, m - r, y, tag);
    wire(m + r, y, x1, y, tag);
    cv->disc(ox + m, oy + y, r, on(tag) ? light : paper);
    cv->circle(ox + m, oy + y, r, c);
    static const int8_t sv[13] = {0, 2, 3, 4, 3, 2, 0, -2, -3, -4, -3, -2, 0};
    int px = 0, py = 0;
    for (int i = 0; i <= 12; i++) {
        int X = (m - 6 + i) * 16, Y = (y - sv[i]) * 16;
        if (i) cv->lineSub(ox * 16 + px, oy * 16 + py, ox * 16 + X, oy * 16 + Y, c);
        px = X;
        py = Y;
    }
}

void Sch::sw(int x0, int y, int x1, int tag)
{
    uint16_t c = col(tag);
    int m = (x0 + x1) / 2;
    wire(x0, y, m - 6, y, tag);
    wire(m + 6, y, x1, y, tag);
    line(m - 6, y, m + 5, y - 6, c);
    dot(m - 6, y);
    dot(m + 6, y);
}

void Sch::pot(int x0, int y0, int x1, int y1, int tag)
{
    res(x0, y0, x1, y1, tag);
    Span s = spanOf(x0, y0, x1, y1);
    uint16_t c = col(tag);
    if (s.h) {
        line(s.mid - 7, s.t + 7, s.mid + 6, s.t - 6, c);
        cv->arrowHead(ox + s.mid + 7, oy + s.t - 7, 1, -1, 4, c);
    } else {
        line(s.t - 7, s.mid + 7, s.t + 6, s.mid - 6, c);
        cv->arrowHead(ox + s.t + 7, oy + s.mid - 7, 1, -1, 4, c);
    }
}

// --------------------------------------------------------------------------
// Composants actifs
// --------------------------------------------------------------------------
void Sch::opamp(int x, int y, int w, bool minusTop, int tag)
{
    int h = w + 4;
    uint16_t c = col(tag);
    int X0 = ox + x, Y0 = oy + y;
    cv->fillTri(X0, Y0 - h / 2, X0, Y0 + h / 2, X0 + w, Y0, on(tag) ? light : paper);
    cv->line(X0, Y0 - h / 2, X0, Y0 + h / 2, c);
    cv->line(X0, Y0 - h / 2, X0 + w, Y0, c);
    cv->line(X0, Y0 + h / 2, X0 + w, Y0, c);
    int dy = inDy(w);
    int ym = minusTop ? y - dy : y + dy;
    int yp = minusTop ? y + dy : y - dy;
    rect(x + 3, ym, 6, 1, c);               // -
    rect(x + 3, yp, 6, 1, c);               // +
    rect(x + 6, yp - 3, 1, 7, c);
}

void Sch::bjt(int x, int y, bool npn, int tag)
{
    uint16_t c = col(tag);
    int X = ox + x, Y = oy + y;
    cv->disc(X + 12, Y, 11, on(tag) ? light : paper);
    cv->circle(X + 12, Y, 11, c);
    cv->fillRect(X, Y, 9, 1, c);                 // base
    cv->fillRect(X + 8, Y - 7, 2, 15, c);        // barre
    cv->line(X + 10, Y - 4, X + 17, Y - 11, c);  // haut
    cv->line(X + 17, Y - 11, X + 17, Y - 20, c);
    cv->line(X + 10, Y + 4, X + 17, Y + 11, c);  // bas
    cv->line(X + 17, Y + 11, X + 17, Y + 20, c);
    if (npn) cv->arrowHead(X + 17, Y + 11, 1, 1, 5, c);       // emetteur en bas, fleche sortante
    else cv->arrowHead(X + 11, Y - 5, -1, 1, 5, c);           // emetteur en haut, fleche entrante
}

void Sch::mosfet(int x, int y, bool nch, int tag)
{
    uint16_t c = col(tag);
    int X = ox + x, Y = oy + y;
    if (on(tag)) cv->fillRect(X + 5, Y - 11, 15, 23, light);
    cv->fillRect(X, Y, 7, 1, c);                 // grille
    cv->fillRect(X + 7, Y - 9, 1, 19, c);
    cv->fillRect(X + 10, Y - 11, 2, 6, c);       // canal (pointille : enrichissement)
    cv->fillRect(X + 10, Y - 2, 2, 5, c);
    cv->fillRect(X + 10, Y + 6, 2, 6, c);
    cv->fillRect(X + 12, Y - 8, 6, 1, c);        // drain / source
    cv->fillRect(X + 12, Y + 8, 6, 1, c);
    cv->fillRect(X + 18, Y - 20, 1, 13, c);
    cv->fillRect(X + 18, Y + 8, 1, 13, c);
    cv->fillRect(X + 12, Y, 7, 1, c);            // substrat
    if (nch) {
        cv->fillRect(X + 18, Y, 1, 9, c);        // substrat relie a la source (en bas)
        cv->arrowHead(X + 13, Y, -1, 0, 4, c);
    } else {
        cv->fillRect(X + 18, Y - 8, 1, 9, c);    // source en haut
        cv->arrowHead(X + 18, Y, 1, 0, 4, c);
    }
}

void Sch::jfet(int x, int y, bool nch, int tag)
{
    uint16_t c = col(tag);
    int X = ox + x, Y = oy + y;
    cv->disc(X + 14, Y, 11, on(tag) ? light : paper);
    cv->circle(X + 14, Y, 11, c);
    cv->fillRect(X + 11, Y - 9, 2, 19, c);       // canal
    cv->fillRect(X, Y + 4, 11, 1, c);            // grille (cote source)
    cv->fillRect(X + 13, Y - 6, 7, 1, c);
    cv->fillRect(X + 13, Y + 6, 7, 1, c);
    cv->fillRect(X + 19, Y - 20, 1, 15, c);
    cv->fillRect(X + 19, Y + 6, 1, 15, c);
    if (nch) cv->arrowHead(X + 10, Y + 4, 1, 0, 4, c);
    else cv->arrowHead(X + 4, Y + 4, -1, 0, 4, c);
}

void Sch::chip(int x, int y, int w, int h, const char *title, int tag)
{
    uint16_t c = col(tag);
    rect(x, y, w, h, on(tag) ? light : paper);
    frame(x, y, w, h, c);
    if (title) tex::draw(*cv, title, ox + x + w / 2, oy + y + h / 2 + 5, c, tex::CENTER, 1);
}

// --------------------------------------------------------------------------
// Fleches
// --------------------------------------------------------------------------
void Sch::varrow(int x, int y0, int y1, const char *s, int side, int tag)
{
    uint16_t c = on(tag) ? accent : volt;
    int d = sgn(y1 - y0);
    line(x, y0, x, y1 - 3 * d, c);
    cv->arrowHead(ox + x, oy + y1, 0, d, 5, c);
    if (s) {
        int w = tex::width(s);
        int lx = side < 0 ? x - 4 - w : x + 5;
        tex::draw(*cv, s, ox + lx, oy + (y0 + y1) / 2 + 5, c, tex::LEFT, 1);
    }
}

void Sch::harrow(int x0, int x1, int y, const char *s, int side, int tag)
{
    uint16_t c = on(tag) ? accent : volt;
    int d = sgn(x1 - x0);
    line(x0, y, x1 - 3 * d, y, c);
    cv->arrowHead(ox + x1, oy + y, d, 0, 5, c);
    if (s) {
        int by = side < 0 ? y - 4 : y + 14;
        tex::draw(*cv, s, ox + (x0 + x1) / 2, oy + by, c, tex::CENTER, 1);
    }
}

void Sch::iarrow(int x, int y, int dx, int dy, const char *s, int lx, int ly, int tag)
{
    uint16_t c = on(tag) ? accent : volt;
    cv->arrowHead(ox + x, oy + y, dx, dy, 5, c);
    if (s) tex::draw(*cv, s, ox + lx, oy + ly, c, tex::LEFT, 2);
}
