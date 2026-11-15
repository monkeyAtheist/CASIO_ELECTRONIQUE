// Application : elements communs, accueil, liste des calculateurs, recherche.
#include "App.hpp"
#include "../doc/Content.hpp"
#include "../doc/Index.hpp"
#include "../font/Font.hpp"
#include "../gfx/Color.hpp"
#include "../gfx/Tex.hpp"
#include "../math/Math.hpp"
#include <string.h>

App g_app;

const uint16_t HOME_BG = rgb(30, 38, 52);
// Accueil : grille de 4 colonnes, 3 rangees visibles.
enum { TILE_W = 94, TILE_H = 57, TILE_DX = 97, TILE_DY = 60, HOME_ROWS = 3 };
enum { LIST_RH = 34 };

static inline int imax(int a, int b) { return a > b ? a : b; }
static inline int imin(int a, int b) { return a < b ? a : b; }

int itoa10(int v, char *out)
{
    char tmp[12];
    int n = 0;
    bool neg = v < 0;
    if (neg) v = -v;
    do { tmp[n++] = (char)('0' + v % 10); v /= 10; } while (v && n < 11);
    int k = 0;
    if (neg) out[k++] = '-';
    while (n) out[k++] = tmp[--n];
    out[k] = 0;
    return k;
}

void drawScrollbar(Canvas &cv, int x, int y, int h, int total, int offset)
{
    if (total <= h) return;
    cv.fillRect(x, y, 3, h, rgb(220, 224, 232));
    int th = imax(12, h * h / total);
    int ty = y + (h - th) * offset / (total - h);
    cv.fillRect(x, ty, 3, th, rgb(120, 128, 146));
}

// --------------------------------------------------------------------------
// Initialisation
// --------------------------------------------------------------------------
void App::init()
{
    g_index.build();
    registryInit();
    view = V_HOME;
    shift = false;
    homeSel = 0;
    homeTop = 0;
    section = 0;
    listSel = 0;
    listScroll = 0;
    listCount = 0;
    calc = -1;
    rows.n = 0;
    rowSel = 0;
    rowScroll = 0;
    editing = false;
    elen = 0;
    ebuf[0] = 0;
    tlen = 0;
    tbuf[0] = 0;
    status = nullptr;
    statusBad = false;
    resetArmed = false;
    formFrom = V_HOME;
    fchapter = 0;
    fpage = -1;
    fsel = 0;
    fscroll = 0;
    zoomCard = -1;
    gValid = false;
    searchFrom = V_HOME;
    qlen = 0;
    query[0] = 0;
    nres = 0;
    rsel = 0;
    rscroll = 0;
}

// --------------------------------------------------------------------------
// Elements communs
// --------------------------------------------------------------------------
void App::drawHeader(Canvas &cv, uint16_t bg, const char *title, const char *sub, const char *right)
{
    cv.fillRect(0, 0, SCREEN_W, CONTENT_TOP, bg);
    cv.fillRect(0, CONTENT_TOP - 1, SCREEN_W, 1, mix16(bg, col::black, 4));
    int rx = SCREEN_W - 4;
    if (shift) {
        int w = font::width(BOLD_12, "SHIFT") + 6;
        rx -= w;
        cv.roundRect(rx, 3, w, 14, 2, col::white, true, 0, false);
        font::drawText(cv, BOLD_12, rx + 3, 14, "SHIFT", bg);
        rx -= 4;
    }
    if (right && right[0]) {
        int w = font::width(SANS_12, right);
        rx -= w;
        font::drawText(cv, SANS_12, rx, 14, right, col::white);
        rx -= 6;
    }
    int avail = rx - 6;
    int tw = font::width(BOLD_14, title);
    if (tw > avail || !sub) {
        font::drawTextFit(cv, BOLD_14, 6, 15, title, col::white, avail);
        return;
    }
    font::drawText(cv, BOLD_14, 6, 15, title, col::white);
    char sb[96];
    int k = scopy(sb, sizeof(sb), " \xE2\x80\xBA ");
    scopy(sb + k, (int)sizeof(sb) - k, sub);
    font::drawTextFit(cv, SANS_14, 6 + tw, 15, sb, mix16(col::white, bg, 3), avail - tw);
}

void App::drawFKeys(Canvas &cv, const char *const labels[6])
{
    int y = CONTENT_BOTTOM;
    cv.fillRect(0, y, SCREEN_W, SCREEN_H - y, col::fbar);
    for (int i = 0; i < 6; i++) {
        if (!labels[i] || !labels[i][0]) continue;
        int x = i * 66 + 1;
        cv.roundRect(x, y + 2, 64, SCREEN_H - y - 3, 2, rgb(74, 84, 104), true, 0, false);
        int w = font::width(SANS_12, labels[i]);
        if (w > 62) font::drawTextFit(cv, SANS_12, x + 2, y + 14, labels[i], col::fbarText, 60);
        else font::drawText(cv, SANS_12, x + (64 - w) / 2, y + 14, labels[i], col::fbarText);
    }
}

void App::draw(Canvas &cv)
{
    switch (view) {
    case V_HOME: drawHome(cv); break;
    case V_LIST: drawList(cv); break;
    case V_CALC: drawCalc(cv); break;
    case V_FORM: drawForm(cv); break;
    case V_ZOOM: drawZoom(cv); break;
    case V_GRAPH: drawGraph(cv); break;
    case V_SEARCH: drawSearch(cv); break;
    }
}

bool App::key(int k, char ch, char letter)
{
    if (k == AK_SHIFT) {
        shift = !shift;
        return true;
    }
    bool r = true;
    switch (view) {
    case V_HOME: r = keyHome(k); break;
    case V_LIST: r = keyList(k); break;
    case V_CALC: r = editing ? keyEdit(k, ch) : keyCalc(k, ch); break;
    case V_FORM: r = keyForm(k); break;
    case V_ZOOM: r = keyZoom(k); break;
    case V_GRAPH: r = keyGraph(k); break;
    case V_SEARCH: r = keySearch(k, letter); break;
    }
    shift = false;
    return r;
}

// --------------------------------------------------------------------------
// Accueil
// --------------------------------------------------------------------------
void App::drawHome(Canvas &cv)
{
    cv.fillRect(0, CONTENT_TOP, SCREEN_W, CONTENT_H, col::pageBg);
    drawHeader(cv, HOME_BG, "\xC3\x89lectronique", nullptr, "Graph 90+E");
    int n = g_sectionCount;
    int rowsTot = (n + 3) / 4;
    for (int i = 0; i < n; i++) {
        const Section &s = g_sections[i];
        int c = i % 4, r = i / 4;
        if (r < homeTop || r >= homeTop + HOME_ROWS) continue;
        int tx = 4 + c * TILE_DX, ty = CONTENT_TOP + 3 + (r - homeTop) * TILE_DY;
        bool on = i == homeSel;
        if (on) {
            cv.roundRect(tx, ty, TILE_W, TILE_H, 3, s.color, true, s.color, true);
        } else {
            cv.roundRect(tx, ty, TILE_W, TILE_H, 3, col::white, true, col::line, true);
            cv.fillRect(tx + 3, ty + TILE_H - 3, TILE_W - 6, 2, mix16(col::white, s.color, 9));
        }
        if (s.icon) s.icon(cv, tx + TILE_W / 2, ty + 22, on ? col::white : s.color, on ? s.color : col::white);
        int f = on ? BOLD_12 : SANS_12;
        int w = font::width(f, s.label);
        uint16_t tc = on ? col::white : col::ink;
        if (w > TILE_W - 4) font::drawTextFit(cv, f, tx + 2, ty + 50, s.label, tc, TILE_W - 4);
        else font::drawText(cv, f, tx + (TILE_W - w) / 2, ty + 50, s.label, tc);
    }
    if (rowsTot > HOME_ROWS)
        drawScrollbar(cv, SCREEN_W - 5, CONTENT_TOP + 3, HOME_ROWS * TILE_DY - 3, rowsTot * TILE_DY - 3,
                      homeTop * TILE_DY);
    static const char *const fk[6] = {"Ouvrir", "", "", "Chercher", "Aide", "Quitter"};
    drawFKeys(cv, fk);
}

void App::homeReveal()
{
    int r = homeSel / 4;
    int rowsTot = (g_sectionCount + 3) / 4;
    if (r < homeTop) homeTop = r;
    if (r >= homeTop + HOME_ROWS) homeTop = r - HOME_ROWS + 1;
    if (homeTop > rowsTot - HOME_ROWS) homeTop = rowsTot - HOME_ROWS;
    if (homeTop < 0) homeTop = 0;
}

bool App::keyHome(int k)
{
    int n = g_sectionCount;
    switch (k) {
    case AK_LEFT: homeSel = (homeSel + n - 1) % n; break;
    case AK_RIGHT: homeSel = (homeSel + 1) % n; break;
    case AK_UP: if (homeSel >= 4) homeSel -= 4; break;
    case AK_DOWN:
        if (homeSel + 4 < n) homeSel += 4;
        else if (homeSel / 4 < (n - 1) / 4) homeSel = n - 1;
        break;
    case AK_EXE:
    case AK_F1: openSection(homeSel, 0); return true;
    case AK_F4: openSearch(); return true;
    case AK_F5: openFormulas(g_chapterCount - 1, -1, V_HOME); return true;   // aide
    case AK_F6:
    case AK_EXIT: return k != AK_F6;
    default: break;
    }
    homeReveal();
    return true;
}

// --------------------------------------------------------------------------
// Liste des calculateurs d'une section
// --------------------------------------------------------------------------
void App::openSection(int s, int calcSel)
{
    if (s < 0 || s >= g_sectionCount) return;
    section = s;
    listCount = 0;
    for (int i = 0; i < calcCount() && listCount < MAX_SECTION_CALCS; i++)
        if (calcDef(i).section == s) listCalcs[listCount++] = i;
    listSel = imin(imax(calcSel, 0), imax(0, listCount - 1));
    listScroll = imax(0, listSel * LIST_RH - (CONTENT_H - LIST_RH) / 2);
    int maxS = imax(0, listCount * LIST_RH + 4 - CONTENT_H);
    if (listScroll > maxS) listScroll = maxS;
    view = V_LIST;
}

void App::drawList(Canvas &cv)
{
    const Section &sec = g_sections[section];
    cv.fillRect(0, CONTENT_TOP, SCREEN_W, CONTENT_H, col::pageBg);
    cv.setClip(0, CONTENT_TOP, SCREEN_W, CONTENT_BOTTOM);
    for (int i = 0; i < listCount; i++) {
        int y = CONTENT_TOP + 3 + i * LIST_RH - listScroll;
        if (y + LIST_RH < CONTENT_TOP || y > CONTENT_BOTTOM) continue;
        const CalcDef &d = calcDef(listCalcs[i]);
        bool on = i == listSel;
        cv.roundRect(4, y, SCREEN_W - 14, LIST_RH - 3, 3, on ? mix16(col::white, sec.color, 3) : col::white,
                     true, on ? sec.color : col::line, true);
        if (on) cv.frame(5, y + 1, SCREEN_W - 16, LIST_RH - 5, sec.color);
        // pastille numerotee
        cv.roundRect(9, y + 7, 18, 18, 3, sec.color, true, 0, false);
        char num[4];
        itoa10(i + 1, num);
        int nw = font::width(BOLD_12, num);
        font::drawText(cv, BOLD_12, 18 - nw / 2, y + 20, num, col::white);
        font::drawTextFit(cv, BOLD_14, 34, y + 14, d.title, col::ink, SCREEN_W - 52);
        if (d.sub) {
            Canvas &c2 = cv;
            c2.setClip(0, imax(CONTENT_TOP, y), SCREEN_W - 12, imin(CONTENT_BOTTOM, y + LIST_RH - 4));
            tex::draw(c2, d.sub, 34, y + 27, col::inkSoft, tex::LEFT, 2);
            c2.setClip(0, CONTENT_TOP, SCREEN_W, CONTENT_BOTTOM);
        }
    }
    drawScrollbar(cv, SCREEN_W - 5, CONTENT_TOP + 2, CONTENT_H - 4, listCount * LIST_RH + 4, listScroll);
    cv.resetClip();
    char right[16];
    int k = itoa10(listSel + 1, right);
    right[k++] = '/';
    itoa10(listCount, right + k);
    drawHeader(cv, sec.color, sec.title, nullptr, right);
    static const char *const fk[6] = {"Ouvrir", "", "", "Chercher", "Formules", "Th\xC3\xA8mes"};
    drawFKeys(cv, fk);
}

bool App::keyList(int k)
{
    switch (k) {
    case AK_DOWN: if (listSel + 1 < listCount) listSel++; break;
    case AK_UP: if (listSel > 0) listSel--; break;
    case AK_RIGHT: listSel = imin(listCount - 1, listSel + 5); break;
    case AK_LEFT: listSel = imax(0, listSel - 5); break;
    case AK_EXE:
    case AK_F1:
        if (listCount) openCalc(listCalcs[listSel]);
        return true;
    case AK_F4: openSearch(); return true;
    case AK_F5:
        if (listCount) {
            int p = calcPage(listCalcs[listSel]);
            openFormulas(section, p, V_LIST);
        }
        return true;
    case AK_EXIT:
    case AK_F6:
        view = V_HOME;
        homeSel = section;
        homeReveal();
        return true;
    default: break;
    }
    if (listSel * LIST_RH < listScroll) listScroll = listSel * LIST_RH;
    if ((listSel + 1) * LIST_RH + 6 > listScroll + CONTENT_H)
        listScroll = (listSel + 1) * LIST_RH + 6 - CONTENT_H;
    return true;
}

// --------------------------------------------------------------------------
// Recherche (titres, sous-titres, mots cles des calculateurs)
// --------------------------------------------------------------------------
void App::openSearch()
{
    if (view != V_SEARCH) searchFrom = view;
    rsel = 0;
    rscroll = 0;
    view = V_SEARCH;
}

static int normalize(const char *s, char *out, int cap, int n0)
{
    const char *e = s + strlen(s);
    int n = n0;
    bool lastSpace = n == 0 || out[n - 1] == ' ';
    while (s < e && n < cap - 1) {
        uint32_t cp = font::utf8Next(s, e);
        char c = txt::fold(cp);
        if (!c) {
            if (!lastSpace) { out[n++] = ' '; lastSpace = true; }
            continue;
        }
        out[n++] = c;
        lastSpace = false;
    }
    out[n] = 0;
    return n;
}

static bool contains(const char *hay, const char *needle)
{
    int n = (int)strlen(needle);
    if (!n) return true;
    for (const char *h = hay; *h; h++)
        if (!strncmp(h, needle, n)) return true;
    return false;
}

void App::runSearch()
{
    nres = 0;
    rsel = 0;
    rscroll = 0;
    char q[40];
    normalize(query, q, sizeof(q), 0);
    const char *words[8];
    int nw = 0;
    char *p = q;
    while (*p && nw < 8) {
        while (*p == ' ') *p++ = 0;
        if (!*p) break;
        words[nw++] = p;
        while (*p && *p != ' ') p++;
    }
    if (!nw) return;
    for (int pass = 0; pass < 2; pass++) {
        for (int i = 0; i < calcCount() && nres < MAX_CALCS; i++) {
            const CalcDef &d = calcDef(i);
            char hay[400];
            int n = normalize(d.title, hay, sizeof(hay), 0);
            if (pass == 1) {
                hay[n++] = ' ';
                hay[n] = 0;
                n = normalize(g_sections[d.section].title, hay, sizeof(hay), n);
                if (d.tags) { hay[n++] = ' '; hay[n] = 0; n = normalize(d.tags, hay, sizeof(hay), n); }
            }
            bool okAll = true;
            for (int w = 0; w < nw && okAll; w++) okAll = contains(hay, words[w]);
            if (!okAll) continue;
            bool dup = false;
            for (int j = 0; j < nres; j++) if (results[j] == i) { dup = true; break; }
            if (!dup) results[nres++] = i;
        }
    }
}

void App::drawSearch(Canvas &cv)
{
    cv.fillRect(0, CONTENT_TOP, SCREEN_W, CONTENT_H, col::pageBg);
    cv.roundRect(4, CONTENT_TOP + 4, SCREEN_W - 8, 24, 3, col::white, true, rgb(120, 128, 146), true);
    int x = 10;
    if (qlen) x = font::drawText(cv, BOLD_16, 10, CONTENT_TOP + 22, query, col::ink);
    else font::drawText(cv, SANS_14, 10, CONTENT_TOP + 21, "Tapez un mot (les lettres rouges)", col::note);
    cv.fillRect(x + 1, CONTENT_TOP + 9, 2, 15, col::blue);
    const int RH = 30, top = CONTENT_TOP + 32;
    cv.setClip(0, top, SCREEN_W, CONTENT_BOTTOM);
    if (!qlen) {
        font::drawText(cv, SANS_14, 10, top + 18, "Exemples : schmitt, led, bode, mosfet,", col::inkSoft);
        font::drawText(cv, SANS_14, 10, top + 36, "rlc, 555, zener, couleur, inverseur", col::inkSoft);
        font::drawText(cv, SANS_12, 10, top + 62, "Lettres = touches avec une lettre rouge", col::note);
        font::drawText(cv, SANS_12, 10, top + 78, "(sans appuyer sur ALPHA). [.] = espace.", col::note);
    } else if (!nres) {
        font::drawText(cv, SANS_14, 10, top + 18, "Aucun r\xC3\xA9sultat", col::inkSoft);
    }
    for (int i = 0; i < nres; i++) {
        int y = top + i * RH - rscroll;
        if (y + RH < top || y > CONTENT_BOTTOM) continue;
        const CalcDef &d = calcDef(results[i]);
        const Section &s = g_sections[d.section];
        bool on = i == rsel;
        if (on) cv.roundRect(4, y, SCREEN_W - 12, RH - 2, 2, mix16(col::white, s.color, 3), true, s.color, true);
        font::drawTextFit(cv, on ? BOLD_14 : SANS_14, 10, y + 14, d.title, col::ink, SCREEN_W - 26);
        font::drawTextFit(cv, SANS_12, 18, y + 26, s.title, s.color, SCREEN_W - 34);
    }
    drawScrollbar(cv, SCREEN_W - 5, top, CONTENT_BOTTOM - top, nres * RH, rscroll);
    cv.resetClip();
    char right[24];
    right[0] = 0;
    if (qlen) {
        int k = itoa10(nres, right);
        scopy(right + k, 24 - k, nres > 1 ? " r\xC3\xA9sultats" : " r\xC3\xA9sultat");
    }
    drawHeader(cv, HOME_BG, "Recherche", nullptr, right);
    static const char *const fk[6] = {"Ouvrir", "", "", "", "Effacer", "Retour"};
    drawFKeys(cv, fk);
}

bool App::keySearch(int k, char letter)
{
    const int RH = 30, listH = CONTENT_BOTTOM - (CONTENT_TOP + 32);
    switch (k) {
    case AK_DIGIT:
    case AK_DOT:
    case AK_SUB:
    case AK_ADD: case AK_MUL: case AK_DIV: case AK_LPAR: case AK_RPAR: case AK_XOT:
    case AK_CHAR:
        if (letter && qlen < (int)sizeof(query) - 1 &&
            !(letter == ' ' && (qlen == 0 || query[qlen - 1] == ' '))) {
            query[qlen++] = letter;
            query[qlen] = 0;
            runSearch();
        }
        break;
    case AK_DEL:
        if (qlen) {
            query[--qlen] = 0;
            runSearch();
        }
        break;
    case AK_AC:
    case AK_F5:
        qlen = 0;
        query[0] = 0;
        nres = 0;
        break;
    case AK_DOWN: if (rsel + 1 < nres) rsel++; break;
    case AK_UP: if (rsel > 0) rsel--; break;
    case AK_EXE:
    case AK_F1:
        if (nres) {
            int c = results[rsel];
            int idx, cnt;
            idx = calcIndexInSection(c, cnt);
            openSection(calcDef(c).section, idx);
            openCalc(c);
        }
        return true;
    case AK_EXIT:
    case AK_F6:
        view = searchFrom == V_LIST ? V_LIST : V_HOME;
        return true;
    default: break;
    }
    if (rsel * RH < rscroll) rscroll = rsel * RH;
    if ((rsel + 1) * RH > rscroll + listH) rscroll = (rsel + 1) * RH - listH;
    return true;
}
