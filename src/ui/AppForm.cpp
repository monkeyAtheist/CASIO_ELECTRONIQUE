// Vue formules : pages de cartes (moteur de Formulaire) et carte agrandie.
#include "App.hpp"
#include "../doc/Content.hpp"
#include "../doc/Index.hpp"
#include "../font/Font.hpp"
#include "../gfx/Color.hpp"
#include "../math/Math.hpp"

int itoa10(int v, char *out);
void drawScrollbar(Canvas &cv, int x, int y, int h, int total, int offset);

static inline int imax(int a, int b) { return a > b ? a : b; }
static inline int imin(int a, int b) { return a < b ? a : b; }

enum { PAGE_OPS = 3200, ZOOM_OPS = 1400, STEP = 56 };
static DrawOp s_pageOps[PAGE_OPS];
static DrawOp s_zoomOps[ZOOM_OPS];
static DisplayList s_pageDL, s_zoomDL;
static bool s_dlInit = false;

static void initDL()
{
    if (s_dlInit) return;
    s_pageDL.init(s_pageOps, PAGE_OPS);
    s_zoomDL.init(s_zoomOps, ZOOM_OPS);
    s_dlInit = true;
}

void App::openFormulas(int chapter, int page, ViewId from)
{
    initDL();
    if (chapter < 0 || chapter >= g_chapterCount || g_index.chapterPageCount[chapter] <= 0) return;
    fchapter = chapter;
    formFrom = from;
    if (page < 0 || g_index.pages[page].chapter != chapter) page = g_index.chapterFirstPage[chapter];
    openFormPage(page, -1);
}

void App::openFormPage(int p, int card)
{
    if (p < 0 || p >= g_index.pageCount) return;
    fpage = p;
    math::g_errors = 0;
    layoutPage(s_pageDL, p, flayout);
    fsel = 0;
    fscroll = 0;
    if (card >= 0) {
        for (int i = 0; i < flayout.cardCount; i++)
            if (flayout.cards[i].card == card) { fsel = i; break; }
        formReveal(fsel, true);
    }
    view = V_FORM;
}

void App::formClamp()
{
    int maxS = imax(0, flayout.height - CONTENT_H);
    if (fscroll > maxS) fscroll = maxS;
    if (fscroll < 0) fscroll = 0;
}

void App::formReveal(int i, bool down)
{
    if (i < 0 || i >= flayout.cardCount) return;
    const CardBox &c = flayout.cards[i];
    int h = c.bottom - c.top;
    if (h + 10 <= CONTENT_H) {
        if (c.top - 5 < fscroll) fscroll = c.top - 5;
        else if (c.bottom + 5 > fscroll + CONTENT_H) fscroll = c.bottom + 5 - CONTENT_H;
    } else {
        fscroll = down ? c.top - 5 : c.bottom + 5 - CONTENT_H;
    }
    formClamp();
}

void App::formMove(int dir)
{
    if (flayout.cardCount == 0) return;
    const CardBox &c = flayout.cards[fsel];
    if (dir > 0) {
        if (c.bottom + 5 > fscroll + CONTENT_H) {
            fscroll = imin(fscroll + STEP, c.bottom + 5 - CONTENT_H);
            formClamp();
            return;
        }
        if (fsel + 1 < flayout.cardCount) { fsel++; formReveal(fsel, true); }
    } else {
        if (c.top - 5 < fscroll) {
            fscroll = imax(fscroll - STEP, c.top - 5);
            formClamp();
            return;
        }
        if (fsel > 0) { fsel--; formReveal(fsel, false); }
    }
}

// Calculateur associe a une page de formules (-1 si aucun).
static int calcOfPage(int page)
{
    for (int i = 0; i < calcCount(); i++)
        if (calcPage(i) == page) return i;
    return -1;
}

void App::drawForm(Canvas &cv)
{
    const Chapter &ch = g_chapters[fchapter];
    const PageRef &pg = g_index.pages[fpage];
    cv.fillRect(0, CONTENT_TOP, SCREEN_W, CONTENT_H, col::pageBg);
    cv.setClip(0, CONTENT_TOP, SCREEN_W, CONTENT_BOTTOM);
    s_pageDL.draw(cv, 0, CONTENT_TOP - fscroll);
    if (fsel >= 0 && fsel < flayout.cardCount) {
        const CardBox &c = flayout.cards[fsel];
        int y = CONTENT_TOP + c.top - fscroll;
        int h = c.bottom - c.top;
        cv.roundRect(PAGE_CARD_X, y, PAGE_CARD_W, h, 3, 0, false, ch.color, true);
        cv.frame(PAGE_CARD_X + 1, y + 1, PAGE_CARD_W - 2, h - 2, ch.color);
    }
    drawScrollbar(cv, SCREEN_W - 5, CONTENT_TOP + 2, CONTENT_H - 4, flayout.height, fscroll);
    cv.resetClip();
    char right[16];
    int first = g_index.chapterFirstPage[fchapter];
    int k = itoa10(fpage - first + 1, right);
    right[k++] = '/';
    itoa10(g_index.chapterPageCount[fchapter], right + k);
    char title[96];
    txt::copy(title, sizeof(title), pg.title, pg.titleLen);
    bool help = fchapter == g_chapterCount - 1;
    drawHeader(cv, ch.color, help ? "Aide" : "Formules", title, right);
    bool hasCalc = !help && calcOfPage(fpage) >= 0;
    const char *fk[6] = {hasCalc ? "Calculer" : "", "\xE2\x97\x80 Page", "Page \xE2\x96\xB6", "", "Zoom", "Retour"};
    drawFKeys(cv, fk);
}

void App::drawZoom(Canvas &cv)
{
    const CardRef &c = g_index.cards[zoomCard];
    const Chapter &ch = g_chapters[g_index.chapterOfCard(zoomCard)];
    cv.fillRect(0, CONTENT_TOP, SCREEN_W, CONTENT_H, col::white);
    cv.setClip(0, CONTENT_TOP, SCREEN_W, CONTENT_BOTTOM);
    s_zoomDL.draw(cv, -zx, CONTENT_TOP - zy);
    drawScrollbar(cv, SCREEN_W - 5, CONTENT_TOP + 2, CONTENT_H - 4, zlayout.height, zy);
    if (zlayout.width > SCREEN_W + 2) {
        int total = zlayout.width, h = SCREEN_W - 8;
        cv.fillRect(4, CONTENT_BOTTOM - 4, h, 3, rgb(220, 224, 232));
        int tw = imax(16, h * h / total);
        int tx = 4 + (h - tw) * zx / (total - SCREEN_W);
        cv.fillRect(tx, CONTENT_BOTTOM - 4, tw, 3, rgb(120, 128, 146));
    }
    cv.resetClip();
    char title[96];
    txt::copy(title, sizeof(title), c.title, c.titleLen);
    drawHeader(cv, ch.color, title, nullptr, "Zoom");
    static const char *const fk[6] = {"\xE2\x97\x80 Carte", "Carte \xE2\x96\xB6", "", "", "", "Retour"};
    drawFKeys(cv, fk);
}

static void openZoomCard(App &a, int card, DisplayList &dl, PageLayout &L)
{
    a.zoomCard = card;
    layoutCardZoom(dl, card, L);
    a.zx = 0;
    a.zy = 0;
    a.view = V_ZOOM;
}

bool App::keyForm(int k)
{
    int first = g_index.chapterFirstPage[fchapter];
    int last = first + g_index.chapterPageCount[fchapter] - 1;
    switch (k) {
    case AK_DOWN:
        if (shift) { fscroll += CONTENT_H - 24; formClamp(); }
        else formMove(1);
        break;
    case AK_UP:
        if (shift) { fscroll -= CONTENT_H - 24; formClamp(); }
        else formMove(-1);
        break;
    case AK_LEFT:
    case AK_F2:
        if (fpage > first) openFormPage(fpage - 1, -1);
        break;
    case AK_RIGHT:
    case AK_F3:
        if (fpage < last) openFormPage(fpage + 1, -1);
        break;
    case AK_EXE:
    case AK_F5:
        if (fsel >= 0 && fsel < flayout.cardCount) openZoomCard(*this, flayout.cards[fsel].card, s_zoomDL, zlayout);
        break;
    case AK_F1: {
        int c = calcOfPage(fpage);
        if (c >= 0 && fchapter != g_chapterCount - 1) openCalc(c);
        break;
    }
    case AK_F6:
    case AK_EXIT:
        if (formFrom == V_CALC && calc >= 0) view = V_CALC;
        else if (formFrom == V_LIST) view = V_LIST;
        else view = V_HOME;
        break;
    default: break;
    }
    return true;
}

bool App::keyZoom(int k)
{
    int maxY = imax(0, zlayout.height - CONTENT_H);
    int maxX = zlayout.width > SCREEN_W + 2 ? zlayout.width - SCREEN_W : 0;
    bool wide = maxX > 0;
    switch (k) {
    case AK_DOWN: zy = imin(maxY, zy + (shift ? CONTENT_H - 24 : 32)); break;
    case AK_UP: zy = imax(0, zy - (shift ? CONTENT_H - 24 : 32)); break;
    case AK_RIGHT:
        if (wide) { zx = imin(maxX, zx + 48); break; }
        /* fallthrough */
    case AK_F2:
        if (zoomCard + 1 < g_index.cardCount &&
            g_index.cards[zoomCard + 1].page == g_index.cards[zoomCard].page)
            openZoomCard(*this, zoomCard + 1, s_zoomDL, zlayout);
        break;
    case AK_LEFT:
        if (wide) { zx = imax(0, zx - 48); break; }
        /* fallthrough */
    case AK_F1:
        if (zoomCard > 0 && g_index.cards[zoomCard - 1].page == g_index.cards[zoomCard].page)
            openZoomCard(*this, zoomCard - 1, s_zoomDL, zlayout);
        break;
    case AK_EXE:
    case AK_EXIT:
    case AK_F6:
        for (int i = 0; i < flayout.cardCount; i++)
            if (flayout.cards[i].card == zoomCard) { fsel = i; formReveal(i, true); }
        view = V_FORM;
        break;
    default: break;
    }
    return true;
}
