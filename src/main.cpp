// Electronique pour Casio Graph 90+E (gint) : calculateurs de composants,
// circuits, filtres, AOP et transistors, avec schemas et formules.
//
// La boucle principale est dans main() : quand on appuie sur MENU,
// gint_osmenu() est appele avec une pile presque vide (meme methode que
// CASIO_GAME, EM_FIELD et Formulaire). L'application est une variable
// globale : son etat (valeurs saisies) est conserve en revenant dans l'appli.
#include <stddef.h>
#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/gint.h>
#include "ui/App.hpp"
#include "ui/KeyMap.hpp"

int main(void)
{
    gint_setrestart(1);
    g_app.init();
    Canvas cv;
    for (;;) {
        cv.begin(gint_vram);
        g_app.draw(cv);
        dupdate();
        key_event_t ev = getkey_opt(GETKEY_REP_ARROWS, NULL);
        if (ev.key == KEY_MENU) {
            gint_osmenu();
            continue;
        }
        char ch, letter;
        int k = mapGintKey(ev.key, ch, letter);
        if (k == AK_NONE) continue;
        if (!g_app.key(k, ch, letter)) break;
    }
    return 1;
}
