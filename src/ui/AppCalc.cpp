// Vue calculateur : schema a gauche, champs a droite, ligne d'aide en bas.
#include "App.hpp"
#include "../doc/Content.hpp"
#include "../font/Font.hpp"
#include "../gfx/Color.hpp"
#include "../gfx/Tex.hpp"
#include "../sch/Sch.hpp"
#include <string.h>

int itoa10(int v, char *out);
void drawScrollbar(Canvas &cv, int x, int y, int h, int total, int offset);

static inline int imax(int a, int b) { return a > b ? a : b; }
static inline int imin(int a, int b) { return a < b ? a : b; }

static const uint16_t VOLT = rgb(40, 100, 185);
static const uint16_t STATUS_BG = rgb(226, 230, 238);
enum { VALUE_RIGHT = SCREEN_W - 9, PANEL_TOP = CONTENT_TOP + 2 };

static bool selectable(const Row &r) { return r.kind != RK_HEAD; }

// Texte du choix numero k dans "A|B|C".
static int choiceText(const char *choices, int k, char *out, int cap)
{
    const char *p = choices;
    for (int i = 0; i < k && *p; i++) {
        while (*p && *p != '|') p++;
        if (*p == '|') p++;
    }
    int n = 0;
    while (*p && *p != '|' && n < cap - 1) out[n++] = *p++;
    out[n] = 0;
    return n;
}

static int choiceCount(const char *choices)
{
    int n = 1;
    for (const char *p = choices; *p; p++) if (*p == '|') n++;
    return n;
}

// --------------------------------------------------------------------------
// Ouverture, lignes
// --------------------------------------------------------------------------
void App::openCalc(int c)
{
    if (c < 0 || c >= calcCount()) return;
    const CalcDef &d = calcDef(c);
    if (d.section != section || listCount == 0) {
        int idx, cnt;
        idx = calcIndexInSection(c, cnt);
        openSection(d.section, idx);
    }
    for (int i = 0; i < listCount; i++) if (listCalcs[i] == c) listSel = i;
    calc = c;
    calcState(c);
    editing = false;
    status = nullptr;
    resetArmed = false;
    gValid = false;
    rowSel = 0;
    rowScroll = 0;
    rebuildRows(-1, -1);
    view = V_CALC;
}

void App::rebuildRows(int keepKind, int keepSlot)
{
    const CalcDef &d = calcDef(calc);
    CalcState &s = g_states[calc];
    rows.n = 0;
    d.rows(s, rows);
    if (keepKind >= 0) {
        for (int i = 0; i < rows.n; i++)
            if (rows.r[i].kind == keepKind && rows.r[i].slot == keepSlot) { rowSel = i; break; }
    }
    if (rowSel >= rows.n) rowSel = rows.n - 1;
    if (rowSel < 0) rowSel = 0;
    // ligne selectionnable la plus proche
    if (rows.n && !selectable(rows.r[rowSel])) {
        int i = rowSel;
        while (i < rows.n && !selectable(rows.r[i])) i++;
        if (i >= rows.n) { i = rowSel; while (i > 0 && !selectable(rows.r[i])) i--; }
        rowSel = i;
    }
    revealRow();
}

void App::recompute()
{
    const CalcDef &d = calcDef(calc);
    CalcState &s = g_states[calc];
    if (d.compute) d.compute(s);
    int kind = rows.n ? rows.r[rowSel].kind : -1;
    int slot = rows.n ? rows.r[rowSel].slot : -1;
    rebuildRows(kind, slot);
    gValid = false;
}

int App::rowHeight(int i) const
{
    return rows.r[i].kind == RK_HEAD ? HEAD_H : ROW_H;
}

int App::rowY(int i) const
{
    int y = 0;
    for (int k = 0; k < i && k < rows.n; k++) y += rowHeight(k);
    return y;
}

void App::revealRow()
{
    const int H = PANEL_BOTTOM - PANEL_TOP - 1;
    if (!rows.n) { rowScroll = 0; return; }
    int y = rowY(rowSel), h = rowHeight(rowSel);
    // garder le titre de section juste au-dessus
    int yTop = y;
    if (rowSel > 0 && rows.r[rowSel - 1].kind == RK_HEAD) yTop -= HEAD_H;
    if (yTop < rowScroll) rowScroll = yTop;
    if (y + h > rowScroll + H) rowScroll = y + h - H;
    int total = rowY(rows.n);
    // derniere ligne selectionnable : tout montrer jusqu'en bas
    int last = rows.n - 1;
    while (last > 0 && !selectable(rows.r[last])) last--;
    if (rowSel == last) rowScroll = imax(rowScroll, total - H);
    if (rowScroll > imax(0, total - H)) rowScroll = imax(0, total - H);
    if (rowScroll < 0) rowScroll = 0;
}

void App::moveRow(int dir)
{
    int i = rowSel + dir;
    while (i >= 0 && i < rows.n && !selectable(rows.r[i])) i += dir;
    if (i < 0 || i >= rows.n) {
        // en haut : montrer les premieres lignes
        if (dir < 0) rowScroll = 0;
        return;
    }
    rowSel = i;
    revealRow();
}

// --------------------------------------------------------------------------
// Dessin
// --------------------------------------------------------------------------
static const char *const s_pfxKeys[6] = {"p", "n", "\xC2\xB5", "m", "k", "M"};
static const char *const s_pfxKeysShift[6] = {"f", "", "", "", "G", "T"};
static const int s_pfxIdx[6] = {PFX_P, PFX_N, PFX_U, PFX_M, PFX_K, PFX_MEGA};
static const int s_pfxIdxShift[6] = {PFX_F, -1, -1, -1, PFX_G, PFX_T};

const char *App::rowHelp(const Row &r) const
{
    if (editing && r.kind == RK_TEXT)
        return "EXE : valider  \xC2\xB7  DEL : effacer  \xC2\xB7  AC : tout effacer  \xC2\xB7  EXIT : annuler";
    if (editing) {
        if (unitHasPrefix(r.unit))
            return "EXE : valider  \xC2\xB7  F1\xE2\x80\xA6" "F6 : pr\xC3\xA9" "fixe (SHIFT : f, G, T)  \xC2\xB7  EXIT : annuler";
        return "EXE : valider  \xC2\xB7  \xC3\x97" "10^x et (\xE2\x88\x92) possibles  \xC2\xB7  EXIT : annuler";
    }
    if (r.desc) return r.desc;
    switch (r.kind) {
    case RK_IN:
        switch (r.step) {
        case ST_E24: return "Tapez une valeur  \xC2\xB7  \xE2\x86\x90 \xE2\x86\x92 : s\xC3\xA9rie E24";
        case ST_E12: return "Tapez une valeur  \xC2\xB7  \xE2\x86\x90 \xE2\x86\x92 : s\xC3\xA9rie E12";
        case ST_E6: return "Tapez une valeur  \xC2\xB7  \xE2\x86\x90 \xE2\x86\x92 : s\xC3\xA9rie E6";
        case ST_125: return "Tapez une valeur  \xC2\xB7  \xE2\x86\x90 \xE2\x86\x92 : 1, 2, 5, 10\xE2\x80\xA6";
        case ST_NONE: return "Tapez une valeur (EXE pour modifier)";
        default: return "Tapez une valeur  \xC2\xB7  \xE2\x86\x90 \xE2\x86\x92 : moins / plus";
        }
    case RK_CHOICE:
        if (calcDef(calc).flags & CF_DIGITCHOICE) return "0, 1 ou X,\xCE\xB8,T : valeur et ligne suivante  \xC2\xB7  \xE2\x86\x90 \xE2\x86\x92 : changer";
        return "\xE2\x86\x90 \xE2\x86\x92 ou EXE : changer";
    case RK_TEXT: return "EXE : modifier l'expression";
    case RK_OUT: return "R\xC3\xA9sultat calcul\xC3\xA9";
    case RK_MSG: {
        if (r.flags & RF_TEX) return "R\xC3\xA9sultat (F4 : vue compl\xC3\xA8te)";
        const char *m = g_states[calc].msg[r.slot];
        return m ? m : "";
    }
    default: return "";
    }
}

void App::drawCalc(Canvas &cv)
{
    const CalcDef &d = calcDef(calc);
    const Section &sec = g_sections[d.section];
    CalcState &s = g_states[calc];
    uint16_t accent = sec.color;
    cv.fillRect(0, CONTENT_TOP, SCREEN_W, CONTENT_H, col::pageBg);
    // schema
    cv.fillRect(SCH_X, CONTENT_TOP, SCH_W, PANEL_BOTTOM - CONTENT_TOP, col::white);
    cv.fillRect(SCH_X + SCH_W, CONTENT_TOP, 1, PANEL_BOTTOM - CONTENT_TOP, col::line);
    if (d.schema) {
        Sch g;
        g.cv = &cv;
        g.ox = SCH_X;
        g.oy = CONTENT_TOP;
        g.ink = col::ink;
        g.accent = accent;
        g.light = mix16(col::white, accent, 3);
        g.paper = col::white;
        g.label = col::inkSoft;
        g.volt = VOLT;
        g.hilite = rows.n ? rows.r[rowSel].tag : 0;
        cv.setClip(SCH_X, CONTENT_TOP, SCH_X + SCH_W, PANEL_BOTTOM);
        d.schema(g, s);
        cv.resetClip();
    }
    drawFields(cv, accent);
    // ligne d'aide
    cv.fillRect(0, STATUS_Y, SCREEN_W, CONTENT_BOTTOM - STATUS_Y, STATUS_BG);
    cv.fillRect(0, STATUS_Y, SCREEN_W, 1, col::line);
    const char *help = status ? status : (rows.n ? rowHelp(rows.r[rowSel]) : "");
    font::drawRichFit(cv, statusBad && status ? BOLD_12 : SANS_12, 5, STATUS_Y + 12, help,
                      statusBad && status ? col::red : col::inkSoft, SCREEN_W - 10);
    // en-tete
    char right[16];
    int k = itoa10(listSel + 1, right);
    right[k++] = '/';
    itoa10(listCount, right + k);
    drawHeader(cv, accent, sec.label, d.title, right);
    // touches
    if (editing && rows.r[rowSel].kind == RK_TEXT) {
        const TextKeys *tk = d.tkeys;
        static const char *const none[6] = {"", "", "", "", "", ""};
        drawFKeys(cv, tk ? (shift ? tk->labS : tk->lab) : none);
        return;
    }
    if (editing) {
        const Row &r = rows.r[rowSel];
        static const char *const none[6] = {"", "", "", "", "", ""};
        if (unitHasPrefix(r.unit)) drawFKeys(cv, shift ? s_pfxKeysShift : s_pfxKeys);
        else drawFKeys(cv, none);
        return;
    }
    const char *fk[6] = {"Formules", "\xE2\x97\x80 Calc.", "Calc. \xE2\x96\xB6", "", "", "Liste"};
    if (d.flags & CF_LIST) {
        fk[3] = "Ajouter";
        fk[4] = "Retirer";
    } else {
        if (d.graph != G_NONE) fk[3] = d.graphLabel ? d.graphLabel : (d.graph == G_BODE ? "Bode" : "Graphe");
        fk[4] = resetArmed ? "Confirmer" : "D\xC3\xA9" "faut";
    }
    drawFKeys(cv, fk);
}

void App::drawFields(Canvas &cv, uint16_t accent)
{
    CalcState &s = g_states[calc];
    const int x0 = FIELDS_X + 1;
    const int H = PANEL_BOTTOM - PANEL_TOP - 1;
    uint16_t outInk = mix16(accent, col::ink, 6);
    cv.setClip(x0, CONTENT_TOP, SCREEN_W, PANEL_BOTTOM);
    int y = PANEL_TOP - rowScroll;
    char buf[64];
    for (int i = 0; i < rows.n; i++) {
        const Row &r = rows.r[i];
        int h = rowHeight(i);
        if (y + h < CONTENT_TOP || y > PANEL_BOTTOM) { y += h; continue; }
        bool sel = i == rowSel;
        if (r.kind == RK_HEAD) {
            int w = font::drawText(cv, BOLD_12, x0 + 5, y + 11, r.text, accent);
            cv.fillRect(w + 5, y + 7, VALUE_RIGHT - w - 3, 1, mix16(col::line, accent, 5));
            y += h;
            continue;
        }
        if (sel) {
            cv.roundRect(x0 + 1, y, SCREEN_W - x0 - 8, h - 1, 3, mix16(col::white, accent, 3), true, accent,
                         true);
        } else if (r.kind == RK_OUT || r.kind == RK_MSG) {
            cv.fillRect(x0 + 1, y, SCREEN_W - x0 - 8, h - 1, mix16(col::white, accent, 1));
        }
        if (r.kind == RK_MSG && (r.flags & RF_TEX)) {
            int lw = r.label ? tex::draw(cv, r.label, x0 + 6, y + 13, outInk, tex::LEFT, 2) : 0;
            int vx = x0 + 6 + lw + 6;
            const char *m = s.msg[r.slot];
            if (m) {
                int w = tex::width(m, 2);
                cv.setClip(vx, CONTENT_TOP, VALUE_RIGHT + 1, PANEL_BOTTOM);
                if (VALUE_RIGHT - w >= vx) tex::draw(cv, m, VALUE_RIGHT, y + 13, col::ink, tex::RIGHT, 2);
                else tex::draw(cv, m, vx, y + 13, col::ink, tex::LEFT, 2);
                cv.setClip(x0, CONTENT_TOP, SCREEN_W, PANEL_BOTTOM);
                if (VALUE_RIGHT - w < vx) cv.fillRect(VALUE_RIGHT - 1, y + 2, 2, h - 5, accent);
            }
            y += h;
            continue;
        }
        if (r.kind == RK_TEXT) {
            const CalcDef &cd = calcDef(calc);
            int lw = tex::draw(cv, r.label, x0 + 6, y + 13, col::ink, tex::LEFT, 2);
            int vx = x0 + 6 + lw + 6;
            char tb[160];
            const char *src = (sel && editing) ? tbuf : s.t[r.slot];
            if (cd.toTex) cd.toTex(src, tb, sizeof tb);
            else scopy(tb, sizeof tb, src);
            if (sel && editing) {
                cv.roundRect(vx - 2, y + 1, VALUE_RIGHT - vx + 5, h - 3, 2, col::white, true, col::blue, true);
            }
            int w = tb[0] ? tex::width(tb, 2) : 0;
            int right = (sel && editing) ? VALUE_RIGHT - 3 : VALUE_RIGHT;
            cv.setClip(vx, CONTENT_TOP, VALUE_RIGHT + 1, PANEL_BOTTOM);
            int tx = right - w >= vx ? right - w : (sel && editing ? right - w : vx);
            if (tb[0]) tex::draw(cv, tb, tx, y + 13, col::ink, tex::LEFT, 2);
            if (sel && editing) cv.fillRect(right + 1, y + 3, 2, 12, col::blue);
            cv.setClip(x0, CONTENT_TOP, SCREEN_W, PANEL_BOTTOM);
            y += h;
            continue;
        }
        if (r.kind == RK_MSG) {
            const char *m = s.msg[r.slot];
            if (m) {
                uint16_t c = s.lvl[r.slot] == ML_BAD ? col::red
                           : s.lvl[r.slot] == ML_WARN ? col::orange
                           : s.lvl[r.slot] == ML_OK ? col::green : col::inkSoft;
                int f = s.lvl[r.slot] >= ML_WARN ? BOLD_12 : SANS_12;
                int x = x0 + 6;
                if (s.lvl[r.slot] >= ML_WARN) x = font::drawText(cv, BOLD_12, x, y + 13, "\xE2\x9A\xA0 ", c);
                else if (s.lvl[r.slot] == ML_OK) x = font::drawText(cv, BOLD_12, x, y + 13, "\xE2\x9C\x93 ", c);
                font::drawRichFit(cv, f, x, y + 13, m, c, VALUE_RIGHT - x);
            }
            y += h;
            continue;
        }
        // etiquette
        uint16_t lc = r.kind == RK_OUT ? outInk : col::ink;
        int lw = tex::draw(cv, r.label, x0 + 6, y + 13, lc, tex::LEFT, 2);
        int vx = x0 + 6 + lw + 8;   // limite gauche de la valeur
        // valeur
        if (r.kind == RK_CHOICE) {
            choiceText(r.text, s.c[r.slot], buf, sizeof buf);
            int f = SANS_14;
            int w = font::richWidth(f, buf);
            int right = sel ? VALUE_RIGHT - 9 : VALUE_RIGHT;
            if (right - w < vx) font::drawRichFit(cv, f, vx, y + 13, buf, col::ink, right - vx);
            else font::drawRich(cv, f, right - w, y + 13, buf, col::ink);
            if (sel) {
                font::drawText(cv, SANS_12, imax(vx - 9, right - w - 10), y + 13, "\xE2\x97\x82", accent);
                font::drawText(cv, SANS_12, VALUE_RIGHT - 7, y + 13, "\xE2\x96\xB8", accent);
            }
        } else if (sel && editing) {
            // zone de saisie
            int bx = imax(vx - 2, VALUE_RIGHT - 118);
            cv.roundRect(bx, y + 1, VALUE_RIGHT - bx + 3, h - 3, 2, col::white, true, col::blue, true);
            char shown[64];
            fmtInput(shown, sizeof shown, ebuf);
            const char *unit = unitSymbol(r.unit);
            int uw = unit[0] ? font::width(SANS_12, unit) + 3 : 0;
            int tw = font::width(BOLD_14, shown);
            int tx = VALUE_RIGHT - uw - tw - 3;
            if (!elen) {
                fmtValue(buf, sizeof buf, s.v[r.slot], r.unit);
                font::drawTextFit(cv, SANS_12, bx + 4, y + 13, buf, col::line, VALUE_RIGHT - bx - 8);
                tx = VALUE_RIGHT - uw - 3;
            } else {
                if (tx < bx + 3) tx = bx + 3;
                font::drawText(cv, BOLD_14, tx, y + 13, shown, col::ink);
            }
            cv.fillRect(tx + tw + 1, y + 3, 2, 12, col::blue);
            if (uw) font::drawText(cv, SANS_12, VALUE_RIGHT - uw + 3, y + 13, unit, col::note);
        } else {
            if (r.kind == RK_OUT && (r.flags & RF_CPLX))
                fmtComplex(buf, sizeof buf, s.v[r.slot], s.v[r.slot + 1], r.unit, r.flags & RF_POLAR);
            else if (r.kind == RK_OUT && (r.flags & RF_INT)) fmtInt(buf, sizeof buf, s.v[r.slot]);
            else fmtValue(buf, sizeof buf, s.v[r.slot], r.unit);
            bool main = r.kind == RK_OUT && (r.flags & RF_MAIN);
            int f = main ? BOLD_14 : SANS_14;
            uint16_t c = r.kind == RK_OUT ? (main ? outInk : col::ink) : col::ink;
            int w = font::width(f, buf);
            if (VALUE_RIGHT - w < vx) {   // valeur longue (complexe) : police plus petite
                f = main ? BOLD_12 : SANS_12;
                w = font::width(f, buf);
            }
            if (VALUE_RIGHT - w < vx) font::drawTextFit(cv, f, vx, y + 13, buf, c, VALUE_RIGHT - vx);
            else font::drawText(cv, f, VALUE_RIGHT - w, y + 13, buf, c);
        }
        y += h;
    }
    drawScrollbar(cv, SCREEN_W - 5, PANEL_TOP, H, rowY(rows.n), rowScroll);
    cv.resetClip();
}

// --------------------------------------------------------------------------
// Saisie et modification des valeurs
// --------------------------------------------------------------------------
void App::stepValue(int dir)
{
    const Row &r = rows.r[rowSel];
    CalcState &s = g_states[calc];
    double v = s.v[r.slot], nv = v;
    switch (r.step) {
    case ST_E24: nv = eStep(E24, v, dir); break;
    case ST_E12: nv = eStep(E12, v, dir); break;
    case ST_E6: nv = eStep(E6, v, dir); break;
    case ST_125: nv = step125(v, dir); break;
    case ST_LIN1: nv = fm::floor(v + 0.5) + dir; if (fm::fabs(v - fm::floor(v + 0.5)) > 1e-9) nv = dir > 0 ? fm::ceil(v) : fm::floor(v); break;
    case ST_LIN01: nv = fm::floor(v * 10 + 0.5) / 10 + 0.1 * dir; break;
    case ST_LIN10: nv = fm::floor(v / 10 + 0.5) * 10 + 10 * dir; break;
    case ST_LIN5: nv = fm::floor(v / 5 + 0.5) * 5 + 5 * dir; break;
    case ST_INT: nv = fm::floor(v + 0.5) + dir; break;
    case ST_E96: nv = eStep(E96, v, dir); break;
    case ST_POW2: {
        double p = 1;
        if (v >= 1) {
            while (p * 2 <= v * (1 + 1e-12)) p *= 2;
            nv = dir > 0 ? p * 2 : (p < v * (1 - 1e-12) ? p : p / 2);
        } else nv = dir > 0 ? 1 : 0.5;
        if (nv < 1) return;
        break;
    }
    default: return;
    }
    if (fm::fabs(nv) < 1e-12) nv = 0;
    if ((r.flags & RF_POS) && !(nv > 0)) return;
    if ((r.flags & RF_NONNEG) && nv < 0) return;
    s.v[r.slot] = nv;
    recompute();
}

void App::startEdit(char first)
{
    editing = true;
    elen = 0;
    ebuf[0] = 0;
    status = nullptr;
    if (first) {
        if (first == 'E') { ebuf[elen++] = '1'; }
        ebuf[elen++] = first;
        ebuf[elen] = 0;
    }
}

bool App::commitEdit(int prefix)
{
    if (!elen) { editing = false; return true; }
    double v;
    if (!parseNumber(ebuf, v)) {
        status = "Nombre invalide";
        statusBad = true;
        return false;
    }
    if (prefix >= 0) v *= fm::p10(prefixExp(prefix));
    const Row &r = rows.r[rowSel];
    if (r.flags & RF_INT) v = fm::floor(v + 0.5);
    if (!fm::isfinite(v)) { status = "Valeur hors limites"; statusBad = true; return false; }
    if ((r.flags & RF_POS) && !(v > 0)) {
        status = "Valeur strictement positive attendue";
        statusBad = true;
        return false;
    }
    if ((r.flags & RF_NONNEG) && v < 0) {
        status = "Valeur positive ou nulle attendue";
        statusBad = true;
        return false;
    }
    g_states[calc].v[r.slot] = v;
    editing = false;
    status = nullptr;
    recompute();
    return true;
}

bool App::keyEditText(int k, char ch)
{
    const Row &r = rows.r[rowSel];
    const CalcDef &d = calcDef(calc);
    const TextKeys *tk = d.tkeys;
    status = nullptr;
    auto put = [&](const char *t) {
        while (t && *t && tlen < TXT_LEN - 1) { tbuf[tlen++] = *t++; tbuf[tlen] = 0; }
    };
    auto putc = [&](char c) {
        if (c && tlen < TXT_LEN - 1) { tbuf[tlen++] = c; tbuf[tlen] = 0; }
    };
    switch (k) {
    case AK_F1: case AK_F2: case AK_F3: case AK_F4: case AK_F5: case AK_F6:
        if (tk) put(shift ? tk->insS[k - AK_F1] : tk->ins[k - AK_F1]);
        break;
    case AK_ADD: if (tk) putc(tk->add); break;
    case AK_MUL: if (tk) putc(tk->mul); break;
    case AK_LPAR: if (tk) putc(tk->lpar); break;
    case AK_RPAR: if (tk) putc(tk->rpar); break;
    case AK_DIGIT:
        if (tk && tk->digits) for (const char *p = tk->digits; *p; p++) if (*p == ch) putc(ch);
        break;
    case AK_DEL: if (tlen) tbuf[--tlen] = 0; break;
    case AK_AC: tlen = 0; tbuf[0] = 0; break;
    case AK_EXE:
    case AK_UP:
    case AK_DOWN: {
        const char *err = d.textCheck ? d.textCheck(tbuf) : nullptr;
        if (err) { status = err; statusBad = true; break; }
        scopy(g_states[calc].t[r.slot], TXT_LEN, tbuf);
        editing = false;
        recompute();
        if (k != AK_EXE) moveRow(k == AK_DOWN ? 1 : -1);
        break;
    }
    case AK_EXIT: editing = false; break;
    default: break;
    }
    return true;
}

bool App::keyEdit(int k, char ch)
{
    if (rows.r[rowSel].kind == RK_TEXT) return keyEditText(k, ch);
    const Row &r = rows.r[rowSel];
    bool hasE = false, hasDot = false;
    for (int i = 0; i < elen; i++) {
        if (ebuf[i] == 'E') hasE = true;
        if (ebuf[i] == '.' && !hasE) hasDot = true;
    }
    status = nullptr;
    auto put = [&](char c) {
        if (elen < (int)sizeof(ebuf) - 2) { ebuf[elen++] = c; ebuf[elen] = 0; }
    };
    switch (k) {
    case AK_DIGIT: put(ch); break;
    case AK_DOT: if (!hasDot && !hasE) put('.'); break;
    case AK_EXP:
        if (!hasE) { if (!elen) put('1'); put('E'); }
        break;
    case AK_NEG:
    case AK_SUB:
        if (!elen || ebuf[elen - 1] == 'E') put('-');
        else if (ebuf[0] == '-') { memmove(ebuf, ebuf + 1, elen); elen--; }
        else if (elen < (int)sizeof(ebuf) - 2) { memmove(ebuf + 1, ebuf, elen + 1); ebuf[0] = '-'; elen++; }
        break;
    case AK_DEL: if (elen) ebuf[--elen] = 0; break;
    case AK_AC: elen = 0; ebuf[0] = 0; break;
    case AK_EXE: commitEdit(PFX_1); break;
    case AK_F1: case AK_F2: case AK_F3: case AK_F4: case AK_F5: case AK_F6: {
        if (!unitHasPrefix(r.unit)) break;
        int i = k - AK_F1;
        int p = shift ? s_pfxIdxShift[i] : s_pfxIdx[i];
        if (p >= 0) commitEdit(p);
        break;
    }
    case AK_UP:
    case AK_DOWN:
        if (!elen) editing = false;
        else if (!commitEdit(PFX_1)) break;
        moveRow(k == AK_DOWN ? 1 : -1);
        break;
    case AK_EXIT: editing = false; break;
    default: break;
    }
    return true;
}

// --------------------------------------------------------------------------
// Listes (associations)
// --------------------------------------------------------------------------
void App::listAdd()
{
    CalcState &s = g_states[calc];
    if (s.n >= MAX_LIST) { status = "12 composants au maximum"; statusBad = true; return; }
    const Row &r = rows.r[rowSel];
    int at = (r.kind == RK_IN && r.slot < s.n) ? r.slot + 1 : s.n;
    double copyV = s.v[at > 0 ? at - 1 : 0];
    for (int i = s.n; i > at; i--) s.v[i] = s.v[i - 1];
    s.v[at] = copyV;
    s.n++;
    recompute();
    for (int i = 0; i < rows.n; i++)
        if (rows.r[i].kind == RK_IN && rows.r[i].slot == at) { rowSel = i; break; }
    revealRow();
}

void App::listRemove()
{
    CalcState &s = g_states[calc];
    const Row &r = rows.r[rowSel];
    if (!(r.kind == RK_IN && r.slot < s.n)) { status = "Choisissez d'abord un composant"; statusBad = true; return; }
    if (s.n <= 1) { status = "Il faut au moins un composant"; statusBad = true; return; }
    int at = r.slot;
    for (int i = at; i + 1 < s.n; i++) s.v[i] = s.v[i + 1];
    s.n--;
    recompute();
    int want = at < s.n ? at : s.n - 1;
    for (int i = 0; i < rows.n; i++)
        if (rows.r[i].kind == RK_IN && rows.r[i].slot == want) { rowSel = i; break; }
    revealRow();
}

// --------------------------------------------------------------------------
// Clavier (hors saisie)
// --------------------------------------------------------------------------
bool App::keyCalc(int k, char ch)
{
    const CalcDef &d = calcDef(calc);
    CalcState &s = g_states[calc];
    if (!rows.n) {
        if (k == AK_EXIT || k == AK_F6) view = V_LIST;
        return true;
    }
    const Row &r = rows.r[rowSel];
    bool wasArmed = resetArmed;
    resetArmed = false;
    status = nullptr;
    switch (k) {
    case AK_UP: moveRow(-1); break;
    case AK_DOWN: moveRow(1); break;
    case AK_LEFT:
    case AK_RIGHT: {
        int dir = k == AK_RIGHT ? 1 : -1;
        if (r.kind == RK_CHOICE) {
            int n = choiceCount(r.text);
            s.c[r.slot] = (uint8_t)((s.c[r.slot] + n + dir) % n);
            recompute();
        } else if (r.kind == RK_IN) {
            stepValue(dir);
        }
        break;
    }
    case AK_EXE:
        if (r.kind == RK_CHOICE) {
            int n = choiceCount(r.text);
            s.c[r.slot] = (uint8_t)((s.c[r.slot] + 1) % n);
            recompute();
        } else if (r.kind == RK_IN) {
            startEdit(0);
        } else if (r.kind == RK_TEXT) {
            editing = true;
            tlen = scopy(tbuf, TXT_LEN, s.t[r.slot]);
        }
        break;
    case AK_XOT:
        if (r.kind == RK_CHOICE && (d.flags & CF_DIGITCHOICE) && choiceCount(r.text) > 2) {
            s.c[r.slot] = 2;
            recompute();
            moveRow(1);
        }
        break;
    case AK_DIGIT:
        if (r.kind == RK_IN) startEdit(ch);
        else if (r.kind == RK_CHOICE && (d.flags & CF_DIGITCHOICE)) {
            int v = ch - '0';
            if (v >= 0 && v < choiceCount(r.text)) {
                s.c[r.slot] = (uint8_t)v;
                recompute();
                moveRow(1);
            }
        }
        break;
    case AK_DOT: if (r.kind == RK_IN) startEdit('.'); break;
    case AK_EXP: if (r.kind == RK_IN) startEdit('E'); break;
    case AK_NEG:
    case AK_SUB: if (r.kind == RK_IN) startEdit('-'); break;
    case AK_DEL: if (d.flags & CF_LIST) listRemove(); break;
    case AK_ADD: case AK_MUL: case AK_LPAR: case AK_RPAR:
        if (r.kind == RK_TEXT) {
            editing = true;
            tlen = scopy(tbuf, TXT_LEN, s.t[r.slot]);
            keyEditText(k, ch);
        }
        break;
    case AK_F1: openFormulas(d.section, calcPage(calc), V_CALC); break;
    case AK_F2: if (listSel > 0) openCalc(listCalcs[listSel - 1]); break;
    case AK_F3: if (listSel + 1 < listCount) openCalc(listCalcs[listSel + 1]); break;
    case AK_F4:
        if (d.flags & CF_LIST) listAdd();
        else if (d.graph != G_NONE) openGraph();
        break;
    case AK_F5:
        if (d.flags & CF_LIST) listRemove();
        else if (wasArmed) {
            calcReset(calc);
            rowSel = 0;
            rowScroll = 0;
            rebuildRows(-1, -1);
            status = "Valeurs par d\xC3\xA9" "faut r\xC3\xA9tablies";
            statusBad = false;
        } else {
            resetArmed = true;
            status = "F5 encore : remettre les valeurs par d\xC3\xA9" "faut";
            statusBad = true;
        }
        break;
    case AK_EXIT:
    case AK_F6:
        view = V_LIST;
        break;
    default: break;
    }
    return true;
}
