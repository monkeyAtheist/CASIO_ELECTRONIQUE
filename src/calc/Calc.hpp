// Modele des calculateurs : chaque calculateur decrit ses lignes (entrees,
// choix, resultats), calcule ses resultats, dessine son schema et, s'il y a
// lieu, fournit une fonction de transfert (diagramme de Bode) ou un graphe.
//
// L'etat (valeurs saisies et resultats) de chaque calculateur est conserve
// pendant toute la session dans g_states.
#pragma once
#include <stdint.h>
#include "../num/Num.hpp"
#include "../num/Cx.hpp"

struct Canvas;
struct Sch;

// Pas des fleches gauche/droite sur une valeur.
enum Step : uint8_t {
    ST_NONE, ST_E24, ST_E12, ST_E6, ST_125, ST_LIN1, ST_LIN01, ST_LIN10, ST_INT, ST_LIN5,
    ST_POW2,   // x2 / /2 (diviseurs, nombres de points)
    ST_E96,
};

enum RowKind : uint8_t { RK_IN, RK_OUT, RK_CHOICE, RK_HEAD, RK_MSG, RK_TEXT };

enum RowFlags : uint8_t {
    RF_POS = 1,      // valeur > 0 obligatoire
    RF_NONNEG = 2,   // valeur >= 0
    RF_MAIN = 4,     // resultat principal (gras)
    RF_INT = 8,      // entier (saisie arrondie ; resultat affiche en entier exact)
    RF_CPLX = 16,    // resultat complexe : v[slot] + j v[slot+1]
    RF_POLAR = 32,   // complexe affiche en module et argument
    RF_TEX = 64,     // message (RK_MSG) en LaTeX, avec etiquette
};

struct Row {
    uint8_t kind, unit, step, slot;
    uint8_t tag;          // composant mis en evidence sur le schema (0 : aucun)
    uint8_t flags;
    const char *label;    // LaTeX ; commence par '#' : texte simple
    const char *text;     // choix "A|B|C" ; titre (HEAD)
    const char *desc;     // ligne d'aide en bas de l'ecran
};

enum { MAX_ROWS = 48, NV = 48, NC = 24, NMSG = 6, MAX_LIST = 12, MAX_CALCS = 104, NTXT = 2, TXT_LEN = 40 };

enum MsgLevel : uint8_t { ML_INFO, ML_OK, ML_WARN, ML_BAD };

struct CalcState {
    double v[NV];
    uint8_t c[NC];
    uint8_t n;               // nombre d'elements (associations)
    uint8_t ready;
    const char *msg[NMSG];   // messages calcules (RK_MSG)
    uint8_t lvl[NMSG];
    char t[NTXT][TXT_LEN];   // textes saisis (RK_TEXT)
};

struct RowList {
    Row r[MAX_ROWS];
    int n;

    Row &add(uint8_t kind, const char *label, int slot, uint8_t unit, uint8_t step,
             int tag, const char *desc)
    {
        static Row sink;
        if (n >= MAX_ROWS) return sink;
        Row &w = r[n++];
        w.kind = kind; w.label = label; w.slot = (uint8_t)slot; w.unit = unit;
        w.step = step; w.tag = (uint8_t)tag; w.flags = 0; w.text = nullptr; w.desc = desc;
        return w;
    }
    // Entree numerique.
    Row &in(const char *label, int slot, uint8_t unit, uint8_t step, int tag = 0,
            const char *desc = nullptr, uint8_t flags = RF_POS)
    {
        Row &w = add(RK_IN, label, slot, unit, step, tag, desc);
        w.flags = flags;
        return w;
    }
    // Resultat.
    Row &out(const char *label, int slot, uint8_t unit, const char *desc = nullptr,
             bool main = false, int tag = 0)
    {
        Row &w = add(RK_OUT, label, slot, unit, ST_NONE, tag, desc);
        if (main) w.flags = RF_MAIN;
        return w;
    }
    // Choix parmi "A|B|C" (indice dans CalcState::c).
    Row &choice(const char *label, int slot, const char *choices, int tag = 0,
                const char *desc = nullptr)
    {
        Row &w = add(RK_CHOICE, label, slot, U_NONE, ST_NONE, tag, desc);
        w.text = choices;
        return w;
    }
    void head(const char *title)
    {
        Row &w = add(RK_HEAD, nullptr, 0, U_NONE, ST_NONE, 0, nullptr);
        w.text = title;
    }
    // Message calcule (CalcState::msg[slot]).
    void msg(int slot, const char *desc = nullptr) { add(RK_MSG, nullptr, slot, U_NONE, ST_NONE, 0, desc); }
    // Resultat sous forme de formule LaTeX (CalcState::msg[slot]) avec etiquette.
    Row &texOut(const char *label, int slot, const char *desc = nullptr, int tag = 0)
    {
        Row &w = add(RK_MSG, label, slot, U_NONE, ST_NONE, tag, desc);
        w.flags = RF_TEX;
        return w;
    }
    // Resultat complexe (v[slot], v[slot+1]).
    Row &outC(const char *label, int slot, uint8_t unit, bool polar, const char *desc = nullptr,
              bool main = false, int tag = 0)
    {
        Row &w = out(label, slot, unit, desc, main, tag);
        w.flags |= RF_CPLX | (polar ? RF_POLAR : 0);
        return w;
    }
    // Resultat entier exact.
    Row &outI(const char *label, int slot, const char *desc = nullptr, bool main = false, int tag = 0)
    {
        Row &w = out(label, slot, U_NONE, desc, main, tag);
        w.flags |= RF_INT;
        return w;
    }
    // Texte saisi (CalcState::t[slot]).
    Row &text(const char *label, int slot, int tag = 0, const char *desc = nullptr)
    {
        return add(RK_TEXT, label, slot, U_NONE, ST_NONE, tag, desc);
    }
};

// Clavier de saisie d'un texte (RK_TEXT) : touches F1 a F6 (et SHIFT),
// caracteres des touches + x ( ) et chiffres permis.
struct TextKeys {
    const char *lab[6], *ins[6];
    const char *labS[6], *insS[6];
    char add, mul, lpar, rpar;
    const char *digits;
};

// Graphes
enum GraphKind : uint8_t { G_NONE, G_BODE, G_CUSTOM };
enum CalcFlags : uint8_t { CF_LIST = 1, CF_DIGITCHOICE = 2 };

struct GraphArea {
    int x, y, w, h;
};

struct CalcDef {
    const char *title;     // titre (aussi titre de la page de formules)
    const char *sub;       // sous-titre de la liste
    const char *tags;      // mots cles (recherche)
    uint8_t section;
    uint8_t graph;
    uint8_t flags;
    void (*defaults)(CalcState &s);
    void (*rows)(const CalcState &s, RowList &r);
    void (*compute)(CalcState &s);
    void (*schema)(Sch &g, const CalcState &s);
    // Bode : H(f) ; centre conseille des frequences (Hz) ou 0.
    bool (*transfer)(const CalcState &s, double f, Cx &H);
    double (*fcenter)(const CalcState &s);
    // Graphe personnalise (hysteresis, charge...).
    void (*drawGraph)(Canvas &cv, const CalcState &s, const GraphArea &a, uint16_t accent);
    const char *graphLabel;   // touche F4 (defaut "Bode" / "Graphe")
    uint8_t fslot1;           // champ frequence (indice + 1, 0 : aucun) : F3 du Bode
    // Saisie de texte (RK_TEXT) : clavier, rendu LaTeX, verification.
    const TextKeys *tkeys;
    int (*toTex)(const char *src, char *out, int cap);
    const char *(*textCheck)(const char *src);   // message d'erreur ou nullptr
    uint8_t decades;          // largeur initiale du Bode en decades (0 : 5)
};

struct Section {
    const char *title;
    const char *label;
    uint16_t color;
    void (*icon)(Canvas &cv, int cx, int cy, uint16_t ink, uint16_t bg);
};

extern const Section g_sections[];
extern const int g_sectionCount;
extern CalcState g_states[];

// Registre (calc/Registry.cpp) : tous les calculateurs, section par section.
void registryInit();
const CalcDef &calcDef(int i);
int calcCount();
int calcPage(int i);                      // page de formules (-1 si aucune)
int calcIndexInSection(int i, int &count);

// Etat initialise (valeurs par defaut au premier acces).
CalcState &calcState(int i);
void calcReset(int i);

// --------------------------------------------------------------------------
// Outils communs aux calculateurs
// --------------------------------------------------------------------------
// Impedance d'un dipole de type ZT_* a la pulsation w.
enum ZType : uint8_t { ZT_R, ZT_C, ZT_L, ZT_RC_S, ZT_RC_P, ZT_RL_S, ZT_RL_P, ZT_COUNT };
extern const char *const Z_CHOICES;   // "R|C|L|R+C|R∥C|R+L|R∥L"
Cx zEval(int type, double R, double C, double L, double w);
bool zHasR(int t);
bool zHasC(int t);
bool zHasL(int t);

// Ajoute les lignes d'une impedance generale : choix du type + valeurs.
// labelZ "Z_1", labels des valeurs "R_1", "C_1", "L_1".
void zRows(RowList &r, const CalcState &s, int cslot, int rslot, int cslotC, int lslot,
           const char *labelZ, const char *labelR, const char *labelC, const char *labelL, int tag);
Cx zOf(const CalcState &s, int cslot, int rslot, int cslotC, int lslot, double w);

// Frequence caracteristique d'un ensemble de composants (centre du Bode).
double fCorner(const double *R, int nR, const double *C, int nC, const double *L, int nL);

// Utilitaires
inline double NaN() { return fm::nan(); }
inline bool ok(double x) { return fm::isfinite(x); }
void setMsg(CalcState &s, int k, const char *m, uint8_t lvl);
