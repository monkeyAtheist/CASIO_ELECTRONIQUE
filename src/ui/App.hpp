// Application Electronique : accueil (sections), liste des calculateurs,
// calculateur (schema + champs), formules, graphe (Bode...), recherche.
#pragma once
#include <stdint.h>
#include "../gfx/Canvas.hpp"
#include "../gfx/DisplayList.hpp"
#include "../doc/PageLayout.hpp"
#include "../calc/Calc.hpp"

enum ViewId : uint8_t { V_HOME, V_LIST, V_CALC, V_FORM, V_ZOOM, V_GRAPH, V_SEARCH };

// Touches de l'application (independantes de gint).
enum AppKey {
    AK_NONE, AK_UP, AK_DOWN, AK_LEFT, AK_RIGHT, AK_EXE, AK_EXIT, AK_SHIFT, AK_ALPHA,
    AK_F1, AK_F2, AK_F3, AK_F4, AK_F5, AK_F6, AK_DEL, AK_AC, AK_OPTN, AK_VARS,
    AK_DIGIT,  // chiffre (parametre ch)
    AK_DOT, AK_EXP, AK_NEG, AK_SUB,
    AK_ADD, AK_MUL, AK_DIV, AK_LPAR, AK_RPAR, AK_XOT,
    AK_CHAR,   // autre touche a lettre rouge (recherche)
};

enum {
    CONTENT_TOP = 20, CONTENT_BOTTOM = 206, CONTENT_H = CONTENT_BOTTOM - CONTENT_TOP,
    // vue calculateur
    SCH_X = 0, SCH_W = 179, FIELDS_X = 180, FIELDS_W = SCREEN_W - FIELDS_X,
    STATUS_Y = 191, PANEL_BOTTOM = STATUS_Y, ROW_H = 18, HEAD_H = 14,
};

enum { MAX_SECTION_CALCS = 16, GRAPH_N = 350 };

struct App {
    ViewId view;
    bool shift;

    // accueil
    int homeSel, homeTop;

    // liste des calculateurs d'une section
    int section;
    int listSel, listScroll;
    int listCalcs[MAX_SECTION_CALCS];
    int listCount;

    // calculateur
    int calc;
    RowList rows;
    int rowSel, rowScroll;
    bool editing;
    char ebuf[24];
    int elen;
    char tbuf[TXT_LEN];     // saisie d'un texte (RK_TEXT)
    int tlen;
    const char *status;     // message temporaire (barre d'aide)
    bool statusBad;
    bool resetArmed;        // F5 deux fois : valeurs par defaut

    // formules (pages du chapitre de la section)
    ViewId formFrom;
    int fchapter, fpage, fsel, fscroll;
    PageLayout flayout;
    int zoomCard, zx, zy;
    PageLayout zlayout;

    // graphe
    double gLmin, gLmax;    // log10 des frequences extremes
    double gCurL;           // log10 de la frequence du curseur
    bool gValid;
    float gTop, gBot, pTop, pBot;   // echelles (dB, degres)
    float gDb[GRAPH_N], gPh[GRAPH_N];
    uint8_t gOk[GRAPH_N];

    // recherche
    ViewId searchFrom;
    char query[28];
    int qlen;
    int results[MAX_CALCS];
    int nres, rsel, rscroll;

    void init();
    void draw(Canvas &cv);
    // Renvoie false pour quitter l'application.
    bool key(int k, char ch, char letter);

    // navigation
    void openSection(int s, int calcSel);
    void openCalc(int c);
    void openFormulas(int chapter, int page, ViewId from);
    void openGraph();
    void openSearch();

    // outils de dessin communs
    void drawHeader(Canvas &cv, uint16_t bg, const char *title, const char *sub, const char *right);
    void drawFKeys(Canvas &cv, const char *const labels[6]);

private:
    void drawHome(Canvas &cv);
    void drawList(Canvas &cv);
    void drawCalc(Canvas &cv);
    void drawFields(Canvas &cv, uint16_t accent);
    void drawForm(Canvas &cv);
    void drawZoom(Canvas &cv);
    void drawGraph(Canvas &cv);
    void drawBode(Canvas &cv, uint16_t accent);
    void drawSearch(Canvas &cv);

    bool keyHome(int k);
    void homeReveal();
    bool keyList(int k);
    bool keyCalc(int k, char ch);
    bool keyEdit(int k, char ch);
    bool keyEditText(int k, char ch);
    bool keyForm(int k);
    bool keyZoom(int k);
    bool keyGraph(int k);
    bool keySearch(int k, char letter);

    // calculateur
    void rebuildRows(int keepKind, int keepSlot);
    void recompute();
    void moveRow(int dir);
    void revealRow();
    int rowY(int i) const;
    int rowHeight(int i) const;
    void stepValue(int dir);
    void startEdit(char first);
    bool commitEdit(int prefix);
    void listAdd();
    void listRemove();
    const char *rowHelp(const Row &r) const;

    // formules
    void openFormPage(int page, int card);
    void formMove(int dir);
    void formReveal(int i, bool down);
    void formClamp();

    // graphe
    void computeBode();
    void autoRange();

    void runSearch();
};

extern App g_app;

