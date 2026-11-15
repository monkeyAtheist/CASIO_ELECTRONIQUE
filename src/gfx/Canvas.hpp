// Dessin direct dans la VRAM avec decoupage (clip) et transparence 4 bits.
#pragma once
#include <stdint.h>
#include "Color.hpp"

enum { SCREEN_W = 396, SCREEN_H = 224 };

struct Canvas {
    uint16_t *fb = nullptr;
    int cx0 = 0, cy0 = 0, cx1 = SCREEN_W, cy1 = SCREEN_H;   // clip [x0,x1[

    void begin(uint16_t *vram);
    void setClip(int x0, int y0, int x1, int y1);
    void resetClip();

    void fill(uint16_t c);
    void fillRect(int x, int y, int w, int h, uint16_t c);
    void blendRect(int x, int y, int w, int h, uint16_t c, int alpha16);
    void hline(int x, int y, int w, uint16_t c) { fillRect(x, y, w, 1, c); }
    void vline(int x, int y, int h, uint16_t c) { fillRect(x, y, 1, h, c); }
    void frame(int x, int y, int w, int h, uint16_t c);
    // Rectangle a coins arrondis (rayon 1 a 4), rempli et/ou borde.
    void roundRect(int x, int y, int w, int h, int r, uint16_t fill, bool doFill,
                   uint16_t border, bool doBorder);

    // Bitmap alpha 4 bits (voir FontTypes.hpp). (x, y) = coin haut gauche.
    void alpha4(const uint8_t *bits, int w, int h, int x, int y, uint16_t c);
    // Repete une ligne du bitmap 'count' fois vers le bas.
    void alpha4RowRepeat(const uint8_t *row, int w, int x, int y, int count, uint16_t c);
    // Repete une colonne (bitmap de largeur 1) 'count' fois vers la droite.
    void alpha4ColRepeat(const uint8_t *bits, int h, int x, int y, int count, uint16_t c);

    // Formes anticrenelees (schemas, graphes). Coordonnees = centres des pixels.
    void line(int x0, int y0, int x1, int y1, uint16_t c);
    // Ligne en coordonnees 1/16 de pixel (courbes des graphes).
    void lineSub(int x0, int y0, int x1, int y1, uint16_t c);
    void disc(int cx, int cy, int r, uint16_t c);
    void circle(int cx, int cy, int r, uint16_t c);
    // Demi-cercle : half 0 haut, 1 droite, 2 bas, 3 gauche.
    void halfCircle(int cx, int cy, int r, int half, uint16_t c);
    void fillTri(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t c);
    // Pointe de fleche en (x, y) dirigee vers (dx, dy) (un des 8 sens).
    void arrowHead(int x, int y, int dx, int dy, int size, uint16_t c);

    inline void blendPixel(int x, int y, uint16_t c, int a15)
    {
        if (x < cx0 || x >= cx1 || y < cy0 || y >= cy1 || a15 <= 0) return;
        uint16_t *p = fb + y * SCREEN_W + x;
        if (a15 >= 15) { *p = c; return; }
        *p = blend15(*p, c, a15);
    }
    static uint16_t blend15(uint16_t d, uint16_t s, int a);
};
