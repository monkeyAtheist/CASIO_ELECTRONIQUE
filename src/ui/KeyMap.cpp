#include "KeyMap.hpp"
#include "App.hpp"
#include <gint/keyboard.h>

int mapGintKey(int key, char &ch, char &letter)
{
    ch = 0;
    letter = 0;
    switch (key) {
    case KEY_UP: return AK_UP;
    case KEY_DOWN: return AK_DOWN;
    case KEY_LEFT: return AK_LEFT;
    case KEY_RIGHT: return AK_RIGHT;
    case KEY_EXE: return AK_EXE;
    case KEY_EXIT: return AK_EXIT;
    case KEY_SHIFT: return AK_SHIFT;
    case KEY_ALPHA: return AK_ALPHA;
    case KEY_F1: return AK_F1;
    case KEY_F2: return AK_F2;
    case KEY_F3: return AK_F3;
    case KEY_F4: return AK_F4;
    case KEY_F5: return AK_F5;
    case KEY_F6: return AK_F6;
    case KEY_DEL: return AK_DEL;
    case KEY_ACON: return AK_AC;
    case KEY_OPTN: return AK_OPTN;
    case KEY_VARS: return AK_VARS;
    case KEY_EXP: return AK_EXP;
    case KEY_NEG: return AK_NEG;
    default: break;
    }
    // chiffres (et leurs lettres rouges)
    static const struct { int key; char d, l; } digits[] = {
        {KEY_0, '0', 'z'}, {KEY_1, '1', 'u'}, {KEY_2, '2', 'v'}, {KEY_3, '3', 'w'},
        {KEY_4, '4', 'p'}, {KEY_5, '5', 'q'}, {KEY_6, '6', 'r'}, {KEY_7, '7', 'm'},
        {KEY_8, '8', 'n'}, {KEY_9, '9', 'o'},
    };
    for (unsigned i = 0; i < sizeof(digits) / sizeof(digits[0]); i++) {
        if (digits[i].key == key) {
            ch = digits[i].d;
            letter = digits[i].l;
            return AK_DIGIT;
        }
    }
    if (key == KEY_DOT) { letter = ' '; return AK_DOT; }
    if (key == KEY_SUB) { letter = 'y'; return AK_SUB; }
    if (key == KEY_ADD) { letter = 'x'; return AK_ADD; }
    if (key == KEY_MUL) { letter = 's'; return AK_MUL; }
    if (key == KEY_DIV) { letter = 't'; return AK_DIV; }
    if (key == KEY_LEFTP) { letter = 'i'; return AK_LPAR; }
    if (key == KEY_RIGHTP) { letter = 'j'; return AK_RPAR; }
    if (key == KEY_XOT) { letter = 'a'; return AK_XOT; }
    // autres lettres rouges du clavier de la Graph 90+E
    static const struct { int key; char ch; } letters[] = {
        {KEY_XOT, 'a'}, {KEY_LOG, 'b'}, {KEY_LN, 'c'}, {KEY_SIN, 'd'}, {KEY_COS, 'e'},
        {KEY_TAN, 'f'}, {KEY_FRAC, 'g'}, {KEY_FD, 'h'}, {KEY_LEFTP, 'i'}, {KEY_RIGHTP, 'j'},
        {KEY_COMMA, 'k'}, {KEY_ARROW, 'l'}, {KEY_MUL, 's'}, {KEY_DIV, 't'}, {KEY_ADD, 'x'},
    };
    for (unsigned i = 0; i < sizeof(letters) / sizeof(letters[0]); i++) {
        if (letters[i].key == key) {
            letter = letters[i].ch;
            return AK_CHAR;
        }
    }
    return AK_NONE;
}
