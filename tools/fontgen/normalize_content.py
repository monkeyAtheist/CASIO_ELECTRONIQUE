#!/usr/bin/env python3
"""Typographie francaise des fiches : espace insecable (U+00A0) avant : ; ! ?
et a l'interieur des guillemets « », dans les lignes de texte (hors formules $...$).
Usage : python3 tools/fontgen/normalize_content.py   (depuis la racine du projet)
"""
import glob
import os
import re

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
NBSP = ' '
# titres de page et de carte exclus : une ligne, et le titre de page doit
# rester identique au titre du calculateur
TEXT_DIRECTIVES = ('@text', '@li', '@note', '@sub')


def fix_text(t):
    t = re.sub(r' +([:;!?])', NBSP + r'\1', t)
    t = re.sub(r'« +', '«' + NBSP, t)
    t = re.sub(r' +»', NBSP + '»', t)
    return t


def fix_line(line):
    if not line.startswith(TEXT_DIRECTIVES):
        return line
    head, sep, rest = line.partition(' ')
    parts = rest.split('$')
    for i in range(0, len(parts), 2):     # hors formules
        parts[i] = fix_text(parts[i])
    return head + sep + '$'.join(parts)


changed = 0
for p in sorted(glob.glob(os.path.join(ROOT, 'src', 'content', 'ch_*.cpp'))):
    src = open(p, encoding='utf-8').read()
    out = '\n'.join(fix_line(l) for l in src.split('\n'))
    if out != src:
        open(p, 'w', encoding='utf-8').write(out)
        changed += 1
print('%d fichier(s) modifie(s)' % changed)
