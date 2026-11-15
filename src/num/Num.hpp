// Unites, formatage des valeurs (prefixes SI, virgule decimale), saisie,
// series normalisees E6 a E96.
#pragma once
#include <stdint.h>
#include "FMath.hpp"

enum Unit : uint8_t {
    U_NONE,    // nombre sans unite (gain, beta, Q...)
    U_OHM, U_F, U_H, U_V, U_A, U_W, U_HZ, U_S, U_SIE, U_AV2, U_RADS, U_COUL, U_J, U_WB,
    U_DB, U_DBM, U_DBW, U_DBV, U_DBU, U_DEG, U_PCT, U_DEGC, U_CW,
    // avec prefixes SI
    U_M, U_T, U_NM, U_BPS, U_BAUD, U_MPS, U_VSR, U_BYTE, U_VA, U_VAR,
    // sans prefixe
    U_MM2, U_CM2, U_RPM, U_K, U_AMM2,
    // decibels
    U_DBI, U_DBC, U_DBMHZ,
    U_COUNT
};

// Unite avec prefixes SI (k, m, µ...) ?
bool unitHasPrefix(uint8_t u);
// Symbole de l'unite (UTF-8), sans prefixe.
const char *unitSymbol(uint8_t u);

// Prefixes : indice 0 = f (1e-15) ... 5 = sans prefixe ... 9 = T (1e12).
enum { PFX_F, PFX_P, PFX_N, PFX_U, PFX_M, PFX_1, PFX_K, PFX_MEGA, PFX_G, PFX_T, PFX_COUNT };
const char *prefixSymbol(int p);
int prefixExp(int p);

// Valeur + unite : "4,7 kΩ", "−3,01 dB", "1,59 kHz", "—" si NaN.
// sig : chiffres significatifs (zeros inutiles retires).
int fmtValue(char *out, int cap, double v, uint8_t unit, int sig = 4);
// Nombre seul (sans unite ni prefixe) : "12,5", "1,2×10⁻⁵".
int fmtNumber(char *out, int cap, double v, int sig = 4);
// Entier exact (|v| < 10^15), chiffres groupes par 3 : "65 536".
int fmtInt(char *out, int cap, double v);
// Complexe : "1,2 kΩ + j560 Ω" ou, en polaire, "1,3 kΩ ∠ 25,0°".
int fmtComplex(char *out, int cap, double re, double im, uint8_t unit, bool polar, int sig = 4);

// Saisie : chiffres, '.', 'E' (puissance de 10), '-'. Renvoie false si
// la chaine n'est pas un nombre.
bool parseNumber(const char *s, double &v);
// Affichage d'une saisie en cours : '.' -> ',', "E-3" -> "×10⁻³".
int fmtInput(char *out, int cap, const char *s);

// Series normalisees.
enum ESeries { E6, E12, E24, E48, E96, ESERIES_COUNT };
int eCount(int series);
int eMantissa(int series, int i);           // x100 : 100 ... 976
double eNearest(int series, double x);      // valeur la plus proche (ecart relatif)
double eStep(int series, double x, int dir);  // valeur strictement au-dessus / au-dessous
double eCeil(int series, double x);         // plus petite valeur >= x
double eFloor(int series, double x);        // plus grande valeur <= x
double step125(double x, int dir);          // suite 1, 2, 5, 10...

// Copie UTF-8 bornee, renvoie la longueur.
int scopy(char *dst, int cap, const char *src);
