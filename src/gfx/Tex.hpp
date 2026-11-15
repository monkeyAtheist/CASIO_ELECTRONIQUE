// Petites formules dessinees directement (etiquettes des champs et des
// schemas) avec le moteur de formules de Formulaire.
// Une chaine qui commence par '#' est un texte simple (police DejaVu Sans).
#pragma once
#include <stdint.h>
#include "Canvas.hpp"

namespace tex {

enum { LEFT = 0, CENTER = 1, RIGHT = 2 };

// Style : 1 = texte (math::ST_T), 2 = indice (plus petit).
int width(const char *s, int style = 1);
// Dessine, ligne de base en y. Renvoie la largeur.
int draw(Canvas &cv, const char *s, int x, int base, uint16_t c, int align = LEFT, int style = 1);
// Texte simple : police de texte de la taille correspondant au style.
int textFace(int style);

}  // namespace tex
