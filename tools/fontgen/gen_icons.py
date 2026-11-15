#!/usr/bin/env python3
"""Icones de l'add-in (92x64) : assets-cg/icon-uns.png et icon-sel.png.

Un AOP (triangle), une resistance et une sinusoide.
"""
import math
import os
from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
SANSB = '/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf'


def icon(selected):
    S = 4                       # sur-echantillonnage
    W, H = 92 * S, 64 * S
    img = Image.new('RGB', (W, H), (255, 255, 255))
    d = ImageDraw.Draw(img)
    if selected:
        top, bot = (230, 120, 20), (170, 70, 0)
    else:
        top, bot = (252, 244, 232), (242, 222, 196)
    for y in range(H):
        t = y / (H - 1)
        c = tuple(int(top[i] + (bot[i] - top[i]) * t) for i in range(3))
        d.line([(0, y), (W, y)], fill=c)
    d.rounded_rectangle([2 * S, 2 * S, W - 2 * S - 1, H - 2 * S - 1], radius=8 * S,
                        outline=(255, 255, 255) if selected else (200, 110, 30), width=2 * S)
    ink = (255, 255, 255) if selected else (40, 45, 60)
    acc = (255, 230, 120) if selected else (200, 90, 0)
    lw = 2 * S
    # AOP
    x0, y0, w = 34 * S, 30 * S, 26 * S
    tri = [(x0, y0 - w * 0.6), (x0, y0 + w * 0.6), (x0 + w, y0)]
    d.polygon(tri, outline=ink, width=lw)
    d.line([(x0 + 3 * S, y0 - 8 * S), (x0 + 8 * S, y0 - 8 * S)], fill=ink, width=lw)
    d.line([(x0 + 3 * S, y0 + 8 * S), (x0 + 8 * S, y0 + 8 * S)], fill=ink, width=lw)
    d.line([(x0 + 5.5 * S, y0 + 5.5 * S), (x0 + 5.5 * S, y0 + 10.5 * S)], fill=ink, width=lw)
    # fils et resistance de contre-reaction
    d.line([(10 * S, y0 - 8 * S), (x0, y0 - 8 * S)], fill=ink, width=lw)
    d.line([(10 * S, y0 + 8 * S), (x0, y0 + 8 * S)], fill=ink, width=lw)
    d.line([(x0 + w, y0), (84 * S, y0)], fill=ink, width=lw)
    d.line([(24 * S, y0 - 8 * S), (24 * S, 9 * S), (74 * S, 9 * S), (74 * S, y0)], fill=ink, width=lw)
    d.rectangle([41 * S, 6 * S, 57 * S, 12 * S], fill=top if not selected else (220, 110, 15), outline=ink, width=lw)
    # sinusoide de sortie
    pts = []
    for i in range(61):
        t = i / 60
        pts.append(((14 + 64 * t) * S, (54 - 5 * math.sin(t * 4 * math.pi)) * S))
    d.line(pts, fill=acc, width=lw + S)
    return img.resize((92, 64), Image.LANCZOS)


os.makedirs(os.path.join(ROOT, 'assets-cg'), exist_ok=True)
icon(False).save(os.path.join(ROOT, 'assets-cg', 'icon-uns.png'))
icon(True).save(os.path.join(ROOT, 'assets-cg', 'icon-sel.png'))
print('icones ecrites')
