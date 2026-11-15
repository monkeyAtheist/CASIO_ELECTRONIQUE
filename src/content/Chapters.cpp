// Fiches de formules : un chapitre par section (meme ordre que g_sections),
// une page par calculateur (meme titre), puis l'aide.
#include "../doc/Content.hpp"
#include "../gfx/Color.hpp"

extern const char CH_ASSOC[];
extern const char CH_RLC[];
extern const char CH_FILTRES[];
extern const char CH_AOP[];
extern const char CH_BJT[];
extern const char CH_FET[];
extern const char CH_BASES[];
extern const char CH_ALIM[];
extern const char CH_CODE[];
extern const char CH_OSC[];
extern const char CH_QUAD[];
extern const char CH_MAG[];
extern const char CH_LOGIC[];
extern const char CH_MCU[];
extern const char CH_MOTOR[];
extern const char CH_RF[];
extern const char CH_SIG[];
extern const char CH_AIDE[];

const Chapter g_chapters[] = {
    {"Associations de composants", "Associations", "", rgb(30, 90, 190), CH_ASSOC, 1},
    {"Circuits RC, RL, LC et RLC", "RC, RL, RLC", "", rgb(0, 120, 125), CH_RLC, 1},
    {"Filtres passifs et actifs", "Filtres", "", rgb(25, 125, 60), CH_FILTRES, 1},
    {"Amplificateurs opérationnels", "AOP", "", rgb(200, 95, 0), CH_AOP, 1},
    {"Transistors bipolaires", "Bipolaires", "", rgb(115, 60, 175), CH_BJT, 1},
    {"Transistors à effet de champ", "MOSFET, JFET", "", rgb(170, 40, 120), CH_FET, 1},
    {"Lois de base", "Lois de base", "", rgb(200, 45, 40), CH_BASES, 1},
    {"Alimentations et régulation", "Alimentations", "", rgb(110, 80, 65), CH_ALIM, 1},
    {"Code couleur et séries E", "Code couleur", "", rgb(75, 60, 180), CH_CODE, 1},
    {"Oscillateurs et temporisations", "Oscillateurs", "", rgb(150, 105, 20), CH_OSC, 1},
    {"Quadripôles", "Quadripôles", "", rgb(55, 85, 125), CH_QUAD, 1},
    {"Transformateurs et bobines", "Transfos, bobines", "", rgb(175, 75, 25), CH_MAG, 1},
    {"Logique combinatoire", "Logique", "", rgb(0, 135, 190), CH_LOGIC, 1},
    {"Timers et microcontrôleurs", "Timers, µC", "", rgb(95, 125, 0), CH_MCU, 1},
    {"Optocoupleurs et moteurs", "Opto, moteurs", "", rgb(140, 55, 70), CH_MOTOR, 1},
    {"Antennes et radiofréquences", "Antennes, RF", "", rgb(190, 50, 90), CH_RF, 1},
    {"Traitement du signal", "Signal", "", rgb(20, 120, 100), CH_SIG, 1},
    {"Aide", "Aide", "", rgb(60, 70, 90), CH_AIDE, 1},
};
const int g_chapterCount = sizeof(g_chapters) / sizeof(g_chapters[0]);
