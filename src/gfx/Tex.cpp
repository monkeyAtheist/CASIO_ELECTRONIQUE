#include "Tex.hpp"
#include "DisplayList.hpp"
#include "../math/Math.hpp"
#include "../font/Font.hpp"

namespace tex {

enum { OPS = 400 };
static DrawOp s_ops[OPS];
static DisplayList s_dl;

int textFace(int style)
{
    return style >= 2 ? SANS_12 : SANS_14;
}

static uint8_t mathStyle(int style)
{
    return style >= 2 ? (uint8_t)(math::ST_T | math::ST_SMALL) : (uint8_t)math::ST_T;
}

int width(const char *s, int style)
{
    if (!s) return 0;
    if (s[0] == '#') return font::richWidth(textFace(style), s + 1);
    math::g_oom = false;
    math::Box *b = math::typeset(s, -1, mathStyle(style), false);
    int w = b->w;
    math::resetBoxes();
    return w;
}

int draw(Canvas &cv, const char *s, int x, int base, uint16_t c, int align, int style)
{
    if (!s) return 0;
    if (s[0] == '#') {
        int f = textFace(style);
        int w = font::richWidth(f, s + 1);
        if (align == CENTER) x -= w / 2;
        else if (align == RIGHT) x -= w;
        font::drawRich(cv, f, x, base, s + 1, c);
        return w;
    }
    math::g_oom = false;
    math::Box *b = math::typeset(s, -1, mathStyle(style), false);
    int w = b->w;
    if (align == CENTER) x -= w / 2;
    else if (align == RIGHT) x -= w;
    s_dl.init(s_ops, OPS);
    math::emit(s_dl, b, x, base, c);
    math::resetBoxes();
    s_dl.draw(cv, 0, 0);
    return w;
}

}  // namespace tex
