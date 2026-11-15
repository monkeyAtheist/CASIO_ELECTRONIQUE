// Dessin des schemas electroniques (symboles normalises IEC, usage francais) :
// fils, noeuds, masses, resistances (rectangles), condensateurs, bobines,
// impedances composees, diodes, sources, AOP, transistors bipolaires, MOSFET
// et JFET, fleches de tension et de courant.
//
// Coordonnees relatives a l'origine du panneau (ox, oy). Les dipoles sont
// places entre deux points alignes (horizontaux ou verticaux) et dessinent
// leurs fils de raccordement. Le composant dont l'etiquette (tag) est
// selectionnee dans la liste des champs est dessine dans la couleur du theme.
#pragma once
#include <stdint.h>
#include "../gfx/Canvas.hpp"

struct Sch {
    Canvas *cv;
    int ox, oy;
    uint16_t ink;      // traits
    uint16_t accent;   // composant selectionne
    uint16_t light;    // fond du composant selectionne
    uint16_t paper;    // fond
    uint16_t label;    // etiquettes
    uint16_t volt;     // fleches de tension
    int hilite;        // tag selectionne (0 : aucun)

    bool on(int tag) const { return tag && tag == hilite; }
    uint16_t col(int tag) const { return on(tag) ? accent : ink; }

    // fils
    void wire(int x0, int y0, int x1, int y1, int tag = 0);
    // fil en L : horizontal puis vertical (hv) ou vertical puis horizontal
    void wireHV(int x0, int y0, int x1, int y1, int tag = 0);
    void wireVH(int x0, int y0, int x1, int y1, int tag = 0);
    void dot(int x, int y);
    void term(int x, int y);                  // borne (petit cercle)
    void gnd(int x, int y, int tag = 0);      // masse sous (x, y)
    void rail(int x, int y, const char *texLabel, int tag = 0);   // alimentation (barre + etiquette)
    void railDown(int x, int y, const char *texLabel, int tag = 0); // alimentation negative

    // dipoles (points alignes)
    void res(int x0, int y0, int x1, int y1, int tag = 0);
    void cap(int x0, int y0, int x1, int y1, int tag = 0);
    void ind(int x0, int y0, int x1, int y1, int tag = 0);
    void zdip(int x0, int y0, int x1, int y1, int ztype, int tag = 0);
    // diode : anode (x0, y0) -> cathode (x1, y1) ; kind 0 diode, 1 LED, 2 Zener
    void diode(int x0, int y0, int x1, int y1, int kind, int tag = 0);
    // source verticale entre ytop et ybot ; kind 0 continue, 1 alternative, 2 courant
    void vsrc(int x, int ytop, int ybot, int kind, int tag = 0);
    // source de courant (dipole) : fleche vers (x1, y1)
    void isrc(int x0, int y0, int x1, int y1, int tag = 0);
    // source de tension commandee (losange) : + du cote (x0, y0)
    void vcsrc(int x0, int y0, int x1, int y1, int tag = 0);
    // source alternative horizontale entre x0 et x1
    void acH(int x0, int x1, int y, int tag = 0);
    // interrupteur (ouvert) horizontal
    void sw(int x0, int y, int x1, int tag = 0);
    // potentiometre : resistance + fleche du curseur
    void pot(int x0, int y0, int x1, int y1, int tag = 0);

    // AOP : (x, y) milieu du cote gauche, largeur w (hauteur = w + 4).
    // Entrees en (x, y -/+ inDy(w)), sortie en (x + w, y).
    void opamp(int x, int y, int w, bool minusTop, int tag = 0);
    static int inDy(int w) { return (w + 4) / 4; }
    // Bipolaire : base en (x, y) ; bornes en (x + 17, y -/+ 20).
    // NPN : collecteur en haut ; PNP : emetteur en haut.
    void bjt(int x, int y, bool npn, int tag = 0);
    // MOSFET a enrichissement : grille (x, y) ; bornes (x + 18, y -/+ 20).
    // Canal N : drain en haut ; canal P : source en haut.
    void mosfet(int x, int y, bool nch, int tag = 0);
    // JFET : grille (x, y) ; bornes (x + 19, y -/+ 20). N : drain en haut.
    void jfet(int x, int y, bool nch, int tag = 0);
    // Boitier rectangulaire (NE555, LM317...) avec un titre.
    void chip(int x, int y, int w, int h, const char *title, int tag = 0);

    // etiquettes (LaTeX ou '#texte') ; align 0 gauche, 1 centre, 2 droite
    void tex(int x, int base, const char *s, int align = 0, int tag = 0);
    void texSmall(int x, int base, const char *s, int align = 0, int tag = 0);
    // fleche de tension verticale de y0 vers y1 (pointe en y1), etiquette a
    // gauche (side < 0) ou a droite (side > 0)
    void varrow(int x, int y0, int y1, const char *texLabel, int side, int tag = 0);
    // fleche de tension horizontale de x0 vers x1, etiquette dessus (side < 0) ou dessous
    void harrow(int x0, int x1, int y, const char *texLabel, int side, int tag = 0);
    // fleche de courant sur un fil (pointe en (x, y), sens dx, dy)
    void iarrow(int x, int y, int dx, int dy, const char *texLabel, int lx, int ly, int tag = 0);

    // primitives en coordonnees du panneau
    void line(int x0, int y0, int x1, int y1, uint16_t c);
    void rect(int x, int y, int w, int h, uint16_t c);
    void frame(int x, int y, int w, int h, uint16_t c);
};

