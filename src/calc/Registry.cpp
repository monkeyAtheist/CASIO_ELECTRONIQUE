// Registre des sections et des calculateurs.
#include "Calc.hpp"
#include "../doc/Index.hpp"
#include "../doc/Content.hpp"
#include "../gfx/Color.hpp"
#include <string.h>

// Icones de l'accueil (sch/Icons.cpp)
void iconAssoc(Canvas &, int, int, uint16_t, uint16_t);
void iconRlc(Canvas &, int, int, uint16_t, uint16_t);
void iconFilter(Canvas &, int, int, uint16_t, uint16_t);
void iconAop(Canvas &, int, int, uint16_t, uint16_t);
void iconBjt(Canvas &, int, int, uint16_t, uint16_t);
void iconFet(Canvas &, int, int, uint16_t, uint16_t);
void iconOhm(Canvas &, int, int, uint16_t, uint16_t);
void iconAlim(Canvas &, int, int, uint16_t, uint16_t);
void iconCode(Canvas &, int, int, uint16_t, uint16_t);
void iconOsc(Canvas &, int, int, uint16_t, uint16_t);
void iconQuad(Canvas &, int, int, uint16_t, uint16_t);
void iconMag(Canvas &, int, int, uint16_t, uint16_t);
void iconLogic(Canvas &, int, int, uint16_t, uint16_t);
void iconMcu(Canvas &, int, int, uint16_t, uint16_t);
void iconMotor(Canvas &, int, int, uint16_t, uint16_t);
void iconRf(Canvas &, int, int, uint16_t, uint16_t);
void iconSig(Canvas &, int, int, uint16_t, uint16_t);

const Section g_sections[] = {
    {"Associations de composants", "Associations", rgb(30, 90, 190), iconAssoc},
    {"Circuits RC, RL, LC et RLC", "RC, RL, RLC", rgb(0, 120, 125), iconRlc},
    {"Filtres passifs et actifs", "Filtres", rgb(25, 125, 60), iconFilter},
    {"Amplificateurs op\xC3\xA9rationnels", "AOP", rgb(200, 95, 0), iconAop},
    {"Transistors bipolaires", "Bipolaires", rgb(115, 60, 175), iconBjt},
    {"Transistors \xC3\xA0 effet de champ", "MOSFET, JFET", rgb(170, 40, 120), iconFet},
    {"Lois de base", "Lois de base", rgb(200, 45, 40), iconOhm},
    {"Alimentations et r\xC3\xA9gulation", "Alimentations", rgb(110, 80, 65), iconAlim},
    {"Code couleur et s\xC3\xA9ries E", "Code couleur", rgb(75, 60, 180), iconCode},
    {"Oscillateurs et temporisations", "Oscillateurs", rgb(150, 105, 20), iconOsc},
    {"Quadrip\xC3\xB4les", "Quadrip\xC3\xB4les", rgb(55, 85, 125), iconQuad},
    {"Transformateurs et bobines", "Transfos, bobines", rgb(175, 75, 25), iconMag},
    {"Logique combinatoire", "Logique", rgb(0, 135, 190), iconLogic},
    {"Timers et microcontr\xC3\xB4leurs", "Timers, \xC2\xB5" "C", rgb(95, 125, 0), iconMcu},
    {"Optocoupleurs et moteurs", "Opto, moteurs", rgb(140, 55, 70), iconMotor},
    {"Antennes et radiofr\xC3\xA9quences", "Antennes, RF", rgb(190, 50, 90), iconRf},
    {"Traitement du signal", "Signal", rgb(20, 120, 100), iconSig},
};
const int g_sectionCount = sizeof(g_sections) / sizeof(g_sections[0]);

// Tableaux de calculateurs (calc/sec_*.cpp)
#define SECTION_TABLE(name) extern const CalcDef name[]; extern const int name##N;
SECTION_TABLE(g_calcAssoc)
SECTION_TABLE(g_calcRlc)
SECTION_TABLE(g_calcFilter)
SECTION_TABLE(g_calcAop)
SECTION_TABLE(g_calcBjt)
SECTION_TABLE(g_calcFet)
SECTION_TABLE(g_calcBase)
SECTION_TABLE(g_calcAlim)
SECTION_TABLE(g_calcCode)
SECTION_TABLE(g_calcOsc)
SECTION_TABLE(g_calcAmpBjt)
SECTION_TABLE(g_calcAmpFet)
SECTION_TABLE(g_calcQuad)
SECTION_TABLE(g_calcMag)
SECTION_TABLE(g_calcLogic)
SECTION_TABLE(g_calcMcu)
SECTION_TABLE(g_calcMotor)
SECTION_TABLE(g_calcRf)
SECTION_TABLE(g_calcSig)

struct Tab {
    const CalcDef *defs;
    const int *n;
};
static const Tab s_tabs[] = {
    {g_calcAssoc, &g_calcAssocN}, {g_calcRlc, &g_calcRlcN}, {g_calcFilter, &g_calcFilterN},
    {g_calcAop, &g_calcAopN}, {g_calcBjt, &g_calcBjtN}, {g_calcAmpBjt, &g_calcAmpBjtN},
    {g_calcFet, &g_calcFetN}, {g_calcAmpFet, &g_calcAmpFetN},
    {g_calcBase, &g_calcBaseN}, {g_calcAlim, &g_calcAlimN}, {g_calcCode, &g_calcCodeN},
    {g_calcOsc, &g_calcOscN},
    {g_calcQuad, &g_calcQuadN}, {g_calcMag, &g_calcMagN}, {g_calcLogic, &g_calcLogicN},
    {g_calcMcu, &g_calcMcuN}, {g_calcMotor, &g_calcMotorN},
    {g_calcRf, &g_calcRfN}, {g_calcSig, &g_calcSigN},
};

CalcState g_states[MAX_CALCS];
static const CalcDef *s_defs[MAX_CALCS];
static int16_t s_page[MAX_CALCS];
static int s_n = 0;

void registryInit()
{
    s_n = 0;
    for (unsigned t = 0; t < sizeof(s_tabs) / sizeof(s_tabs[0]); t++)
        for (int k = 0; k < *s_tabs[t].n && s_n < MAX_CALCS; k++) s_defs[s_n++] = &s_tabs[t].defs[k];
    // page de formules : meme titre, dans le chapitre de la section
    for (int i = 0; i < s_n; i++) {
        s_page[i] = -1;
        int ch = s_defs[i]->section;
        if (ch >= g_chapterCount) continue;
        int first = g_index.chapterFirstPage[ch], cnt = g_index.chapterPageCount[ch];
        int tl = (int)strlen(s_defs[i]->title);
        for (int p = first; p < first + cnt; p++) {
            const PageRef &pg = g_index.pages[p];
            if (pg.titleLen == tl && !strncmp(pg.title, s_defs[i]->title, tl)) { s_page[i] = (int16_t)p; break; }
        }
    }
}

const CalcDef &calcDef(int i) { return *s_defs[i]; }
int calcCount() { return s_n; }
int calcPage(int i) { return (i >= 0 && i < s_n) ? s_page[i] : -1; }

int calcIndexInSection(int i, int &count)
{
    int sec = s_defs[i]->section, idx = 0;
    count = 0;
    for (int k = 0; k < s_n; k++) {
        if (s_defs[k]->section != sec) continue;
        if (k == i) idx = count;
        count++;
    }
    return idx;
}
