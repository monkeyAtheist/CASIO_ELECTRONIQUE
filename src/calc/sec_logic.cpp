// Section : logique combinatoire (table de verite d'une expression,
// tableau de Karnaugh, portes logiques et equivalences, bases et codes).
//
// Expressions : variables A a D, constantes 0 et 1, ! (NON, prefixe, dessine
// en barre), . ou juxtaposition (ET), + (OU), ^ (OU exclusif), @ (NON-ET),
// % (NON-OU). Priorites : NON > ET > OU-X > OU > NON-ET, NON-OU.
// Une fonction de 4 variables au plus est un masque de 16 bits (mintermes,
// A = poids fort de l'indice). Simplification exacte (minimum de termes puis
// de litteraux) : impliquants premiers puis recouvrement minimal.
#include "CalcUtil.hpp"

using namespace cu;

namespace lg {

// Masques des variables pour 4 variables (indice m = 8A + 4B + 2C + D).
static const uint16_t VMASK[4] = {0xFF00, 0xF0F0, 0xCCCC, 0xAAAA};

// --------------------------------------------------------------------------
// Analyse et evaluation
// --------------------------------------------------------------------------
enum { PE_OK, PE_OPERAND, PE_PAREN, PE_CHAR, PE_EMPTY };
struct Parser {
    const char *p;
    int err;
    uint8_t used;
    int depth;
};
static uint16_t pExpr(Parser &P);
static bool startsUnary(char c) { return (c >= 'A' && c <= 'D') || c == '0' || c == '1' || c == '(' || c == '!'; }
static uint16_t pPrimary(Parser &P)
{
    char c = *P.p;
    if (c >= 'A' && c <= 'D') { P.p++; P.used |= (uint8_t)(1 << (c - 'A')); return VMASK[c - 'A']; }
    if (c == '0') { P.p++; return 0; }
    if (c == '1') { P.p++; return 0xFFFF; }
    if (c == '(') {
        P.p++;
        if (++P.depth > 30) { P.err = PE_PAREN; return 0; }
        uint16_t v = pExpr(P);
        P.depth--;
        if (*P.p == ')') P.p++;
        else if (!P.err) P.err = PE_PAREN;
        return v;
    }
    if (!P.err) P.err = PE_OPERAND;
    return 0;
}
static uint16_t pUnary(Parser &P)
{
    if (*P.p == '!') {
        P.p++;
        if (++P.depth > 40) { P.err = PE_OPERAND; return 0; }
        uint16_t v = (uint16_t)~pUnary(P);
        P.depth--;
        return v;
    }
    return pPrimary(P);
}
static uint16_t pAnd(Parser &P)
{
    uint16_t v = pUnary(P);
    while (!P.err) {
        if (*P.p == '.') { P.p++; v &= pUnary(P); }
        else if (startsUnary(*P.p)) v &= pUnary(P);
        else break;
    }
    return v;
}
static uint16_t pXor(Parser &P)
{
    uint16_t v = pAnd(P);
    while (!P.err && *P.p == '^') { P.p++; v ^= pAnd(P); }
    return v;
}
static uint16_t pOr(Parser &P)
{
    uint16_t v = pXor(P);
    while (!P.err && *P.p == '+') { P.p++; v |= pXor(P); }
    return v;
}
static uint16_t pExpr(Parser &P)
{
    uint16_t v = pOr(P);
    while (!P.err && (*P.p == '@' || *P.p == '%')) {
        char op = *P.p++;
        uint16_t w = pOr(P);
        v = (uint16_t)(op == '@' ? ~(v & w) : ~(v | w));
    }
    return v;
}
// Table de verite (4 variables) ; renvoie le code d'erreur.
static int evalExpr(const char *s, uint16_t &tt, uint8_t &used)
{
    Parser P{s, PE_OK, 0, 0};
    tt = 0;
    used = 0;
    if (!s[0]) return PE_EMPTY;
    tt = pExpr(P);
    if (!P.err && *P.p) P.err = PE_CHAR;
    used = P.used;
    return P.err;
}
static int varCount(uint8_t used)
{
    int n = 1;
    for (int k = 0; k < 4; k++) if (used & (1 << k)) n = k + 1;
    return n < 2 ? 2 : n;
}
// Reduction a n variables : minterme i (n variables) = minterme i << (4 - n).
static uint16_t reduce(uint16_t tt, int n)
{
    uint16_t r = 0;
    for (int i = 0; i < (1 << n); i++)
        if (tt & (1u << (i << (4 - n)))) r |= (uint16_t)(1u << i);
    return r;
}
// Les tables 16 bits sont indexees par minterme : bit i = minterme i.
static uint16_t toMinterms(uint16_t masks)
{
    // VMASK est ecrit avec le bit 15 = minterme 15 : deja dans ce sens
    return masks;
}

static const char *checkExpr(const char *s)
{
    if (!s[0]) return nullptr;
    uint16_t tt;
    uint8_t used;
    switch (evalExpr(s, tt, used)) {
    case PE_OPERAND: return "Opérande manquant (variable, 0, 1 ou parenthèse)";
    case PE_PAREN: return "Parenthèse fermante manquante";
    case PE_CHAR: return "Expression incorrecte (parenthèse ou opérateur en trop)";
    default: return nullptr;
    }
}

// --------------------------------------------------------------------------
// LaTeX
// --------------------------------------------------------------------------
struct Out {
    char *p;
    int cap, n;
    void c(char ch) { if (n < cap - 1) p[n++] = ch; }
    void s(const char *t) { while (*t) c(*t++); }
    void end() { p[n < cap ? n : cap - 1] = 0; }
};
static const char *texSeq(const char *p, Out &o, bool inParen, int depth);
static const char *texFactor(const char *p, Out &o, int depth)
{
    if (depth > 40) return p + (*p ? 1 : 0);
    char c = *p;
    if (c == '!') {
        o.s("\\overline{");
        if (!p[1] || !startsUnary(p[1])) o.s("\\ \\ ");
        else p = texFactor(p + 1, o, depth + 1) - 1;
        o.s("}");
        return p + 1;
    }
    if (c == '(') {
        o.c('(');
        p = texSeq(p + 1, o, true, depth + 1);
        if (*p == ')') { o.c(')'); p++; }
        return p;
    }
    if ((c >= 'A' && c <= 'D') || c == '0' || c == '1') { o.c(c); return p + 1; }
    return p + 1;
}
static const char *texSeq(const char *p, Out &o, bool inParen, int depth)
{
    while (*p) {
        char c = *p;
        if (c == ')') {
            if (inParen) return p;
            o.c(')');
            p++;
            continue;
        }
        if (startsUnary(c)) { p = texFactor(p, o, depth); continue; }
        switch (c) {
        case '+': o.s("+"); break;
        case '.': o.s("\\cdot "); break;
        case '^': o.s("\\oplus "); break;
        case '@': o.s("\\uparrow "); break;
        case '%': o.s("\\downarrow "); break;
        default: break;
        }
        p++;
    }
    return p;
}
static int toTex(const char *src, char *out, int cap)
{
    Out o{out, cap, 0};
    texSeq(src, o, false, 0);
    o.end();
    return o.n;
}

// --------------------------------------------------------------------------
// Simplification (impliquants premiers + recouvrement minimal)
// --------------------------------------------------------------------------
struct Cube {
    uint8_t val, mask;   // mask : variables absentes du terme
};
static int popc(int x) { int k = 0; while (x) { k += x & 1; x >>= 1; } return k; }
static uint16_t coverOf(Cube c, int n)
{
    uint16_t s = 0;
    for (int m = 0; m < (1 << n); m++)
        if ((m & ~c.mask) == c.val) s |= (uint16_t)(1u << m);
    return s;
}
static int primes(int n, uint16_t on, uint16_t dc, Cube *out, uint16_t *cov)
{
    int N = 1 << n;
    uint16_t okm = on | dc;
    Cube cand[81];
    uint16_t cs[81];
    int nc = 0;
    // du plus grand au plus petit (masques avec plus de bits d'abord)
    for (int bits = n; bits >= 0; bits--)
        for (int mask = 0; mask < N; mask++) {
            if (popc(mask) != bits) continue;
            for (int val = 0; val < N; val++) {
                if (val & mask) continue;
                Cube c{(uint8_t)val, (uint8_t)mask};
                uint16_t s = coverOf(c, n);
                if ((s & ~okm) || !(s & on)) continue;
                if (nc < 81) { cand[nc] = c; cs[nc] = s; nc++; }
            }
        }
    int np = 0;
    for (int i = 0; i < nc; i++) {
        bool prime = true;
        for (int j = 0; j < nc && prime; j++)
            if (j != i && (cs[i] & ~cs[j]) == 0 && cs[j] != cs[i]) prime = false;
        if (prime) { out[np] = cand[i]; cov[np] = cs[i]; np++; }
    }
    return np;
}
struct Best {
    int nt, nl;
    Cube c[16];
};
static void search(const Cube *pr, const uint16_t *cov, int np, uint16_t need, Cube *cur, int nc, int lits, Best &b,
                   int n)
{
    if (!need) {
        if (nc < b.nt || (nc == b.nt && lits < b.nl)) {
            b.nt = nc;
            b.nl = lits;
            for (int k = 0; k < nc; k++) b.c[k] = cur[k];
        }
        return;
    }
    if (nc + 1 > b.nt || nc >= 16) return;
    int m = 0;
    while (!(need & (1u << m))) m++;
    for (int i = 0; i < np; i++) {
        if (!(cov[i] & (1u << m))) continue;
        int l = n - popc(pr[i].mask);
        if (nc + 1 == b.nt && lits + l >= b.nl) continue;
        cur[nc] = pr[i];
        search(pr, cov, np, need & (uint16_t)~cov[i], cur, nc + 1, lits + l, b, n);
    }
}
// Somme minimale de produits de la fonction (on, dc) a n variables.
static int minimize(int n, uint16_t on, uint16_t dc, Cube *res, int &lits)
{
    Cube pr[81];
    uint16_t cov[81];
    lits = 0;
    if (!on) return 0;
    int np = primes(n, on, dc, pr, cov);
    Best b;
    b.nt = 99;
    b.nl = 999;
    Cube cur[16];
    search(pr, cov, np, on, cur, 0, 0, b, n);
    if (b.nt > 16) return 0;
    for (int k = 0; k < b.nt; k++) res[k] = b.c[k];
    lits = b.nl;
    return b.nt;
}

static const char VN[4] = {'A', 'B', 'C', 'D'};
// Somme de produits en LaTeX (complement : produit de sommes par De Morgan).
static void fmtSop(Out &o, const Cube *c, int nt, int n)
{
    if (!nt) { o.c('0'); return; }
    for (int t = 0; t < nt; t++) {
        if (t) o.c('+');
        int lit = 0;
        bool prevBar = false;
        for (int k = 0; k < n; k++) {
            int bit = 1 << (n - 1 - k);
            if (c[t].mask & bit) continue;
            if (c[t].val & bit) { o.c(VN[k]); prevBar = false; }
            else {
                if (prevBar) o.s("\\,");
                o.s("\\overline{"); o.c(VN[k]); o.c('}');
                prevBar = true;
            }
            lit++;
        }
        if (!lit) o.c('1');
    }
}
static void fmtPos(Out &o, const Cube *c, int nt, int n)
{
    if (!nt) { o.c('1'); return; }
    for (int t = 0; t < nt; t++) {
        int lit = n - popc(c[t].mask);
        if (!lit) { o.n = 0; o.c('0'); return; }
        bool par = lit > 1 && nt > 1;
        if (par) o.c('(');
        int k2 = 0;
        for (int k = 0; k < n; k++) {
            int bit = 1 << (n - 1 - k);
            if (c[t].mask & bit) continue;
            if (k2++) o.c('+');
            if (c[t].val & bit) { o.s("\\overline{"); o.c(VN[k]); o.c('}'); }
            else o.c(VN[k]);
        }
        if (par) o.c(')');
    }
}
// Liste d'indices : "Σm(1,3,5)".
static void fmtList(Out &o, const char *head, uint16_t set, int n)
{
    o.s(head);
    o.c('(');
    bool first = true;
    for (int m = 0; m < (1 << n); m++) {
        if (!(set & (1u << m))) continue;
        if (!first) o.c(',');
        first = false;
        if (m >= 10) o.c((char)('0' + m / 10));
        o.c((char)('0' + m % 10));
    }
    o.c(')');
}

struct Result {
    int n;
    uint16_t on, dc;
    Cube sop[16], pos[16];
    int nSop, nPos, lSop, lPos;
};
static void solveAll(Result &r)
{
    uint16_t all = (uint16_t)((1u << (1 << r.n)) - 1);
    if (r.n == 4) all = 0xFFFF;
    r.nSop = minimize(r.n, r.on, r.dc, r.sop, r.lSop);
    uint16_t off = (uint16_t)(all & ~(r.on | r.dc));
    r.nPos = minimize(r.n, off, r.dc, r.pos, r.lPos);
}

// --------------------------------------------------------------------------
// Tableau de Karnaugh
// --------------------------------------------------------------------------
static const uint8_t GRAYV[4] = {0, 1, 3, 2};      // position -> valeur
static const uint8_t GRAYP[4] = {0, 1, 3, 2};      // valeur -> position (involution)
static const uint16_t GROUP_COL[6] = {rgb(220, 50, 50), rgb(30, 110, 220), rgb(20, 150, 70), rgb(220, 130, 0),
                                       rgb(150, 60, 190), rgb(0, 150, 160)};
struct KLayout {
    int x, y, cw, ch, n, rb, cb;   // rb, cb : bits de lignes et de colonnes
    int rows() const { return 1 << rb; }
    int cols() const { return 1 << cb; }
    int posR(int m) const { int v = m >> cb; return rb == 2 ? GRAYP[v] : v; }
    int posC(int m) const { int v = m & ((1 << cb) - 1); return cb == 2 ? GRAYP[v] : v; }
};
static KLayout klayout(int n, int x, int y, int cw, int ch)
{
    KLayout L{x, y, cw, ch, n, n / 2, n - n / 2};
    return L;
}
// Positions couvertes (masque de bits sur les positions) pour un groupe.
static int posSet(int val, int mask, int bits)
{
    int s = 0;
    for (int v = 0; v < (1 << bits); v++)
        if ((v & ~mask) == val) s |= 1 << (bits == 2 ? GRAYP[v] : v);
    return s;
}
// Segments contigus (lineaires) d'un ensemble de positions.
static int runs(int set, int count, int *a, int *b, bool *openL, bool *openR)
{
    int k = 0;
    bool full = set == (1 << count) - 1;
    for (int p = 0; p < count;) {
        if (!(set & (1 << p))) { p++; continue; }
        int q = p;
        while (q + 1 < count && (set & (1 << (q + 1)))) q++;
        a[k] = p;
        b[k] = q;
        openL[k] = !full && p == 0 && (set & (1 << (count - 1)));
        openR[k] = !full && q == count - 1 && (set & 1);
        k++;
        p = q + 1;
    }
    return k;
}
static void drawGroups(Canvas &cv, const KLayout &L, const Cube *g, int ng)
{
    cv.setClip(L.x, L.y, L.x + L.cols() * L.cw + 1, L.y + L.rows() * L.ch + 1);
    for (int i = 0; i < ng; i++) {
        uint16_t col = GROUP_COL[i % 6];
        int rmask = g[i].mask >> L.cb, rval = g[i].val >> L.cb;
        int cmask = g[i].mask & ((1 << L.cb) - 1), cval = g[i].val & ((1 << L.cb) - 1);
        int rs = posSet(rval, rmask, L.rb), cs = posSet(cval, cmask, L.cb);
        int ra[4], rbb[4], ca[4], cbb[4];
        bool rl[4], rr[4], cl[4], cr[4];
        int nr = runs(rs, L.rows(), ra, rbb, rl, rr), ncc = runs(cs, L.cols(), ca, cbb, cl, cr);
        int ins = 3 + 2 * (i % 3);
        for (int a = 0; a < nr; a++)
            for (int b = 0; b < ncc; b++) {
                int x0 = L.x + ca[b] * L.cw + ins, x1 = L.x + (cbb[b] + 1) * L.cw - ins;
                int y0 = L.y + ra[a] * L.ch + ins, y1 = L.y + (rbb[a] + 1) * L.ch - ins;
                if (cl[b]) x0 -= 3 * ins;
                if (cr[b]) x1 += 3 * ins;
                if (rl[a]) y0 -= 3 * ins;
                if (rr[a]) y1 += 3 * ins;
                cv.roundRect(x0, y0, x1 - x0, y1 - y0, 4, 0, false, col, true);
                cv.roundRect(x0 + 1, y0 + 1, x1 - x0 - 2, y1 - y0 - 2, 3, 0, false, col, true);
            }
    }
    cv.resetClip();
}
static void bitsText(char *buf, int v, int bits)
{
    for (int k = 0; k < bits; k++) buf[k] = (char)('0' + ((v >> (bits - 1 - k)) & 1));
    buf[bits] = 0;
}
// vals[m] : 0, 1 ou 2 (X).
static void drawKmap(Canvas &cv, const KLayout &L, const uint8_t *vals, const Cube *g, int ng, int sel,
                     uint16_t accent, uint16_t ink, int face)
{
    int W = L.cols() * L.cw, H = L.rows() * L.ch;
    cv.fillRect(L.x, L.y, W, H, col::white);
    for (int m = 0; m < (1 << L.n); m++) {
        int x = L.x + L.posC(m) * L.cw, y = L.y + L.posR(m) * L.ch;
        if (m == sel) cv.fillRect(x + 1, y + 1, L.cw - 1, L.ch - 1, mix16(col::white, accent, 4));
        const char *t = vals[m] == 1 ? "1" : vals[m] == 2 ? "X" : "0";
        int f = vals[m] == 1 ? (face == SANS_14 ? BOLD_14 : BOLD_16) : face;
        int w = font::width(f, t);
        uint16_t c = vals[m] == 1 ? ink : vals[m] == 2 ? col::orange : mix16(col::white, ink, 8);
        font::drawText(cv, f, x + (L.cw - w) / 2, y + L.ch / 2 + 5, t, c);
        // indice du minterme
        char idx[4];
        cu::itoaSimple(m, idx);
        font::drawText(cv, SANS_12, x + 2, y + L.ch - 2, idx, mix16(col::white, ink, 5));
    }
    for (int r = 0; r <= L.rows(); r++) cv.fillRect(L.x, L.y + r * L.ch, W + 1, 1, mix16(col::white, ink, 8));
    for (int c = 0; c <= L.cols(); c++) cv.fillRect(L.x + c * L.cw, L.y, 1, H + 1, mix16(col::white, ink, 8));
    drawGroups(cv, L, g, ng);
    // en-tetes (codes de Gray)
    char b[4];
    for (int c = 0; c < L.cols(); c++) {
        bitsText(b, L.cb == 2 ? GRAYV[c] : c, L.cb);
        int w = font::width(SANS_12, b);
        font::drawText(cv, SANS_12, L.x + c * L.cw + (L.cw - w) / 2, L.y - 3, b, ink);
    }
    for (int r = 0; r < L.rows(); r++) {
        bitsText(b, L.rb == 2 ? GRAYV[r] : r, L.rb);
        int w = font::width(SANS_12, b);
        font::drawText(cv, SANS_12, L.x - 4 - w, L.y + r * L.ch + L.ch / 2 + 4, b, ink);
    }
    // variables : lignes (A, AB) a gauche, colonnes au-dessus
    char rv[3] = {0, 0, 0}, cvn[3] = {0, 0, 0};
    for (int k = 0; k < L.rb; k++) rv[k] = VN[k];
    for (int k = 0; k < L.cb; k++) cvn[k] = VN[L.rb + k];
    int cw = font::width(BOLD_12, cvn);
    font::drawText(cv, BOLD_12, L.x + W / 2 - cw / 2, L.y - 16, cvn, accent);
    int rw = font::width(BOLD_12, rv);
    font::drawText(cv, BOLD_12, L.x - 8 - rw - 14, L.y - 6, rv, accent);
    cv.line(L.x - 22, L.y - 16, L.x - 1, L.y - 1, mix16(col::white, ink, 7));
}

// Table de verite dessinee (n variables, colonnes F et G).
static void drawTruth(Canvas &cv, int x0, int y0, int n, uint16_t f, uint16_t g, bool hasG, uint16_t accent,
                      int rowH)
{
    int N = 1 << n;
    int per = N > 8 ? 8 : N, blocks = N > 8 ? 2 : 1;
    const int cw = 12;
    int bw = (n + (hasG ? 2 : 1)) * cw + 8;
    for (int bl = 0; bl < blocks; bl++) {
        int bx = x0 + bl * (bw + 6);
        // en-tete
        for (int k = 0; k < n; k++) {
            char t[2] = {VN[k], 0};
            font::drawText(cv, BOLD_12, bx + k * cw + 2, y0 + 11, t, col::ink);
        }
        font::drawText(cv, BOLD_12, bx + n * cw + 6, y0 + 11, "F", accent);
        if (hasG) font::drawText(cv, BOLD_12, bx + (n + 1) * cw + 6, y0 + 11, "G", accent);
        cv.fillRect(bx, y0 + 14, bw - 4, 1, col::line);
        cv.fillRect(bx + n * cw + 2, y0, 1, 15 + per * rowH, col::line);
        for (int i = 0; i < per; i++) {
            int m = bl * 8 + i;
            if (m >= N) break;
            int y = y0 + 15 + i * rowH;
            int vf = (f >> m) & 1, vg = (g >> m) & 1;
            if (hasG && vf != vg) cv.fillRect(bx, y, bw - 4, rowH, rgb(255, 214, 214));
            else if (i & 1) cv.fillRect(bx, y, bw - 4, rowH, rgb(244, 246, 250));
            for (int k = 0; k < n; k++) {
                char t[2] = {(char)('0' + ((m >> (n - 1 - k)) & 1)), 0};
                font::drawText(cv, SANS_12, bx + k * cw + 3, y + rowH - 4, t, col::inkSoft);
            }
            font::drawText(cv, vf ? BOLD_12 : SANS_12, bx + n * cw + 7, y + rowH - 4, vf ? "1" : "0",
                           vf ? accent : col::inkSoft);
            if (hasG)
                font::drawText(cv, vg ? BOLD_12 : SANS_12, bx + (n + 1) * cw + 7, y + rowH - 4, vg ? "1" : "0",
                               vg ? accent : col::inkSoft);
        }
    }
}

}  // namespace lg

using namespace lg;

// Clavier de saisie des expressions.
static const TextKeys s_boolKeys = {
    {"A", "B", "C", "D", "NON", "\xE2\x8A\x95"}, {"A", "B", "C", "D", "!", "^"},
    {"0", "1", "", "", "\xE2\x86\x91", "\xE2\x86\x93"}, {"0", "1", "", "", "@", "%"},
    '+', '.', '(', ')', "01",
};

// ==========================================================================
// Table de verite d'une expression (et equivalence de deux expressions)
// ==========================================================================
namespace tv {
enum { CFORM };
static char s_sop[160], s_pos[160], s_min[64], s_max[64], s_eq[96], s_var[48];
static void def(CalcState &s)
{
    scopy(s.t[0], TXT_LEN, "!(A.B)+C");
    scopy(s.t[1], TXT_LEN, "!A+!B+C");
}
static void rows(const CalcState &, RowList &r)
{
    r.text("F", 0, 0, "F1 à F4 : A B C D  ·  F5 : NON  ·  F6 : ⊕  ·  × : ET  ·  + : OU  ·  SHIFT : 0 1 ↑ ↓");
    r.text("G", 1, 0, "Seconde expression, pour tester une équivalence (vide : aucune)");
    r.msg(0);
    r.head("Formes simplifiées de F");
    r.texOut("F=", 0 + 1, "Somme de produits minimale");
    r.texOut("F=", 2, "Produit de sommes minimal");
    r.head("Formes canoniques de F");
    r.msg(3);
    r.msg(4);
    r.msg(5);
}
static void calc(CalcState &s)
{
    uint16_t tf, tg;
    uint8_t uf, ug;
    int ef = evalExpr(s.t[0], tf, uf), eg = evalExpr(s.t[1], tg, ug);
    bool hasG = eg != PE_EMPTY;
    for (int k = 0; k < NMSG; k++) setMsg(s, k, "", ML_INFO);
    if (ef != PE_OK) {
        setMsg(s, 0, ef == PE_EMPTY ? "Saisissez une expression F (EXE)" : "Expression F incorrecte", ML_BAD);
        return;
    }
    int n = varCount(uf | (hasG && eg == PE_OK ? ug : 0));
    Result R;
    R.n = n;
    R.on = reduce(toMinterms(tf), n);
    R.dc = 0;
    solveAll(R);
    Out o{s_sop, sizeof s_sop, 0};
    fmtSop(o, R.sop, R.nSop, n);
    o.end();
    o = Out{s_pos, sizeof s_pos, 0};
    fmtPos(o, R.pos, R.nPos, n);
    o.end();
    setMsg(s, 1, s_sop, ML_INFO);
    setMsg(s, 2, s_pos, ML_INFO);
    uint16_t all = n == 4 ? 0xFFFF : (uint16_t)((1u << (1 << n)) - 1);
    o = Out{s_min, sizeof s_min, 0};
    fmtList(o, "F = \xCE\xA3m", R.on, n);
    o.end();
    o = Out{s_max, sizeof s_max, 0};
    fmtList(o, "F = \xCE\xA0M", (uint16_t)(all & ~R.on), n);
    o.end();
    setMsg(s, 3, s_min, ML_INFO);
    setMsg(s, 4, s_max, ML_INFO);
    o = Out{s_var, sizeof s_var, 0};
    o.s("Variables : ");
    for (int k = 0; k < n; k++) { if (k) o.s(", "); o.c(VN[k]); }
    o.s(n < 4 ? " (A poids fort)" : " (A poids fort)");
    o.end();
    setMsg(s, 5, s_var, ML_INFO);
    if (!hasG) setMsg(s, 0, "F4 : table de vérité et tableau de Karnaugh", ML_INFO);
    else if (eg != PE_OK) setMsg(s, 0, "Expression G incorrecte", ML_BAD);
    else {
        uint16_t g = reduce(toMinterms(tg), n);
        if (g == R.on) setMsg(s, 0, "F ≡ G : les deux expressions sont équivalentes", ML_OK);
        else {
            int m = 0;
            while (!(((g ^ R.on) >> m) & 1)) m++;
            o = Out{s_eq, sizeof s_eq, 0};
            o.s("F ≠ G, par exemple pour ");
            for (int k = 0; k < n; k++) {
                if (k) o.c(' ');
                o.c(VN[k]);
                o.c('=');
                o.c((char)('0' + ((m >> (n - 1 - k)) & 1)));
            }
            o.end();
            setMsg(s, 0, s_eq, ML_BAD);
        }
    }
}
static void sch(Sch &g, const CalcState &s)
{
    uint16_t tf, tg;
    uint8_t uf, ug;
    int ef = evalExpr(s.t[0], tf, uf), eg = evalExpr(s.t[1], tg, ug);
    if (ef != PE_OK) {
        font::drawText(*g.cv, SANS_12, g.ox + 8, g.oy + 20, "Expression F ?", col::red);
        return;
    }
    bool hasG = eg == PE_OK;
    int n = varCount(uf | (hasG ? ug : 0));
    drawTruth(*g.cv, g.ox + 6, g.oy + 4, n, reduce(tf, n), hasG ? reduce(tg, n) : 0, hasG, g.accent,
              n == 4 ? 18 : n == 3 ? 18 : 20);
}
static void graph(Canvas &cv, const CalcState &s, const GraphArea &a, uint16_t accent)
{
    uint16_t tf, tg;
    uint8_t uf, ug;
    int ef = evalExpr(s.t[0], tf, uf), eg = evalExpr(s.t[1], tg, ug);
    if (ef != PE_OK) return;
    bool hasG = eg == PE_OK;
    int n = varCount(uf | (hasG ? ug : 0));
    Result R;
    R.n = n;
    R.on = reduce(tf, n);
    R.dc = 0;
    solveAll(R);
    drawTruth(cv, a.x + 6, a.y + 4, n, R.on, hasG ? reduce(tg, n) : 0, hasG, accent, 18);
    uint8_t vals[16];
    for (int m = 0; m < 16; m++) vals[m] = (R.on >> m) & 1;
    KLayout L = klayout(n, a.x + 230, a.y + 34, 38, 26);
    drawKmap(cv, L, vals, R.sop, R.nSop, -1, accent, col::ink, SANS_14);
    char tb[200];
    toTex(s.t[0], tb, sizeof tb);
    char line[260];
    int k = scopy(line, sizeof line, "F=");
    scopy(line + k, (int)sizeof(line) - k, tb);
    tex::draw(cv, line, a.x + 182, a.y + 150, col::ink, tex::LEFT, 1);
    k = scopy(line, sizeof line, "F=");
    scopy(line + k, (int)sizeof(line) - k, s.msg[1] ? s.msg[1] : "");
    tex::draw(cv, line, a.x + 182, a.y + 172, accent, tex::LEFT, 1);
}
}  // namespace tv

// ==========================================================================
// Tableau de Karnaugh
// ==========================================================================
namespace km {
enum { CN = 0, CCELL = 1, CSHOW = 17 };
static char s_sop[200], s_pos[200], s_min[64], s_max[64], s_dc[64];
static char s_lab[16][8];
static char s_desc[3][16][32];
static bool s_init = false;
static void initText()
{
    if (s_init) return;
    s_init = true;
    for (int m = 0; m < 16; m++) {
        int k = scopy(s_lab[m], 8, "m_{");
        if (m >= 10) s_lab[m][k++] = (char)('0' + m / 10);
        s_lab[m][k++] = (char)('0' + m % 10);
        s_lab[m][k++] = '}';
        s_lab[m][k] = 0;
    }
    for (int n = 2; n <= 4; n++)
        for (int m = 0; m < (1 << n); m++) {
            char *d = s_desc[n - 2][m];
            int k = 0;
            for (int v = 0; v < n; v++) {
                if (v) d[k++] = ' ';
                d[k++] = VN[v];
                d[k++] = '=';
                d[k++] = (char)('0' + ((m >> (n - 1 - v)) & 1));
            }
            k += scopy(d + k, 32 - k, "  (0, 1, X)");
            d[k] = 0;
        }
}
static int nOf(const CalcState &s) { return s.c[CN] + 2; }
static void def(CalcState &s)
{
    s.c[CN] = 2;
    static const uint8_t init[16] = {1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 2, 0, 1, 0, 0};
    for (int m = 0; m < 16; m++) s.c[CCELL + m] = init[m];
}
static void rows(const CalcState &s, RowList &r)
{
    initText();
    int n = nOf(s);
    r.choice("#Variables", CN, "2 (A, B)|3 (A, B, C)|4 (A, B, C, D)", 0, "Nombre de variables d'entrée");
    r.choice("#Groupes", CSHOW, "des 1 (somme)|des 0 (produit)", 0, "Regroupements dessinés sur le tableau");
    r.texOut("S=", 0, "Somme de produits minimale (groupes de 1)");
    r.texOut("S=", 1, "Produit de sommes minimal (groupes de 0)");
    r.msg(2);
    r.msg(3);
    r.msg(4);
    r.head("Valeurs (0, 1 ou X indifférent)");
    for (int m = 0; m < (1 << n); m++) r.choice(s_lab[m], CCELL + m, "0|1|X", m + 1, s_desc[n - 2][m]);
}
static void result(const CalcState &s, Result &R)
{
    int n = nOf(s);
    R.n = n;
    R.on = R.dc = 0;
    for (int m = 0; m < (1 << n); m++) {
        if (s.c[CCELL + m] == 1) R.on |= (uint16_t)(1u << m);
        if (s.c[CCELL + m] == 2) R.dc |= (uint16_t)(1u << m);
    }
    solveAll(R);
}
static void calc(CalcState &s)
{
    initText();
    Result R;
    result(s, R);
    int n = R.n;
    Out o{s_sop, sizeof s_sop, 0};
    fmtSop(o, R.sop, R.nSop, n);
    o.end();
    o = Out{s_pos, sizeof s_pos, 0};
    fmtPos(o, R.pos, R.nPos, n);
    o.end();
    setMsg(s, 0, s_sop, ML_INFO);
    setMsg(s, 1, s_pos, ML_INFO);
    uint16_t all = n == 4 ? 0xFFFF : (uint16_t)((1u << (1 << n)) - 1);
    o = Out{s_min, sizeof s_min, 0};
    fmtList(o, "S = \xCE\xA3m", R.on, n);
    o.end();
    o = Out{s_max, sizeof s_max, 0};
    fmtList(o, "S = \xCE\xA0M", (uint16_t)(all & ~(R.on | R.dc)), n);
    o.end();
    o = Out{s_dc, sizeof s_dc, 0};
    if (R.dc) fmtList(o, "Indifférents : d", R.dc, n);
    else o.s("Aucune valeur indifférente");
    o.end();
    setMsg(s, 2, s_min, ML_INFO);
    setMsg(s, 3, s_max, ML_INFO);
    setMsg(s, 4, s_dc, ML_INFO);
}
static void sch(Sch &g, const CalcState &s)
{
    Result R;
    result(s, R);
    int n = R.n;
    uint8_t vals[16];
    for (int m = 0; m < 16; m++) vals[m] = m < (1 << n) ? s.c[CCELL + m] : 0;
    int cw = n == 4 ? 30 : n == 3 ? 30 : 44, ch = n == 2 ? 34 : 26;
    KLayout L = klayout(n, g.ox + (n == 4 ? 44 : n == 3 ? 44 : 60), g.oy + 34, cw, ch);
    int sel = g.hilite >= 1 && g.hilite <= 16 ? g.hilite - 1 : -1;
    bool zeros = s.c[CSHOW] == 1;
    drawKmap(*g.cv, L, vals, zeros ? R.pos : R.sop, zeros ? R.nPos : R.nSop, sel, g.accent, g.ink, SANS_14);
    // expression sous le tableau
    const char *e = s.msg[zeros ? 1 : 0];
    if (e) {
        char line[220];
        int k = scopy(line, sizeof line, "S=");
        scopy(line + k, (int)sizeof(line) - k, e);
        int w = tex::width(line, 2);
        g.cv->setClip(g.ox, g.oy, g.ox + 179, g.oy + 171);
        tex::draw(*g.cv, line, g.ox + (w < 170 ? 89 - w / 2 : 4), g.oy + 34 + L.rows() * ch + 22, g.accent, tex::LEFT, 2);
        g.cv->resetClip();
    }
}
static void graph(Canvas &cv, const CalcState &s, const GraphArea &a, uint16_t accent)
{
    Result R;
    result(s, R);
    int n = R.n;
    uint8_t vals[16];
    for (int m = 0; m < 16; m++) vals[m] = m < (1 << n) ? s.c[CCELL + m] : 0;
    bool zeros = s.c[CSHOW] == 1;
    KLayout L = klayout(n, a.x + 70, a.y + 36, n == 2 ? 70 : 54, n == 4 ? 24 : 32);
    drawKmap(cv, L, vals, zeros ? R.pos : R.sop, zeros ? R.nPos : R.nSop, -1, accent, col::ink, SANS_16);
    char line[220];
    int y = a.y + 36 + L.rows() * L.ch + 24;
    int k = scopy(line, sizeof line, "S=");
    scopy(line + k, (int)sizeof(line) - k, s_sop);
    tex::draw(cv, line, a.x + 10, y, zeros ? col::ink : accent, tex::LEFT, 1);
    k = scopy(line, sizeof line, "S=");
    scopy(line + k, (int)sizeof(line) - k, s_pos);
    tex::draw(cv, line, a.x + 10, y + 24, zeros ? accent : col::ink, tex::LEFT, 1);
}
}  // namespace km

// ==========================================================================
// Portes logiques et equivalences (realisation en NAND ou NOR seules)
// ==========================================================================
namespace gate {
enum Kind { G_NOT, G_AND, G_OR, G_NAND, G_NOR, G_XOR, G_XNOR, G_BUF };
enum { CG, CREAL, CSYM, CA, CB };
enum { SRC_A = 100, SRC_B = 101 };
struct Node {
    uint8_t kind;
    uint8_t in0, in1;   // index de porte ou SRC_A / SRC_B ; in1 = 255 : une entree
    uint8_t level, y;
};
struct Net {
    const Node *n;
    int count;
};
#define ND(k, a, b, l, y) {k, a, b, l, y}
static const Node N_NOT[] = {ND(G_NOT, SRC_A, 255, 0, 70)};
static const Node N_AND[] = {ND(G_AND, SRC_A, SRC_B, 0, 70)};
static const Node N_OR[] = {ND(G_OR, SRC_A, SRC_B, 0, 70)};
static const Node N_NAND[] = {ND(G_NAND, SRC_A, SRC_B, 0, 70)};
static const Node N_NOR[] = {ND(G_NOR, SRC_A, SRC_B, 0, 70)};
static const Node N_XOR[] = {ND(G_XOR, SRC_A, SRC_B, 0, 70)};
static const Node N_XNOR[] = {ND(G_XNOR, SRC_A, SRC_B, 0, 70)};
// NAND seules
static const Node A_NOT[] = {ND(G_NAND, SRC_A, SRC_A, 0, 70)};
static const Node A_AND[] = {ND(G_NAND, SRC_A, SRC_B, 0, 70), ND(G_NAND, 0, 0, 1, 70)};
static const Node A_OR[] = {ND(G_NAND, SRC_A, SRC_A, 0, 42), ND(G_NAND, SRC_B, SRC_B, 0, 98), ND(G_NAND, 0, 1, 1, 70)};
static const Node A_NAND[] = {ND(G_NAND, SRC_A, SRC_B, 0, 70)};
static const Node A_NOR[] = {ND(G_NAND, SRC_A, SRC_A, 0, 42), ND(G_NAND, SRC_B, SRC_B, 0, 98), ND(G_NAND, 0, 1, 1, 70),
                             ND(G_NAND, 2, 2, 2, 70)};
static const Node A_XOR[] = {ND(G_NAND, SRC_A, SRC_B, 0, 70), ND(G_NAND, SRC_A, 0, 1, 42), ND(G_NAND, SRC_B, 0, 1, 98),
                             ND(G_NAND, 1, 2, 2, 70)};
static const Node A_XNOR[] = {ND(G_NAND, SRC_A, SRC_B, 0, 70), ND(G_NAND, SRC_A, 0, 1, 42), ND(G_NAND, SRC_B, 0, 1, 98),
                              ND(G_NAND, 1, 2, 2, 70), ND(G_NAND, 3, 3, 3, 70)};
// NOR seules
static const Node O_NOT[] = {ND(G_NOR, SRC_A, SRC_A, 0, 70)};
static const Node O_AND[] = {ND(G_NOR, SRC_A, SRC_A, 0, 42), ND(G_NOR, SRC_B, SRC_B, 0, 98), ND(G_NOR, 0, 1, 1, 70)};
static const Node O_OR[] = {ND(G_NOR, SRC_A, SRC_B, 0, 70), ND(G_NOR, 0, 0, 1, 70)};
static const Node O_NAND[] = {ND(G_NOR, SRC_A, SRC_A, 0, 42), ND(G_NOR, SRC_B, SRC_B, 0, 98), ND(G_NOR, 0, 1, 1, 70),
                              ND(G_NOR, 2, 2, 2, 70)};
static const Node O_NOR[] = {ND(G_NOR, SRC_A, SRC_B, 0, 70)};
static const Node O_XNOR[] = {ND(G_NOR, SRC_A, SRC_B, 0, 70), ND(G_NOR, SRC_A, 0, 1, 42), ND(G_NOR, SRC_B, 0, 1, 98),
                              ND(G_NOR, 1, 2, 2, 70)};
static const Node O_XOR[] = {ND(G_NOR, SRC_A, SRC_B, 0, 70), ND(G_NOR, SRC_A, 0, 1, 42), ND(G_NOR, SRC_B, 0, 1, 98),
                             ND(G_NOR, 1, 2, 2, 70), ND(G_NOR, 3, 3, 3, 70)};
#undef ND
#define NET(a) {a, (int)(sizeof(a) / sizeof(a[0]))}
static const Net NETS[3][7] = {
    {NET(N_NOT), NET(N_AND), NET(N_OR), NET(N_NAND), NET(N_NOR), NET(N_XOR), NET(N_XNOR)},
    {NET(A_NOT), NET(A_AND), NET(A_OR), NET(A_NAND), NET(A_NOR), NET(A_XOR), NET(A_XNOR)},
    {NET(O_NOT), NET(O_AND), NET(O_OR), NET(O_NAND), NET(O_NOR), NET(O_XOR), NET(O_XNOR)},
};
#undef NET
static const char *const GNAMES = "NON|ET|OU|NON-ET|NON-OU|OU exclusif|NON-OU exclusif";
static const char *const EXPR[7] = {
    "S=\\overline{A}", "S=A\\cdot B", "S=A+B", "S=\\overline{A\\cdot B}", "S=\\overline{A+B}", "S=A\\oplus B",
    "S=\\overline{A\\oplus B}",
};
static const char *const EQ_NAND[7] = {
    "\\overline{A}=\\overline{A\\cdot A}", "A\\cdot B=\\overline{\\overline{A\\cdot B}}",
    "A+B=\\overline{\\overline{A}\\cdot\\overline{B}}", "\\overline{A\\cdot B}",
    "\\overline{A+B}=\\overline{\\overline{\\overline{A}\\cdot\\overline{B}}}",
    "A\\oplus B=\\overline{\\overline{A\\overline{AB}}\\cdot\\overline{B\\overline{AB}}}",
    "\\overline{A\\oplus B}",
};
static const char *const EQ_NOR[7] = {
    "\\overline{A}=\\overline{A+A}", "A\\cdot B=\\overline{\\overline{A}+\\overline{B}}",
    "A+B=\\overline{\\overline{A+B}}", "\\overline{A\\cdot B}=\\overline{\\overline{\\overline{A}+\\overline{B}}}",
    "\\overline{A+B}", "A\\oplus B", "\\overline{A\\oplus B}=\\overline{\\overline{A+\\overline{A+B}}+\\overline{B+\\overline{A+B}}}",
};
static int evalGate(int k, int a, int b)
{
    switch (k) {
    case G_NOT: return !a;
    case G_AND: return a & b;
    case G_OR: return a | b;
    case G_NAND: return !(a & b);
    case G_NOR: return !(a | b);
    case G_XOR: return a ^ b;
    case G_XNOR: return !(a ^ b);
    default: return a;
    }
}
static int netEval(const Net &N, int a, int b, int *vals)
{
    for (int i = 0; i < N.count; i++) {
        const Node &d = N.n[i];
        int x = d.in0 == SRC_A ? a : d.in0 == SRC_B ? b : vals[d.in0];
        int y = d.in1 == 255 ? x : d.in1 == SRC_A ? a : d.in1 == SRC_B ? b : vals[d.in1];
        vals[i] = evalGate(d.kind, x, y);
    }
    return vals[N.count - 1];
}
static char s_tt[64], s_out[24], s_cnt[48];
static void def(CalcState &s) { s.c[CG] = G_XOR; s.c[CREAL] = 1; s.c[CA] = 1; s.c[CB] = 0; }
static void rows(const CalcState &s, RowList &r)
{
    r.choice("#Porte", CG, GNAMES, 0, "Fonction logique");
    r.choice("#Réalisation", CREAL, "porte seule|NON-ET seules|NON-OU seules", 0, "Avec des portes NAND (ou NOR) à 2 entrées uniquement");
    r.choice("#Symboles", CSYM, "américains|européens (CEI)", 0, "Forme des portes");
    r.choice("#Entrée A", CA, "0|1", 1);
    if (s.c[CG] != G_NOT) r.choice("#Entrée B", CB, "0|1", 2);
    r.head("Résultat");
    r.msg(0);
    r.texOut("", 1, "Équation de la porte");
    r.texOut("", 2, "Équivalence utilisée (théorèmes de De Morgan, double négation)");
    r.msg(3);
    r.msg(4);
}
static void calc(CalcState &s)
{
    const Net &N = NETS[s.c[CREAL]][s.c[CG]];
    int vals[8];
    int out = netEval(N, s.c[CA], s.c[CB], vals);
    int k = scopy(s_out, sizeof s_out, "Sortie S = ");
    s_out[k++] = (char)('0' + out);
    s_out[k] = 0;
    setMsg(s, 0, s_out, out ? ML_OK : ML_INFO);
    setMsg(s, 1, EXPR[s.c[CG]], ML_INFO);
    setMsg(s, 2, s.c[CREAL] == 2 ? EQ_NOR[s.c[CG]] : EQ_NAND[s.c[CG]], ML_INFO);
    // table de verite compacte
    Out o{s_tt, sizeof s_tt, 0};
    o.s("Table (AB→S) : ");
    if (s.c[CG] == G_NOT) {
        for (int a = 0; a < 2; a++) { if (a) o.s(", "); o.c((char)('0' + a)); o.s("→"); o.c((char)('0' + netEval(N, a, 0, vals))); }
    } else {
        for (int m = 0; m < 4; m++) {
            if (m) o.s(", ");
            o.c((char)('0' + (m >> 1)));
            o.c((char)('0' + (m & 1)));
            o.s("→");
            o.c((char)('0' + netEval(N, m >> 1, m & 1, vals)));
        }
    }
    o.end();
    setMsg(s, 3, s_tt, ML_INFO);
    o = Out{s_cnt, sizeof s_cnt, 0};
    char nb[4];
    cu::itoaSimple(N.count, nb);
    o.s(nb);
    o.s(N.count > 1 ? " portes" : " porte");
    if (s.c[CREAL]) o.s(s.c[CREAL] == 1 ? " NON-ET" : " NON-OU");
    o.end();
    setMsg(s, 4, s_cnt, ML_INFO);
}
// Dessin d'une porte : entrees (x, y -/+ 7) (ou (x, y)), sortie renvoyee.
static int drawGate(Sch &g, int kind, int x, int y, bool iec, uint16_t c)
{
    Canvas &cv = *g.cv;
    int X = g.ox + x, Y = g.oy + y;
    const int w = 22, h = 9;
    bool bubble = kind == G_NOT || kind == G_NAND || kind == G_NOR || kind == G_XNOR;
    if (iec) {
        cv.fillRect(X, Y - h - 2, w, 2 * h + 5, g.paper);
        cv.frame(X, Y - h - 2, w, 2 * h + 5, c);
        const char *t = kind == G_AND || kind == G_NAND ? "&" : kind == G_OR || kind == G_NOR ? "\xE2\x89\xA5" "1"
                      : kind == G_XOR || kind == G_XNOR ? "=1" : "1";
        int tw = font::width(SANS_12, t);
        font::drawText(cv, SANS_12, X + (w - tw) / 2, Y + 4, t, c);
    } else if (kind == G_NOT || kind == G_BUF) {
        cv.fillTri(X, Y - h, X, Y + h, X + w - 4, Y, g.paper);
        cv.line(X, Y - h, X, Y + h, c);
        cv.line(X, Y - h, X + w - 4, Y, c);
        cv.line(X, Y + h, X + w - 4, Y, c);
    } else if (kind == G_AND || kind == G_NAND) {
        cv.fillRect(X, Y - h, w - h, 2 * h + 1, g.paper);
        cv.disc(X + w - h, Y, h, g.paper);
        cv.fillRect(X, Y - h, 1, 2 * h + 1, c);
        cv.fillRect(X, Y - h, w - h, 1, c);
        cv.fillRect(X, Y + h, w - h, 1, c);
        cv.halfCircle(X + w - h, Y, h, 1, c);
    } else {
        // OU : dos concave, pointe ; OU exclusif : arc supplementaire
        int px = 0, py = 0, qx = 0, qy = 0;
        for (int i = 0; i <= 12; i++) {
            double t = (i / 12.0) * 2 - 1;   // -1 .. 1
            int xb = X + fm::iround(4 * (1 - t * t));
            int yb = Y + fm::iround(h * t);
            if (i) {
                cv.line(px, py, xb, yb, c);
                if (kind == G_XOR || kind == G_XNOR) cv.line(px - 4, py, xb - 4, yb, c);
            }
            px = xb;
            py = yb;
            (void)qx;
            (void)qy;
        }
        // flancs courbes jusqu'a la pointe
        int ax = X, ay = Y - h, bx = X, by = Y + h;
        for (int i = 1; i <= 10; i++) {
            double t = i / 10.0;
            int x1 = X + fm::iround((w - 1) * t), y1 = Y - fm::iround(h * (1 - t * t));
            int x2 = x1, y2 = Y + fm::iround(h * (1 - t * t));
            cv.line(ax, ay, x1, y1, c);
            cv.line(bx, by, x2, y2, c);
            ax = x1; ay = y1; bx = x2; by = y2;
        }
    }
    int ox = x + w;
    if (bubble) {
        cv.disc(g.ox + ox + 2, Y, 3, g.paper);
        cv.circle(g.ox + ox + 2, Y, 3, c);
        ox += 5;
    }
    return ox;
}
static void sch(Sch &g, const CalcState &s)
{
    const Net &N = NETS[s.c[CREAL]][s.c[CG]];
    bool iec = s.c[CSYM] == 1;
    int a = s.c[CA], b = s.c[CB];
    int vals[8];
    int out = netEval(N, a, b, vals);
    uint16_t on = rgb(220, 40, 40), off = rgb(70, 110, 190);
    int maxL = 0;
    for (int i = 0; i < N.count; i++) if (N.n[i].level > maxL) maxL = N.n[i].level;
    int step = maxL >= 3 ? 34 : maxL == 2 ? 42 : 54;
    int x0 = maxL >= 3 ? 30 : 40;
    int outX[8], outY[8];
    const int yA = 46, yB = 94;
    bool one = s.c[CG] == G_NOT;
    // nombre d'utilisations des sources (points de derivation)
    int chan[8] = {0};
    for (int i = 0; i < N.count; i++) {
        const Node &d = N.n[i];
        int gx = x0 + d.level * step, gy = d.y;
        int ins = d.in1 == 255 ? 1 : 2;
        for (int k = 0; k < ins; k++) {
            int src = k ? d.in1 : d.in0;
            int sx, sy, v;
            if (src == SRC_A) { sx = 8; sy = one ? 70 : yA; v = a; }
            else if (src == SRC_B) { sx = 8; sy = yB; v = b; }
            else { sx = outX[src]; sy = outY[src]; v = vals[src]; }
            int iy = ins == 1 ? gy : gy + (k ? 6 : -6);
            if (d.in0 == d.in1 && d.in1 != 255) iy = gy + (k ? 6 : -6);
            static int cx = 0;
            bool tied = d.in0 == d.in1 && d.in1 != 255;
            if (!(tied && k == 1)) cx = gx - 6 - 4 * (chan[d.level]++ % 3);
            uint16_t c = v ? on : off;
            g.line(sx, sy, cx, sy, c);
            g.line(cx, sy, cx, iy, c);
            g.line(cx, iy, gx, iy, c);
            if (tied && k == 1) g.cv->disc(g.ox + cx, g.oy + (iy + sy) / 2 * 0 + gy, 2, c);
            if (src >= SRC_A) g.cv->disc(g.ox + cx, g.oy + sy, 2, c);
        }
        outX[i] = drawGate(g, d.kind, gx, gy, iec, g.ink);
        outY[i] = gy;
    }
    // sortie
    int ox = outX[N.count - 1], oy = outY[N.count - 1];
    uint16_t oc = out ? on : off;
    g.line(ox, oy, 168, oy, oc);
    g.term(168, oy);
    char t[8] = {'S', '=', (char)('0' + out), 0};
    font::drawText(*g.cv, BOLD_14, g.ox + 177 - font::width(BOLD_14, t), g.oy + oy - 8, t, oc);
    // entrees
    char ta[4] = {'A', '=', (char)('0' + a), 0}, tb[4] = {'B', '=', (char)('0' + b), 0};
    font::drawText(*g.cv, BOLD_12, g.ox + 4, g.oy + (one ? 70 : yA) - 5, ta, a ? on : off);
    if (!one) font::drawText(*g.cv, BOLD_12, g.ox + 4, g.oy + yB + 15, tb, b ? on : off);
    // equation
    const char *e = s.c[CREAL] == 2 ? EQ_NOR[s.c[CG]] : EQ_NAND[s.c[CG]];
    if (!s.c[CREAL]) e = EXPR[s.c[CG]];
    int w = tex::width(e, 1);
    g.cv->setClip(g.ox, g.oy, g.ox + 179, g.oy + 171);
    tex::draw(*g.cv, e, g.ox + (w < 172 ? 89 - w / 2 : 3), g.oy + 150, g.ink, tex::LEFT, 1);
    g.cv->resetClip();
}
}  // namespace gate

// ==========================================================================
// Bases et codes (binaire, hexadecimal, Gray, BCD, complement a 2)
// ==========================================================================
namespace base {
enum { N, BITS, OU, OS, OMIN, OMAX };
enum { CSIGN };
static char s_bin[160], s_hex[32], s_oct[40], s_gray[160], s_bcd[160];
static void def(CalcState &s) { s.v[N] = 181; s.v[BITS] = 8; }
static void rows(const CalcState &s, RowList &r)
{
    r.in("N", N, U_NONE, ST_INT, 1, "Nombre entier (décimal)", RF_INT);
    r.in("n", BITS, U_NONE, ST_INT, 1, "Nombre de bits (1 à 32)", RF_POS | RF_INT);
    r.choice("#Codage", CSIGN, "non signé|signé (complément à 2)", 1, "Interprétation du mot binaire");
    r.head("Écritures");
    r.texOut("", 0, "Binaire (groupes de 4 bits)");
    r.texOut("", 1, "Hexadécimal");
    r.texOut("", 2, "Octal");
    r.texOut("", 3, "Code de Gray : g = b ⊕ (b >> 1)");
    r.texOut("", 4, "Décimal codé binaire (chiffres sur 4 bits)");
    r.head("Valeurs du mot");
    r.outI("N_u", OU, "Valeur non signée du mot de n bits");
    r.outI("N_s", OS, "Valeur signée (complément à 2)");
    r.outI("N_{\\min}", OMIN, s.c[CSIGN] ? "−2ⁿ⁻¹" : "0");
    r.outI("N_{\\max}", OMAX, s.c[CSIGN] ? "2ⁿ⁻¹ − 1" : "2ⁿ − 1");
    r.msg(5);
}
static void bitsTo(Out &o, uint32_t v, int bits, int group)
{
    o.s("\\text{");
    for (int k = bits - 1; k >= 0; k--) {
        o.c((char)('0' + ((v >> k) & 1)));
        if (k && group && k % group == 0) o.s("}\\;\\text{");
    }
    o.c('}');
}
static uint32_t pattern(const CalcState &s, int n, bool &over)
{
    double v = fm::floor(s.v[N] + 0.5);
    double lo = s.c[CSIGN] ? -(double)(1ull << (n - 1)) : 0;
    double hi = s.c[CSIGN] ? (double)(1ull << (n - 1)) - 1 : (double)(1ull << n) - 1;
    over = v < lo || v > hi;
    double m = (double)(1ull << n);
    double r = v - fm::floor(v / m) * m;   // modulo 2^n
    return (uint32_t)(long long)r;
}
static void calc(CalcState &s)
{
    int n = (int)s.v[BITS];
    if (n < 1) n = 1;
    if (n > 32) n = 32;
    s.v[BITS] = n;
    bool over;
    uint32_t p = pattern(s, n, over);
    uint32_t mask = n == 32 ? 0xFFFFFFFFu : ((1u << n) - 1);
    p &= mask;
    Out o{s_bin, sizeof s_bin, 0};
    bitsTo(o, p, n, 4);
    o.end();
    o = Out{s_hex, sizeof s_hex, 0};
    o.s("\\text{0x");
    int hd = (n + 3) / 4;
    for (int k = hd - 1; k >= 0; k--) o.c("0123456789ABCDEF"[(p >> (4 * k)) & 15]);
    o.c('}');
    o.end();
    o = Out{s_oct, sizeof s_oct, 0};
    o.s("\\text{0o");
    int od = (n + 2) / 3;
    for (int k = od - 1; k >= 0; k--) o.c((char)('0' + ((p >> (3 * k)) & 7)));
    o.c('}');
    o.end();
    o = Out{s_gray, sizeof s_gray, 0};
    bitsTo(o, (p ^ (p >> 1)) & mask, n, 4);
    o.end();
    o = Out{s_bcd, sizeof s_bcd, 0};
    double v = fm::floor(s.v[N] + 0.5);
    if (v < 0 || v > 99999999) o.s("\\text{---}");
    else {
        char d[12];
        int nd = 0;
        long long q = (long long)v;
        do { d[nd++] = (char)(q % 10); q /= 10; } while (q && nd < 10);
        o.s("\\text{");
        for (int i = nd - 1; i >= 0; i--) {
            for (int k = 3; k >= 0; k--) o.c((char)('0' + ((d[i] >> k) & 1)));
            if (i) o.s("}\\;\\text{");
        }
        o.c('}');
    }
    o.end();
    setMsg(s, 0, s_bin, ML_INFO);
    setMsg(s, 1, s_hex, ML_INFO);
    setMsg(s, 2, s_oct, ML_INFO);
    setMsg(s, 3, s_gray, ML_INFO);
    setMsg(s, 4, s_bcd, ML_INFO);
    s.v[OU] = (double)p;
    s.v[OS] = (n < 32 ? ((p >> (n - 1)) & 1) : (p >> 31)) ? (double)p - (double)(1ull << n) : (double)p;
    s.v[OMIN] = s.c[CSIGN] ? -(double)(1ull << (n - 1)) : 0;
    s.v[OMAX] = s.c[CSIGN] ? (double)(1ull << (n - 1)) - 1 : (double)(1ull << n) - 1;
    if (over) setMsg(s, 5, "N hors limites : le mot est N modulo 2ⁿ (débordement)", ML_WARN);
    else setMsg(s, 5, "N représentable sur n bits", ML_OK);
}
static void sch(Sch &g, const CalcState &s)
{
    int n = (int)s.v[BITS];
    if (n < 1 || n > 32) return;
    bool over;
    uint32_t p = pattern(s, n, over);
    int perRow = n > 16 ? 16 : n;
    int rowsN = (n + perRow - 1) / perRow;
    int cw = perRow > 8 ? 10 : 18, chh = 18;
    int W = perRow * cw;
    int x0 = (179 - W) / 2;
    for (int i = 0; i < n; i++) {
        int bit = n - 1 - i;
        int r = i / perRow, c = i % perRow;
        int x = x0 + c * cw, y = 40 + r * 44;
        bool one = (p >> bit) & 1;
        bool sign = s.c[CSIGN] && bit == n - 1;
        uint16_t bg = one ? (sign ? rgb(220, 60, 60) : g.accent) : g.paper;
        g.rect(x, y, cw - 1, chh, bg);
        g.frame(x, y, cw, chh + 1, g.ink);
        char t[2] = {(char)(one ? '1' : '0'), 0};
        int f = cw > 12 ? BOLD_14 : BOLD_12;
        font::drawText(*g.cv, f, g.ox + x + (cw - font::width(f, t)) / 2, g.oy + y + 14, t, one ? col::white : g.ink);
        if (bit % 4 == 0 || cw > 12) {
            char b[4];
            cu::itoaSimple(bit, b);
            font::drawText(*g.cv, SANS_12, g.ox + x + (cw - font::width(SANS_12, b)) / 2, g.oy + y + 32, b, g.label);
        }
    }
    (void)rowsN;
    font::drawText(*g.cv, SANS_12, g.ox + x0, g.oy + 30, "rang des bits", g.label);
    tex::draw(*g.cv, s.msg[1] ? s.msg[1] : "", g.ox + 89, g.oy + 150, g.volt, tex::CENTER, 1);
}
}  // namespace base

extern const CalcDef g_calcLogic[] = {
    {"Table de vérité et équivalence", "#Expression → table, Σm, ΠM, forme simplifiée", "logique table verite expression booleenne equivalence simplification minterme maxterme de morgan", 12, G_CUSTOM, 0,
     tv::def, tv::rows, tv::calc, tv::sch, nullptr, nullptr, tv::graph, "Karnaugh", 0, &s_boolKeys, lg::toTex, lg::checkExpr},
    {"Tableau de Karnaugh", "#Saisie des cases, regroupements, S simplifiée", "karnaugh tableau simplification minterme maxterme regroupement indifferent", 12, G_CUSTOM, CF_DIGITCHOICE,
     km::def, km::rows, km::calc, km::sch, nullptr, nullptr, km::graph, "Agrandir"},
    {"Portes logiques et équivalences", "#ET, OU, NON-ET… ; réalisation en NAND ou NOR", "porte logique et ou non nand nor xor equivalence de morgan symbole", 12, G_NONE, 0,
     gate::def, gate::rows, gate::calc, gate::sch},
    {"Bases et codes binaires", "#Binaire, hexadécimal, Gray, BCD, complément à 2", "base binaire hexadecimal octal gray bcd complement deux signe bits conversion", 12, G_NONE, 0,
     base::def, base::rows, base::calc, base::sch},
};
extern const int g_calcLogicN = sizeof(g_calcLogic) / sizeof(g_calcLogic[0]);
