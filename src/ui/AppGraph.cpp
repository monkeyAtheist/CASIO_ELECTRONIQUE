// Vue graphe : diagramme de Bode (gain et phase) avec curseur, ou graphe
// propre au calculateur (cycle d'hysteresis, charge d'un condensateur...).
#include "App.hpp"
#include "../font/Font.hpp"
#include "../gfx/Color.hpp"
#include "../gfx/Tex.hpp"

static inline int imax(int a, int b) { return a > b ? a : b; }
static inline int imin(int a, int b) { return a < b ? a : b; }

enum {
    PX0 = 38, PX1 = PX0 + GRAPH_N - 1,
    RD_Y = CONTENT_TOP,                       // ligne de lecture
    GY0 = CONTENT_TOP + 25, GY1 = GY0 + 68,   // gain
    PY0 = GY1 + 12, PY1 = PY0 + 58,           // phase
    FLAB_Y = PY1 + 15,                        // etiquettes de frequence
};

static const uint16_t GRID = rgb(226, 230, 238);
static const uint16_t GRID_DEC = rgb(190, 196, 208);
static const uint16_t CURSOR = rgb(200, 40, 50);

static double colToL(const App &a, int i)
{
    return a.gLmin + (a.gLmax - a.gLmin) * i / (GRAPH_N - 1);
}

static int lToCol(const App &a, double l)
{
    return fm::iround((l - a.gLmin) / (a.gLmax - a.gLmin) * (GRAPH_N - 1));
}

void App::autoRange()
{
    const CalcDef &d = calcDef(calc);
    CalcState &s = g_states[calc];
    double fc = d.fcenter ? d.fcenter(s) : 0;
    if (!(fc > 0) && d.fslot1) fc = s.v[d.fslot1 - 1];
    if (!(fc > 0) || !fm::isfinite(fc)) fc = 1000;
    int span = d.decades ? d.decades : 5;
    double lc = fm::floor(fm::log10(fc) + (span % 2 ? 0 : 0.5));
    if (lc < -2) lc = -2;
    if (lc > 8) lc = 8;
    gLmin = lc - (span - 1) / 2;
    gLmax = gLmin + span;
    if (gLmin < -3) { gLmax += -3 - gLmin; gLmin = -3; }
    if (gLmax > 10) { gLmin -= gLmax - 10; gLmax = 10; }
    gCurL = fm::log10(fc);
    if (d.fslot1 && s.v[d.fslot1 - 1] > 0) {
        double lf = fm::log10(s.v[d.fslot1 - 1]);
        if (lf >= gLmin && lf <= gLmax) gCurL = lf;
    }
}

void App::computeBode()
{
    const CalcDef &d = calcDef(calc);
    CalcState &s = g_states[calc];
    double prev = 0;
    bool havePrev = false;
    double mx = -1e300, mn = 1e300, pmx = -1e300, pmn = 1e300;
    for (int i = 0; i < GRAPH_N; i++) {
        double f = fm::pow10(colToL(*this, i));
        Cx H;
        bool ok = d.transfer && d.transfer(s, f, H) && !cbad(H);
        double m = ok ? cabs(H) : 0;
        if (!(m > 0) || !fm::isfinite(m)) ok = false;
        gOk[i] = ok;
        if (!ok) { havePrev = false; continue; }
        double db = 20 * fm::log10(m);
        double ph = fm::deg(carg(H));
        if (havePrev) {
            while (ph - prev > 180) ph -= 360;
            while (ph - prev < -180) ph += 360;
        }
        prev = ph;
        havePrev = true;
        gDb[i] = (float)db;
        gPh[i] = (float)ph;
        if (db > mx) mx = db;
        if (db < mn) mn = db;
        if (ph > pmx) pmx = ph;
        if (ph < pmn) pmn = ph;
    }
    if (mx < mn) { mx = 0; mn = -40; pmx = 0; pmn = -90; }
    double top = 0, bot = 0;
    for (int pass = 0; pass < 2; pass++) {
        double st = pass ? 20 : 10;
        top = fm::ceil(mx / st - 1e-9) * st;
        if (top - mx < 2) top += st;   // marge au-dessus de la courbe
        bot = fm::floor(mn / st + 1e-9) * st;
        if (top - bot > 100) bot = top - 100;
        if (top - bot < 40) bot = top - 40;
        if (top - bot <= 40) break;
    }
    gTop = (float)top;
    gBot = (float)bot;
    double pt = fm::ceil(pmx / 45 - 1e-9) * 45, pb = fm::floor(pmn / 45 + 1e-9) * 45;
    if (pt - pb < 90) {
        double mid = (pt + pb) / 2;
        pt = fm::ceil((mid + 45) / 45 - 1e-9) * 45;
        pb = pt - 90;
        if (pb > pmn) { pb -= 45; }
    }
    pTop = (float)pt;
    pBot = (float)pb;
    gValid = true;
}

void App::openGraph()
{
    const CalcDef &d = calcDef(calc);
    if (d.graph == G_NONE) return;
    if (d.graph == G_BODE) {
        autoRange();
        computeBode();
    }
    view = V_GRAPH;
}

// --------------------------------------------------------------------------
// Dessin
// --------------------------------------------------------------------------
static int yOf(double v, double top, double bot, int y0, int y1)
{
    return y0 + fm::iround((top - v) / (top - bot) * (y1 - y0));
}

// Etiquette de frequence d'une decade : 1, 10, 100, 1k...
static void decadeLabel(char *out, int e)
{
    static const char *const p[] = {"m", "", "k", "M", "G", "T"};
    int g = e >= 0 ? e / 3 : -((-e + 2) / 3);
    int r = e - 3 * g;
    int k = 0;
    if (r == 0) out[k++] = '1';
    else if (r == 1) { out[k++] = '1'; out[k++] = '0'; }
    else { out[k++] = '1'; out[k++] = '0'; out[k++] = '0'; }
    const char *s = (g + 1 >= 0 && g + 1 < 6) ? p[g + 1] : "?";
    while (*s) out[k++] = *s++;
    out[k] = 0;
}

static void curve(Canvas &cv, const App &a, const float *v, double top, double bot, int y0, int y1,
                  uint16_t c)
{
    cv.setClip(PX0, y0, PX1 + 1, y1 + 1);
    bool have = false;
    int px = 0, py = 0;
    for (int i = 0; i < GRAPH_N; i++) {
        if (!a.gOk[i]) { have = false; continue; }
        int x = (PX0 + i) * 16;
        double yy = y0 + (top - v[i]) / (top - bot) * (y1 - y0);
        if (yy < y0 - 200) yy = y0 - 200;
        if (yy > y1 + 200) yy = y1 + 200;
        int y = (int)(yy * 16);
        if (have) {
            cv.lineSub(px, py, x, y, c);
            cv.lineSub(px, py + 8, x, y + 8, c);   // trait de 1,5 px
        }
        px = x;
        py = y;
        have = true;
    }
    cv.resetClip();
}

void App::drawBode(Canvas &cv, uint16_t accent)
{
    if (!gValid) computeBode();
    cv.fillRect(0, CONTENT_TOP, SCREEN_W, CONTENT_H, col::white);
    // grilles verticales
    int d0 = (int)fm::floor(gLmin + 1e-9), d1 = (int)fm::ceil(gLmax - 1e-9);
    char lab[16];
    for (int d = d0; d <= d1; d++) {
        for (int m = 1; m <= 9; m++) {
            double l = d + fm::log10((double)m);
            if (l < gLmin - 1e-9 || l > gLmax + 1e-9) continue;
            int x = PX0 + lToCol(*this, l);
            uint16_t c = m == 1 ? GRID_DEC : GRID;
            cv.fillRect(x, GY0, 1, GY1 - GY0 + 1, c);
            cv.fillRect(x, PY0, 1, PY1 - PY0 + 1, c);
            if (m == 1) {
                decadeLabel(lab, d);
                int w = font::width(SANS_12, lab);
                font::drawText(cv, SANS_12, imin(imax(x - w / 2, PX0 - 6), SCREEN_W - w - 1), FLAB_Y, lab,
                               col::inkSoft);
            }
        }
    }
    font::drawText(cv, SANS_12, 2, FLAB_Y, "Hz", col::note);
    // grilles horizontales et graduations
    double gstep = (gTop - gBot) <= 40 ? 10 : 20;
    for (double v = gTop; v >= gBot - 1e-6; v -= gstep) {
        int y = yOf(v, gTop, gBot, GY0, GY1);
        cv.fillRect(PX0, y, GRAPH_N, 1, v == 0 ? GRID_DEC : GRID);
        fmtNumber(lab, sizeof lab, v, 3);
        int w = font::width(SANS_12, lab);
        font::drawText(cv, SANS_12, PX0 - 3 - w, y + 4, lab, col::inkSoft);
    }
    double pstep = (pTop - pBot) <= 180 ? 45 : 90;
    for (double v = pTop; v >= pBot - 1e-6; v -= pstep) {
        int y = yOf(v, pTop, pBot, PY0, PY1);
        cv.fillRect(PX0, y, GRAPH_N, 1, v == 0 ? GRID_DEC : GRID);
        int k = fmtNumber(lab, sizeof lab, v, 3);
        scopy(lab + k, (int)sizeof(lab) - k, "\xC2\xB0");
        int w = font::width(SANS_12, lab);
        font::drawText(cv, SANS_12, PX0 - 3 - w, y + 4, lab, col::inkSoft);
    }
    cv.frame(PX0, GY0, GRAPH_N, GY1 - GY0 + 1, GRID_DEC);
    cv.frame(PX0, PY0, GRAPH_N, PY1 - PY0 + 1, GRID_DEC);
    tex::draw(cv, "#dB", PX0 + 3, GY0 + 12, col::note, tex::LEFT, 2);
    tex::draw(cv, "\\varphi", PX0 + 4, PY0 + 12, col::note, tex::LEFT, 2);
    // courbes
    curve(cv, *this, gDb, gTop, gBot, GY0, GY1, accent);
    curve(cv, *this, gPh, pTop, pBot, PY0, PY1, mix16(accent, col::ink, 6));
    // curseur
    int ci = imin(GRAPH_N - 1, imax(0, lToCol(*this, gCurL)));
    int cx = PX0 + ci;
    for (int y = GY0; y <= PY1; y += 2) cv.fillRect(cx, y, 1, 1, CURSOR);
    double f = fm::pow10(colToL(*this, ci));
    char line[128];
    int k = scopy(line, sizeof line, "f = ");
    k += fmtValue(line + k, (int)sizeof(line) - k, f, U_HZ);
    if (gOk[ci]) {
        int yg = yOf(gDb[ci], gTop, gBot, GY0, GY1), yp = yOf(gPh[ci], pTop, pBot, PY0, PY1);
        if (yg >= GY0 && yg <= GY1) cv.disc(cx, yg, 3, CURSOR);
        if (yp >= PY0 && yp <= PY1) cv.disc(cx, yp, 3, CURSOR);
        k += scopy(line + k, (int)sizeof(line) - k, "    |H| = ");
        k += fmtValue(line + k, (int)sizeof(line) - k, gDb[ci], U_DB);
        k += scopy(line + k, (int)sizeof(line) - k, "  (");
        k += fmtValue(line + k, (int)sizeof(line) - k, fm::pow10(gDb[ci] / 20.0), U_NONE, 3);
        k += scopy(line + k, (int)sizeof(line) - k, ")    \xCF\x86 = ");
        fmtValue(line + k, (int)sizeof(line) - k, gPh[ci], U_DEG);
    }
    cv.fillRect(0, CONTENT_TOP, SCREEN_W, 17, rgb(242, 244, 248));
    cv.fillRect(0, CONTENT_TOP + 17, SCREEN_W, 1, col::line);
    font::drawTextFit(cv, SANS_12, 6, RD_Y + 13, line, col::ink, SCREEN_W - 10);
}

void App::drawGraph(Canvas &cv)
{
    const CalcDef &d = calcDef(calc);
    const Section &sec = g_sections[d.section];
    if (d.graph == G_BODE) {
        drawBode(cv, sec.color);
        drawHeader(cv, sec.color, d.graphLabel ? d.graphLabel : "Bode", d.title, nullptr);
        const CalcDef &dd = d;
        const char *fk[6] = {"\xE2\x97\x80 D\xC3\xA9" "c.", "D\xC3\xA9" "c. \xE2\x96\xB6", "Zoom +", "Zoom \xE2\x88\x92",
                             dd.fslot1 ? "Prendre f" : "", "Retour"};
        drawFKeys(cv, fk);
        return;
    }
    cv.fillRect(0, CONTENT_TOP, SCREEN_W, CONTENT_H, col::white);
    GraphArea a{0, CONTENT_TOP, SCREEN_W, CONTENT_H};
    cv.setClip(0, CONTENT_TOP, SCREEN_W, CONTENT_BOTTOM);
    if (d.drawGraph) d.drawGraph(cv, g_states[calc], a, sec.color);
    cv.resetClip();
    drawHeader(cv, sec.color, d.graphLabel ? d.graphLabel : "Graphe", d.title, nullptr);
    static const char *const fk[6] = {"", "", "", "", "", "Retour"};
    drawFKeys(cv, fk);
}

bool App::keyGraph(int k)
{
    const CalcDef &d = calcDef(calc);
    if (d.graph != G_BODE) {
        if (k == AK_EXIT || k == AK_F6 || k == AK_EXE) view = V_CALC;
        return true;
    }
    double span = gLmax - gLmin;
    double colStep = span / (GRAPH_N - 1);
    switch (k) {
    case AK_LEFT: gCurL -= colStep * (shift ? 24 : 2); break;
    case AK_RIGHT: gCurL += colStep * (shift ? 24 : 2); break;
    case AK_F1: gLmin -= 1; gLmax -= 1; gCurL -= 1; gValid = false; break;
    case AK_F2: gLmin += 1; gLmax += 1; gCurL += 1; gValid = false; break;
    case AK_UP:
    case AK_F3:
        if (span > 2.5) {
            // retire une decade du cote le plus eloigne du curseur
            if (gCurL - gLmin > gLmax - gCurL) gLmin += 1;
            else gLmax -= 1;
            gValid = false;
        }
        break;
    case AK_DOWN:
    case AK_F4:
        if (span < 9) {
            if (gCurL - gLmin < gLmax - gCurL) gLmin -= 1;
            else gLmax += 1;
            gValid = false;
        }
        break;
    case AK_F5:
        if (d.fslot1) {
            int ci = imin(GRAPH_N - 1, imax(0, lToCol(*this, gCurL)));
            double f = fm::pow10(colToL(*this, ci));
            // 4 chiffres significatifs
            double e = fm::floor(fm::log10(f)) - 3;
            f = fm::floor(f / fm::pow10(e) + 0.5) * fm::pow10(e);
            g_states[calc].v[d.fslot1 - 1] = f;
            recompute();
            view = V_CALC;
            return true;
        }
        break;
    case AK_EXE:
    case AK_EXIT:
    case AK_F6:
        view = V_CALC;
        return true;
    default: break;
    }
    if (gLmin < -3) { gLmax += -3 - gLmin; gLmin = -3; gValid = false; }
    if (gLmax > 10) { gLmin -= gLmax - 10; gLmax = 10; gValid = false; }
    if (gCurL < gLmin) gCurL = gLmin;
    if (gCurL > gLmax) gCurL = gLmax;
    return true;
}
