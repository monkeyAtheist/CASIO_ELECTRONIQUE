// Execute des calculateurs a partir de leurs etiquettes (pour verify_calcs.py).
// Entree (une ligne par cas) : TITRE<TAB>etiquette=valeur<TAB>...
//   - choix : "#etiquette=indice" ou "?etiquette=indice" (etiquette LaTeX) ;
//   - entree : "etiquette=valeur" ;
//   - "@n" : nombre d'elements d'une liste (associations).
// Sortie : pour chaque cas, une ligne par rangee "genre<TAB>etiquette<TAB>valeur",
// puis "END". Genres : IN, OUT, CHOICE, MSG.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../src/calc/Calc.hpp"
#include "../../src/doc/Index.hpp"

static int findCalc(const char *t)
{
    for (int i = 0; i < calcCount(); i++)
        if (!strcmp(calcDef(i).title, t)) return i;
    return -1;
}

struct KV { char k[64]; double v; char sv[64]; };

int main()
{
    g_index.build();
    registryInit();
    static char line[4096];
    while (fgets(line, sizeof line, stdin)) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = 0;
        if (!line[0]) continue;
        KV kv[40];
        int nkv = 0;
        char *save = nullptr;
        char *tok = strtok_r(line, "\t", &save);
        const char *title = tok;
        while ((tok = strtok_r(nullptr, "\t", &save)) && nkv < 40) {
            char *eq = strrchr(tok, '=');
            if (!eq) continue;
            *eq = 0;
            snprintf(kv[nkv].k, sizeof kv[nkv].k, "%s", tok);
            snprintf(kv[nkv].sv, sizeof kv[nkv].sv, "%s", eq + 1);
            kv[nkv].v = strtod(eq + 1, nullptr);
            nkv++;
        }
        int ci = findCalc(title);
        if (ci < 0) { printf("ERR\tcalculateur inconnu\t%s\nEND\n", title); continue; }
        const CalcDef &d = calcDef(ci);
        calcReset(ci);
        CalcState &s = g_states[ci];
        RowList rl;
        for (int k = 0; k < nkv; k++) {
            if (!strcmp(kv[k].k, "@n")) s.n = (uint8_t)kv[k].v;
            if (kv[k].k[0] == '$') snprintf(s.t[kv[k].k[1] - '0'], TXT_LEN, "%s", kv[k].sv);
        }
        for (int pass = 0; pass < 3; pass++) {
            rl.n = 0;
            d.rows(s, rl);
            for (int k = 0; k < nkv; k++) {
                const char *key = kv[k].k[0] == '?' ? kv[k].k + 1 : kv[k].k;
                if (kv[k].k[0] != '?' && kv[k].k[0] != '#') continue;
                for (int r = 0; r < rl.n; r++)
                    if (rl.r[r].kind == RK_CHOICE && rl.r[r].label && !strcmp(rl.r[r].label, key))
                        s.c[rl.r[r].slot] = (uint8_t)kv[k].v;
            }
            d.compute(s);
        }
        int unknown = 0;
        rl.n = 0;
        d.rows(s, rl);
        for (int k = 0; k < nkv; k++) {
            if (kv[k].k[0] == '@' || kv[k].k[0] == '$') continue;
            bool forceChoice = kv[k].k[0] == '?';
            const char *key = forceChoice ? kv[k].k + 1 : kv[k].k;
            bool found = false;
            for (int r = 0; r < rl.n && !found; r++) {
                const Row &w = rl.r[r];
                if (!w.label || strcmp(w.label, key)) continue;
                if (!forceChoice && w.kind == RK_IN) { s.v[w.slot] = kv[k].v; found = true; }
                else if (w.kind == RK_CHOICE && (forceChoice || key[0] == '#')) found = true;
            }
            if (!found) { printf("ERR\tetiquette inconnue\t%s\n", kv[k].k); unknown++; }
        }
        d.compute(s);
        rl.n = 0;
        d.rows(s, rl);
        for (int r = 0; r < rl.n; r++) {
            const Row &w = rl.r[r];
            switch (w.kind) {
            case RK_IN: printf("IN\t%s\t%.17g\n", w.label, s.v[w.slot]); break;
            case RK_OUT:
                printf("OUT\t%s\t%.17g\n", w.label, s.v[w.slot]);
                if (w.flags & RF_CPLX) printf("OUT\t%s.im\t%.17g\n", w.label, s.v[w.slot + 1]);
                break;
            case RK_CHOICE: printf("CHOICE\t%s\t%d\n", w.label, s.c[w.slot]); break;
            case RK_MSG: printf("MSG\t%d\t%s\n", w.slot, s.msg[w.slot] ? s.msg[w.slot] : ""); break;
            default: break;
            }
        }
        printf("END\n");
    }
    return 0;
}
