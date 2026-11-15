#!/usr/bin/env python3
"""Capture d'ecran de chaque calculateur (et de son graphe) avec le simulateur.

Usage : python3 tools/tests/shots.py SORTIE [section ...]
Les comptes de calculateurs par section sont lus dans src/calc/sec_*.cpp.
"""
import os, re, subprocess, sys
from PIL import Image

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
SECS = list(range(17))

def counts():
    """Nombre de calculateurs par section (champ section des tables CalcDef)."""
    n = [0] * 17
    d = os.path.join(ROOT, 'src', 'calc')
    for f in os.listdir(d):
        if not f.startswith('sec_'):
            continue
        s = open(os.path.join(d, f), encoding='utf-8').read()
        for m in re.finditer(r'(?:^\s*\{|AMP_DEF\()"[^"]*",\s*"[^"]*",\s*"[^"]*",\s*(\d+),', s, re.M):
            n[int(m.group(1))] += 1
    return n

_N = counts()

def count(sec):
    return _N[sec]

def main():
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    only = [int(a) for a in sys.argv[2:]] or list(range(len(SECS)))
    lines = []
    cur = 0
    for si in only:
        n = count(SECS[si])
        for j in range(n):
            d = (si - cur) % len(SECS)
            if d:
                lines.append('key RIGHT %d' % d)
            cur = si
            lines.append('key EXE')
            if j:
                lines.append('key DOWN %d' % j)
            lines.append('key EXE')
            lines.append('shot %s/s%d_c%02d.ppm' % (out, si, j))
            lines.append('key F4')
            lines.append('shot %s/s%d_c%02d_g.ppm' % (out, si, j))
            lines.append('key EXIT')
            lines.append('key EXIT')
            lines.append('key EXIT')
            lines.append('key EXIT')
    script = os.path.join(out, 'script.txt')
    open(script, 'w').write('\n'.join(lines) + '\n')
    sim = os.path.join(ROOT, 'tools', 'host_sim', 'electronique_sim')
    r = subprocess.run([sim, script], capture_output=True, text=True)
    sys.stderr.write(r.stderr[-2000:])
    for f in sorted(os.listdir(out)):
        if f.endswith('.ppm'):
            im = Image.open(os.path.join(out, f))
            im.resize((im.width * 2, im.height * 2), Image.NEAREST).save(os.path.join(out, f[:-4] + '.png'))
            os.remove(os.path.join(out, f))

main()
