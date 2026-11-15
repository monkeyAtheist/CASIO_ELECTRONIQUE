// Verifie tout le contenu et tous les ecrans :
//  - chaque page et chaque carte de formules se met en page (erreurs LaTeX,
//    elements trop larges, glyphes absents, capacites) ;
//  - chaque calculateur a sa page de formules ;
//  - chaque ecran de calculateur (chaque ligne selectionnee, chaque valeur
//    de chaque choix, saisie, graphe) se dessine sans erreur LaTeX ni glyphe
//    absent ; les etiquettes des lignes tiennent dans leur place.
// Option -png DIR : image de chaque page de formules.
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../../src/doc/Index.hpp"
#include "../../src/doc/Content.hpp"
#include "../../src/doc/PageLayout.hpp"
#include "../../src/math/Math.hpp"
#include "../../src/font/Font.hpp"
#include "../../src/ui/App.hpp"
#include "../../src/gfx/Tex.hpp"

static DrawOp s_ops[30000];
static uint16_t s_vram[SCREEN_W * SCREEN_H];

static int s_problems = 0;
static bool s_quietOverflow = false;
static void onOverflow(const char *src, int len, int w, int avail)
{
    if (s_quietOverflow) return;
    if (len > 90) len = 90;
    printf("    trop large %d > %d : %.*s\n", w, avail, len, src);
}

static void savePPM(const char *file, const DisplayList &dl, int H)
{
    static uint16_t img[SCREEN_W * 4000];
    if (H > 4000) H = 4000;
    for (int i = 0; i < SCREEN_W * H; i++) img[i] = rgb(242, 244, 248);
    Canvas cv;
    for (int band = 0; band < H; band += SCREEN_H) {
        cv.fb = img + band * SCREEN_W;
        cv.setClip(0, 0, SCREEN_W, H - band < SCREEN_H ? H - band : SCREEN_H);
        dl.draw(cv, 0, -band);
    }
    FILE *f = fopen(file, "wb");
    if (!f) return;
    fprintf(f, "P6\n%d %d\n255\n", SCREEN_W, H);
    for (int i = 0; i < SCREEN_W * H; i++) {
        uint16_t c = img[i];
        unsigned char px[3] = {(unsigned char)(((c >> 11) & 31) * 255 / 31),
                               (unsigned char)(((c >> 5) & 63) * 255 / 63), (unsigned char)((c & 31) * 255 / 31)};
        fwrite(px, 1, 3, f);
    }
    fclose(f);
}

// Dessine l'ecran courant et signale erreurs et glyphes absents.
static void drawCheck(const char *what, const char *detail)
{
    Canvas cv;
    cv.begin(s_vram);
    int e0 = math::g_errors, m0 = font::g_missing;
    math::g_oom = false;
    g_app.draw(cv);
    if (math::g_errors != e0 || font::g_missing != m0 || math::g_oom) {
        s_problems++;
        printf("ECRAN %s [%s] : erreurs LaTeX=%d (%s) glyphes absents=%d (U+%X)%s\n", what, detail,
               math::g_errors - e0, math::g_errMsg, font::g_missing - m0, font::g_lastMissing,
               math::g_oom ? " MEMOIRE" : "");
    }
}

int main(int argc, char **argv)
{
    g_overflowHook = onOverflow;
    const char *pngDir = nullptr;
    for (int i = 1; i < argc; i++)
        if (!strcmp(argv[i], "-png") && i + 1 < argc) pngDir = argv[++i];
    g_app.init();
    if (g_index.overflow) { printf("INDEX : capacite depassee\n"); s_problems++; }
    printf("%d chapitres, %d pages, %d cartes, %d calculateurs\n", g_chapterCount, g_index.pageCount,
           g_index.cardCount, calcCount());
    // ---------------------------------------------------------------- fiches
    DisplayList dl;
    dl.init(s_ops, 30000);
    int maxOps = 0, maxZoomOps = 0;
    for (int p = 0; p < g_index.pageCount; p++) {
        const PageRef &pg = g_index.pages[p];
        int e0 = math::g_errors, m0 = font::g_missing;
        PageLayout L;
        layoutPage(dl, p, L);
        char title[96];
        txt::copy(title, sizeof(title), pg.title, pg.titleLen);
        if (dl.n > maxOps) maxOps = dl.n;
        if (L.errors || L.overflows || math::g_errors != e0 || font::g_missing != m0 || dl.n > 3200 || dl.overflow) {
            s_problems++;
            printf("PAGE %d [%s / %s] : erreurs=%d (%s) larges=%d glyphes=%d (U+%X) ops=%d\n", p,
                   g_chapters[pg.chapter].label, title, L.errors, math::g_errMsg, L.overflows,
                   font::g_missing - m0, font::g_lastMissing, dl.n);
        }
        if (pngDir) {
            char file[256];
            snprintf(file, sizeof(file), "%s/p%03d.ppm", pngDir, p);
            savePPM(file, dl, L.height);
        }
    }
    s_quietOverflow = true;
    for (int c = 0; c < g_index.cardCount; c++) {
        int e0 = math::g_errors, m0 = font::g_missing;
        PageLayout L;
        layoutCardZoom(dl, c, L);
        if (dl.n > maxZoomOps) maxZoomOps = dl.n;
        if (L.errors || math::g_errors != e0 || font::g_missing != m0 || dl.n > 1400 || dl.overflow) {
            s_problems++;
            printf("ZOOM carte %d : erreurs=%d glyphes=%d ops=%d\n", c, L.errors, font::g_missing - m0, dl.n);
        }
    }
    s_quietOverflow = false;
    printf("ops max : page %d (capacite 3200), zoom %d (capacite 1400)\n", maxOps, maxZoomOps);
    // ---------------------------------------------------------- calculateurs
    for (int i = 0; i < calcCount(); i++) {
        const CalcDef &d = calcDef(i);
        if (calcPage(i) < 0) { s_problems++; printf("CALC %s : pas de page de formules\n", d.title); }
        // pages de formules orphelines : verifiees plus bas
        g_app.openCalc(i);
        drawCheck(d.title, "ouverture");
        // chaque choix, chaque valeur
        for (int pass = 0; pass < 2; pass++) {
            for (int r = 0; r < g_app.rows.n; r++) {
                const Row row = g_app.rows.r[r];
                if (row.kind != RK_CHOICE) continue;
                int n = 1;
                for (const char *p = row.text; *p; p++) if (*p == '|') n++;
                for (int k = 0; k < n; k++) {
                    // selectionner la ligne et changer la valeur
                    g_app.rowSel = r;
                    g_states[i].c[row.slot] = (uint8_t)k;
                    calcDef(i).compute(g_states[i]);
                    g_app.openCalc(i);
                    g_app.rowSel = r < g_app.rows.n ? r : 0;
                    char det[64];
                    snprintf(det, sizeof det, "choix %s = %d", row.label ? row.label : "?", k);
                    drawCheck(d.title, det);
                    // toutes les lignes de cet etat
                    for (int q = 0; q < g_app.rows.n; q++) {
                        g_app.rowSel = q;
                        drawCheck(d.title, det);
                    }
                    if (d.graph != G_NONE && !(d.flags & CF_LIST)) {
                        g_app.openGraph();
                        drawCheck(d.title, "graphe");
                        g_app.view = V_CALC;
                    }
                }
                calcReset(i);
                g_app.openCalc(i);
            }
            if (pass == 0) { calcReset(i); g_app.openCalc(i); }
        }
        // largeur des etiquettes et valeurs a l'etat par defaut
        calcReset(i);
        g_app.openCalc(i);
        for (int q = 0; q < g_app.rows.n; q++) {
            const Row &row = g_app.rows.r[q];
            if (row.kind == RK_HEAD) {
                if (font::width(BOLD_12, row.text) > FIELDS_W - 14) {
                    s_problems++;
                    printf("CALC %s : titre trop long [%s]\n", d.title, row.text);
                }
                continue;
            }
            if (row.kind == RK_MSG) continue;
            int lw = tex::width(row.label, 2);
            char buf[64];
            int vw;
            if (row.kind == RK_CHOICE) {
                // choix le plus large
                vw = 0;
                const char *p = row.text;
                while (*p) {
                    int n = 0;
                    while (p[n] && p[n] != '|') n++;
                    char t[64];
                    memcpy(t, p, n < 63 ? n : 63);
                    t[n < 63 ? n : 63] = 0;
                    int w = font::width(SANS_14, t) + 18;
                    if (w > vw) vw = w;
                    p += n;
                    if (*p == '|') p++;
                }
            } else {
                fmtValue(buf, sizeof buf, g_states[i].v[row.slot], row.unit);
                vw = font::width(BOLD_14, buf);
            }
            if (6 + lw + 8 + vw > FIELDS_W - 10) {
                printf("  (info) %s : ligne [%s] serree : etiquette %d + valeur %d\n", d.title,
                       row.label ? row.label : "", lw, vw);
            }
        }
        if (d.graph != G_NONE && !(d.flags & CF_LIST)) {
            g_app.openGraph();
            drawCheck(d.title, "graphe");
            if (d.graph == G_BODE) {
                for (int k = 0; k < 12; k++) {
                    g_app.key(k < 6 ? AK_DOWN : AK_UP, 0, 0);
                    g_app.key(AK_RIGHT, 0, 0);
                    drawCheck(d.title, "bode zoom");
                }
            }
            g_app.view = V_CALC;
        }
        // saisie
        g_app.openCalc(i);
        for (int q = 0; q < g_app.rows.n; q++) {
            if (g_app.rows.r[q].kind != RK_IN) continue;
            g_app.rowSel = q;
            g_app.key(AK_DIGIT, '4', 'p');
            g_app.key(AK_DOT, 0, ' ');
            g_app.key(AK_DIGIT, '7', 'm');
            g_app.key(AK_EXP, 0, 0);
            g_app.key(AK_NEG, 0, 0);
            g_app.key(AK_DIGIT, '3', 'w');
            drawCheck(d.title, "saisie");
            g_app.key(AK_EXIT, 0, 0);
            break;
        }
        // fiche de formules
        g_app.key(AK_F1, 0, 0);
        drawCheck(d.title, "formules");
        g_app.key(AK_EXIT, 0, 0);
    }
    // pages sans calculateur (hors aide)
    for (int p = 0; p < g_index.pageCount; p++) {
        const PageRef &pg = g_index.pages[p];
        if (pg.chapter == g_chapterCount - 1) continue;
        bool used = false;
        for (int i = 0; i < calcCount(); i++) if (calcPage(i) == p) used = true;
        // pages de reference autorisees (sans calculateur)
        static const char *const refPages[] = {"Algèbre de Boole", "Théorèmes de De Morgan et opérateurs",
                                               "Formes canoniques : mintermes et maxtermes", nullptr};
        bool ref = false;
        for (int r = 0; refPages[r]; r++)
            if ((int)strlen(refPages[r]) == pg.titleLen && !strncmp(refPages[r], pg.title, pg.titleLen)) ref = true;
        if (!used && !ref) {
            s_problems++;
            printf("PAGE sans calculateur : %.*s\n", pg.titleLen, pg.title);
        }
    }
    // accueil, listes, aide, recherche
    g_app.view = V_HOME;
    drawCheck("accueil", "");
    for (int sct = 0; sct < g_sectionCount; sct++) {
        g_app.openSection(sct, 0);
        drawCheck(g_sections[sct].title, "liste");
    }
    g_app.openFormulas(g_chapterCount - 1, -1, V_HOME);
    drawCheck("aide", "");
    printf("memoire de pointe : boites %d o, noeuds %d o\n", math::boxBytesPeak(), math::nodeBytesPeak());
    printf(s_problems ? "%d PROBLEME(S)\n" : "OK\n", s_problems);
    return s_problems ? 1 : 0;
}
