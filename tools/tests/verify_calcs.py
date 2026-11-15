#!/usr/bin/env python3
"""Verification numerique des calculateurs.

Chaque resultat affiche par l'application est recalcule ici de facon
independante : analyse nodale (numpy) pour les montages a AOP et les filtres,
recherche numerique des frequences de coupure et des maximums, simulation
temporelle (charge RC, NE555, multivibrateur, redressement), tables E retapees,
resolution directe des polarisations de transistors.

Usage : python3 tools/tests/verify_calcs.py   (compile tools/tests/dump_calcs)
"""
import cmath
import math
import os
import random
import subprocess
import sys

import numpy as np

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
BIN = os.path.join(ROOT, 'tools', 'tests', 'dump_calcs')

random.seed(1234)
cases = []
stats = {'ok': 0, 'fail': 0}


def case(title, inputs, check, note=''):
    cases.append((title, dict(inputs), check, note))


def build():
    src = []
    for d, _, files in os.walk(os.path.join(ROOT, 'src')):
        for f in files:
            p = os.path.join(d, f)
            if f.endswith('.cpp') and f != 'main.cpp' and '/ui/' not in p:
                src.append(p)
    src.append(os.path.join(ROOT, 'src', 'ui', 'KeyMap.cpp'))
    cmd = ['g++', '-std=c++17', '-O1', '-fno-exceptions', '-fno-rtti', '-I' + os.path.join(ROOT, 'tools', 'host_sim'),
           '-o', BIN, os.path.join(ROOT, 'tools', 'tests', 'dump_calcs.cpp')] + src
    subprocess.run(cmd, check=True)


class Res:
    def __init__(self, lines):
        self.out, self.inp, self.choice, self.msg, self.err = {}, {}, {}, {}, []
        self.order = []
        for l in lines:
            k, a, b = (l.split('\t') + ['', ''])[:3]
            if k == 'OUT':
                self.out.setdefault(a, float(b))
                self.order.append(a)
            elif k == 'IN':
                self.inp[a] = float(b)
            elif k == 'CHOICE':
                self.choice[a] = int(b)
            elif k == 'MSG':
                self.msg[int(a)] = b
            elif k == 'ERR':
                self.err.append(a + ' ' + b)

    def msgs(self):
        return ' | '.join(v for v in self.msg.values() if v)


CUR = ['']


def check(res, label, expected, rel=1e-9, absol=1e-12, src='OUT'):
    d = res.out if src == 'OUT' else res.inp
    if label not in d:
        stats['fail'] += 1
        print('  ABSENT  %s : %s' % (CUR[0], label))
        return
    got = d[label]
    if expected is None or (isinstance(expected, float) and math.isnan(expected)):
        good = math.isnan(got)
    elif math.isinf(expected):
        good = math.isinf(got) and (got > 0) == (expected > 0)
    else:
        good = abs(got - expected) <= max(absol, rel * abs(expected))
    if good:
        stats['ok'] += 1
    else:
        stats['fail'] += 1
        print('  ECART   %s : %s = %.10g, attendu %.10g' % (CUR[0], label, got, expected))


def check_msg(res, text):
    if any(text in m for m in res.msg.values()):
        stats['ok'] += 1
    else:
        stats['fail'] += 1
        print('  MESSAGE %s : « %s » absent (%s)' % (CUR[0], text, res.msgs()))


def lu(lo, hi):
    """Valeur log-uniforme."""
    return math.exp(random.uniform(math.log(lo), math.log(hi)))


def db(x):
    return 20 * math.log10(x)


def deg(z):
    return math.degrees(cmath.phase(z))


def par(a, b):
    return a * b / (a + b)


# --------------------------------------------------------------------------
# Series E retapees a la main (IEC 60063)
# --------------------------------------------------------------------------
E6 = [1.0, 1.5, 2.2, 3.3, 4.7, 6.8]
E12 = [1.0, 1.2, 1.5, 1.8, 2.2, 2.7, 3.3, 3.9, 4.7, 5.6, 6.8, 8.2]
E24 = sorted(E12 + [1.1, 1.3, 1.6, 2.0, 2.4, 3.0, 3.6, 4.3, 5.1, 6.2, 7.5, 9.1])
E96 = [round(10 ** (i / 96), 2) for i in range(96)]
E48 = E96[::2]
SERIES = {'E6': E6, 'E12': E12, 'E24': E24, 'E48': E48, 'E96': E96}


def evals(series):
    out = []
    for d in range(-15, 13):
        out += [m * 10.0 ** d for m in series]
    return out


def enear(series, x):
    return min(evals(series), key=lambda v: abs(math.log(v / x)))


def eceil(series, x):
    return min(v for v in evals(series) if v >= x * (1 - 1e-9))


def efloor(series, x):
    return max(v for v in evals(series) if v <= x * (1 + 1e-9))


# --------------------------------------------------------------------------
# Outils : impedances, analyse nodale, recherches numeriques
# --------------------------------------------------------------------------
def zc(c, w):
    return 1 / (1j * w * c)


def zl(l, w):
    return 1j * w * l


def ztype(t, r, c, l, w):
    R, C, L = r, zc(c, w), zl(l, w)
    return [R, C, L, R + C, par(R, C), R + L, par(R, L)][t]


def bisect(f, a, b, n=200):
    fa = f(a)
    for _ in range(n):
        m = 0.5 * (a + b)
        fm = f(m)
        if (fm > 0) == (fa > 0):
            a, fa = m, fm
        else:
            b = m
    return 0.5 * (a + b)


def golden_max(f, a, b, n=200):
    """Maximum d'une fonction unimodale sur [a, b] (en log)."""
    g = (math.sqrt(5) - 1) / 2
    la, lb = math.log(a), math.log(b)
    for _ in range(n):
        c, d = lb - g * (lb - la), la + g * (lb - la)
        if f(math.exp(c)) > f(math.exp(d)):
            lb = d
        else:
            la = c
    x = math.exp(0.5 * (la + lb))
    return x, f(x)


def nodal(n, elems, fixed, opamps=()):
    """Analyse nodale. elems : (a, b, Z) ; fixed : {noeud: tension} ;
    opamps : (plus, moins, sortie, gain) avec gain None = ideal (V+ = V-)."""
    N = n
    A = np.zeros((N, N), complex)
    B = np.zeros(N, complex)
    for a, b, z in elems:
        y = 1 / z
        for p, q in ((a, b), (b, a)):
            if p >= 0:
                A[p, p] += y
                if q >= 0:
                    A[p, q] -= y
    # sorties d'AOP : l'equation KCL du noeud de sortie est remplacee par la contrainte
    for (p, m, o, k) in opamps:
        A[o, :] = 0
        B[o] = 0
        if k is None:
            if p >= 0:
                A[o, p] += 1
            if m >= 0:
                A[o, m] -= 1
        else:
            A[o, o] = 1
            if p >= 0:
                A[o, p] -= k
            if m >= 0:
                A[o, m] += k
    for node, v in fixed.items():
        A[node, :] = 0
        A[node, node] = 1
        B[node] = v
    return np.linalg.solve(A, B)


# ==========================================================================
# Associations
# ==========================================================================
def assoc_cases():
    for title, lab, cap, ser in (('Résistances', 'R', False, 'E24'), ('Condensateurs', 'C', True, 'E12'),
                                 ('Inductances', 'L', False, 'E12')):
        for trial in range(4):
            n = random.randint(1, 12)
            vals = [lu(1, 1e6) if lab == 'R' else lu(1e-12, 1e-3) for _ in range(n)]
            mode = random.randint(0, 1)
            inp = {'@n': n, '#Montage': mode}
            for i, v in enumerate(vals):
                inp['%s_%s' % (lab, i + 1 if i < 9 else '{%d}' % (i + 1))] = v
            s = sum(vals)
            p = 1 / sum(1 / v for v in vals)
            ser_v, par_v = (p, s) if cap else (s, p)
            eq = par_v if mode else ser_v

            def chk(r, ser_v=ser_v, par_v=par_v, eq=eq, lab=lab, ser=ser):
                check(r, '%s_{\\text{série}}' % lab, ser_v)
                check(r, '%s_{\\text{par.}}' % lab, par_v)
                near = enear(SERIES[ser], eq)
                check(r, '#%s proche' % ser, near, 1e-9)
                check(r, '#Écart', (near - eq) / eq * 100, 1e-6, 1e-9)
            case(title, inp, chk)
    for trial in range(6):
        r1, mode = lu(10, 1e6), random.randint(0, 1)
        rt = r1 * (random.uniform(0.1, 0.9) if mode else random.uniform(1.1, 5))

        def chk(r, r1=r1, rt=rt, mode=mode):
            rx = r1 * rt / (r1 - rt) if mode else rt - r1
            check(r, 'R_x', rx)
            n = enear(E24, rx)
            check(r, '#E24 proche', n)
            got = par(r1, n) if mode else r1 + n
            check(r, 'R_{\\text{obtenue}}', got)
        case('Résistance à associer', {'#Montage': mode, 'R_1': r1, 'R_{\\text{cible}}': rt}, chk)
    case('Résistance à associer', {'#Montage': 1, 'R_1': 1000, 'R_{\\text{cible}}': 2000},
         lambda r: (check(r, 'R_x', float('nan')), check_msg(r, 'Impossible')))


# ==========================================================================
# Circuits RC, RL, LC, RLC
# ==========================================================================
def rlc_cases():
    for _ in range(5):
        R, C, f, U = lu(10, 1e6), lu(1e-12, 1e-3), lu(1, 1e6), lu(0.1, 100)

        def chk(r, R=R, C=C, f=f, U=U):
            w = 2 * math.pi * f
            Z = R + zc(C, w)
            check(r, '\\tau', R * C)
            check(r, 'f_c', 1 / (2 * math.pi * R * C))
            check(r, 'X_C', abs(zc(C, w)))
            check(r, '|Z|', abs(Z))
            check(r, '\\varphi', deg(Z), 1e-9, 1e-9)
            check(r, 'I', U / abs(Z))
            check(r, 'U_C', U / abs(Z) * abs(zc(C, w)))
            check(r, 'U_R', U / abs(Z) * R)
        case('Circuit RC', {'R': R, 'C': C, 'f': f, 'U': U}, chk)
        L = lu(1e-6, 10)

        def chk2(r, R=R, L=L, f=f, U=U):
            w = 2 * math.pi * f
            Z = R + zl(L, w)
            check(r, '\\tau', L / R)
            check(r, 'f_c', R / (2 * math.pi * L))
            check(r, '|Z|', abs(Z))
            check(r, '\\varphi', deg(Z), 1e-9, 1e-9)
            check(r, 'U_L', U / abs(Z) * w * L)
        case('Circuit RL', {'R': R, 'L': L, 'f': f, 'U': U}, chk2)
    # LC : trois modes coherents
    for _ in range(3):
        L, C = lu(1e-6, 1), lu(1e-12, 1e-4)
        f0 = 1 / (2 * math.pi * math.sqrt(L * C))
        case('Circuit LC (résonance)', {'#Calculer': 0, 'L': L, 'C': C},
             lambda r, f0=f0, L=L, C=C: (check(r, 'f_0', f0), check(r, '\\sqrt{L/C}', math.sqrt(L / C)),
                                         check(r, 'T_0', 1 / f0)))
        case('Circuit LC (résonance)', {'#Calculer': 1, 'L': L, 'f_0': f0}, lambda r, C=C: check(r, 'C', C, 1e-9))
        case('Circuit LC (résonance)', {'#Calculer': 2, 'C': C, 'f_0': f0}, lambda r, L=L: check(r, 'L', L, 1e-9))
    # RLC serie : coupures par recherche numerique
    for _ in range(5):
        R, L, C, f, U = lu(1, 1e3), lu(1e-5, 1), lu(1e-10, 1e-5), lu(10, 1e6), lu(0.1, 10)

        def chk(r, R=R, L=L, C=C, f=f, U=U):
            f0 = 1 / (2 * math.pi * math.sqrt(L * C))
            H = lambda x: abs(R / (R + zl(L, 2 * math.pi * x) + zc(C, 2 * math.pi * x)))
            f1 = bisect(lambda x: H(x) - 1 / math.sqrt(2), f0 * 1e-6, f0)
            f2 = bisect(lambda x: H(x) - 1 / math.sqrt(2), f0, f0 * 1e6)
            Q = math.sqrt(L / C) / R
            check(r, 'f_0', f0)
            check(r, 'Q', Q)
            check(r, '\\xi', 1 / (2 * Q))
            check(r, 'f_1', f1, 1e-7)
            check(r, 'f_2', f2, 1e-7)
            check(r, 'B', f2 - f1, 1e-6)
            Z = R + zl(L, 2 * math.pi * f) + zc(C, 2 * math.pi * f)
            check(r, '|Z|', abs(Z))
            check(r, 'U_C', U / abs(Z) * abs(zc(C, 2 * math.pi * f)))
            w0 = 2 * math.pi * f0
            check(r, 'U_{C0}', U / R * abs(zc(C, w0)))
            check_msg(r, 'pseudo' if Q > 0.5 else 'apériodique')
        case('Circuit RLC série', {'R': R, 'L': L, 'C': C, 'f': f, 'U': U}, chk)
    for _ in range(4):
        R, L, C, f, I = lu(100, 1e6), lu(1e-5, 1), lu(1e-10, 1e-5), lu(10, 1e6), lu(1e-4, 1)

        def chk(r, R=R, L=L, C=C, f=f, I=I):
            w = 2 * math.pi * f
            Z = 1 / (1 / R + 1 / zl(L, w) + 1 / zc(C, w))
            f0 = 1 / (2 * math.pi * math.sqrt(L * C))
            Zf = lambda x: abs(1 / (1 / R + 1 / zl(L, 2 * math.pi * x) + 1 / zc(C, 2 * math.pi * x)))
            f1 = bisect(lambda x: Zf(x) - R / math.sqrt(2), f0 * 1e-6, f0)
            f2 = bisect(lambda x: Zf(x) - R / math.sqrt(2), f0, f0 * 1e6)
            check(r, 'f_0', f0)
            check(r, 'Q', R * math.sqrt(C / L))
            check(r, 'B', f2 - f1, 1e-6)
            check(r, '|Z|', abs(Z))
            check(r, '\\varphi', deg(Z), 1e-9, 1e-9)
            check(r, 'U', abs(Z) * I)
        case('Circuit RLC parallèle', {'R': R, 'L': L, 'C': C, 'f': f, 'I': I}, chk)
    # charge RC : integration numerique (RK4)
    for mode in (0, 1):
        E, U0, R, C = 12.0, random.uniform(-3, 4), lu(100, 1e5), lu(1e-7, 1e-3)
        tau = R * C
        t = tau * random.uniform(0.2, 3)
        uf = 0 if mode else E
        uc = uf + (U0 - uf) * 0.3

        def chk(r, E=E, U0=U0, R=R, C=C, t=t, uf=uf, uc=uc):
            # RK4 de du/dt = (Uf - u)/(RC)
            n = 4000
            h = t / n
            u = U0
            fdu = lambda x: (uf - x) / (R * C)
            for _ in range(n):
                k1 = fdu(u)
                k2 = fdu(u + h / 2 * k1)
                k3 = fdu(u + h / 2 * k2)
                k4 = fdu(u + h * k3)
                u += h / 6 * (k1 + 2 * k2 + 2 * k3 + k4)
            check(r, 'u_C(t)', u, 1e-9, 1e-9)
            check(r, 'i(t)', C * fdu(u), 1e-8, 1e-12)
            # instant d'atteinte par dichotomie sur la solution exacte
            sol = lambda tt: uf + (U0 - uf) * math.exp(-tt / (R * C)) - uc
            check(r, 't_{\\text{cible}}', bisect(sol, 0, 50 * R * C), 1e-9)
        inp = {'#Phase': mode, 'U_0': U0, 'R': R, 'C': C, 't': t, 'U_{\\text{cible}}': uc}
        if not mode:
            inp['E'] = E
        case('Charge et décharge RC', inp, chk)


# ==========================================================================
# Filtres
# ==========================================================================
def filter_cases():
    for title, isC in (('Filtre RC du 1er ordre', True), ('Filtre RL du 1er ordre', False)):
        for typ in (0, 1):
            R, X, f = lu(10, 1e5), (lu(1e-10, 1e-4) if isC else lu(1e-5, 1)), lu(1, 1e6)

            def chk(r, R=R, X=X, f=f, typ=typ, isC=isC):
                w = 2 * math.pi * f
                zx = zc(X, w) if isC else zl(X, w)
                # passe-bas RC : C en sortie ; passe-haut RC : R en sortie
                # passe-bas RL : R en sortie ; passe-haut RL : L en sortie
                if isC:
                    zout, zser = (zx, R) if typ == 0 else (R, zx)
                else:
                    zout, zser = (R, zx) if typ == 0 else (zx, R)
                H = zout / (zout + zser)
                fc = 1 / (2 * math.pi * R * X) if isC else R / (2 * math.pi * X)
                check(r, 'f_c', fc)
                check(r, '|H|', abs(H))
                check(r, 'G', db(abs(H)), 1e-9, 1e-9)
                check(r, '\\varphi', deg(H), 1e-9, 1e-9)
            case(title, {'#Type': typ, 'R': R, ('C' if isC else 'L'): X, 'f': f}, chk)
    for typ in range(4):
        for _ in range(3):
            R, L, C, f = lu(1, 1e4), lu(1e-5, 1), lu(1e-10, 1e-5), lu(10, 1e6)

            def chk(r, R=R, L=L, C=C, f=f, typ=typ):
                def Hf(x):
                    w = 2 * math.pi * x
                    zr, zL, zC = R, zl(L, w), zc(C, w)
                    tot = zr + zL + zC
                    return [zC, zL, zr, zL + zC][typ] / tot
                H = Hf(f)
                f0 = 1 / (2 * math.pi * math.sqrt(L * C))
                Q = math.sqrt(L / C) / R
                check(r, 'f_0', f0)
                check(r, 'Q', Q)
                check(r, '|H|', abs(H))
                check(r, '\\varphi', deg(H), 1e-9, 1e-7)
                if typ in (2, 3):
                    check(r, 'B', f0 / Q)
                if typ in (0, 1) and Q > 0.75:
                    fr, hm = golden_max(lambda x: abs(Hf(x)), f0 / 50, f0 * 50)
                    check(r, 'f_r', fr, 1e-6)
                    check(r, 'G_{\\max}', db(hm), 1e-9, 1e-9)
            case('Filtre RLC du 2e ordre', {'#Sortie': typ, 'R': R, 'L': L, 'C': C, 'f': f}, chk)
    # Sallen-Key : analyse nodale (noeuds : 0 entree, 1 A, 2 B=+, 3 sortie, 4 -)
    for hp in (False, True):
        for gain in (0, 1):
            for _ in range(3):
                R1, R2, C1, C2 = lu(1e3, 1e5), lu(1e3, 1e5), lu(1e-9, 1e-6), lu(1e-9, 1e-6)
                Ra, Rb = lu(1e3, 1e5), lu(1e3, 3e4)
                f = lu(10, 1e5)
                if gain:
                    # garder un montage stable
                    K = 1 + Rb / Ra
                    a1 = ((R1 + R2) * C2 + (1 - K) * R1 * C1) if not hp else (R1 * (C1 + C2) + (1 - K) * R2 * C2)
                    if a1 <= 0:
                        Rb = Ra * 0.2

                def chk(r, R1=R1, R2=R2, C1=C1, C2=C2, Ra=Ra, Rb=Rb, f=f, hp=hp, gain=gain):
                    def Hn(x):
                        w = 2 * math.pi * x
                        if not hp:
                            el = [(0, 1, R1), (1, 2, R2), (1, 3, zc(C1, w)), (2, -1, zc(C2, w))]
                        else:
                            el = [(0, 1, zc(C1, w)), (1, 2, zc(C2, w)), (1, 3, R1), (2, -1, R2)]
                        if gain:
                            el += [(3, 4, Rb), (4, -1, Ra)]
                            v = nodal(5, el, {0: 1.0}, [(2, 4, 3, None)])
                        else:
                            v = nodal(5, el + [(4, -1, 1e3)], {0: 1.0}, [(2, 3, 3, None)])
                        return v[3]
                    H = Hn(f)
                    a2 = R1 * R2 * C1 * C2
                    f0 = 1 / (2 * math.pi * math.sqrt(a2))
                    K = 1 + Rb / Ra if gain else 1.0
                    check(r, 'f_0', f0)
                    check(r, '|H|', abs(H), 1e-8)
                    check(r, '\\varphi', deg(H), 1e-8, 1e-7)
                    check(r, 'Q', abs(Hn(f0)) / K, 1e-8)
                    check(r, 'K', K)
                inp = {'R_1': R1, 'R_2': R2, 'C_1': C1, 'C_2': C2, '#Gain': gain, 'f': f}
                if gain:
                    inp.update({'R_a': Ra, 'R_b': Rb})
                case('Sallen-Key passe-haut' if hp else 'Sallen-Key passe-bas', inp, chk)


# ==========================================================================
# AOP
# ==========================================================================
ZL = {'R': 'R', 1: 'C', 2: 'L'}


def zinputs(k, t, R, C, L):
    d = {'?Z_%d' % k: t}
    if t in (0, 3, 4, 5, 6):
        d['R_%d' % k] = R
    if t in (1, 3, 4):
        d['C_%d' % k] = C
    if t in (2, 5, 6):
        d['L_%d' % k] = L
    return d


def aop_cases():
    for title in ('Amplificateur inverseur', 'Amplificateur non inverseur'):
        for _ in range(10):
            t1, t2 = random.randint(0, 6), random.randint(0, 6)
            R1, C1, L1 = lu(100, 1e6), lu(1e-11, 1e-5), lu(1e-5, 1)
            R2, C2, L2 = lu(100, 1e6), lu(1e-11, 1e-5), lu(1e-5, 1)
            f, Ve, Vs = lu(1, 1e6), random.uniform(0, 3), 13.0

            def chk(r, t1=t1, t2=t2, R1=R1, C1=C1, L1=L1, R2=R2, C2=C2, L2=L2, f=f, Ve=Ve, title=title):
                w = 2 * math.pi * f
                z1, z2 = ztype(t1, R1, C1, L1, w), ztype(t2, R2, C2, L2, w)
                # noeuds : 0 entree, 1 entree -, 2 sortie
                if title.endswith('inverseur') and 'non' not in title:
                    v = nodal(3, [(0, 1, z1), (1, 2, z2)], {0: 1.0}, [(-1, 1, 2, None)])
                else:
                    v = nodal(3, [(1, -1, z1), (1, 2, z2)], {0: 1.0}, [(0, 1, 2, None)])
                H = v[2]
                check(r, '|H|', abs(H), 1e-8)
                check(r, 'G', db(abs(H)), 1e-8, 1e-8)
                check(r, '\\varphi', deg(H), 1e-8, 1e-6)
                check(r, 'V_s', abs(H) * Ve, 1e-8, 1e-12)
                check_msg(r, 'saturée' if abs(H) * Ve > 13 else 'linéaire')
            inp = dict(zinputs(1, t1, R1, C1, L1))
            inp.update(zinputs(2, t2, R2, C2, L2))
            inp.update({'f': f, 'V_e': Ve, 'V_{sat}': Vs})
            case(title, inp, chk)
    # sommateur (noeuds : 0,1,2 entrees, 3 -, 4 sortie)
    for n3 in (0, 1):
        for _ in range(3):
            V = [random.uniform(-2, 2) for _ in range(3)]
            R = [lu(1e3, 1e5) for _ in range(3)]
            Rf = lu(1e3, 1e5)

            def chk(r, V=V, R=R, Rf=Rf, n3=n3):
                el = [(0, 3, R[0]), (1, 3, R[1]), (3, 4, Rf)]
                fixed = {0: V[0], 1: V[1], 2: V[2]}
                if n3:
                    el.append((2, 3, R[2]))
                else:
                    el.append((2, -1, 1.0))
                v = nodal(5, el, fixed, [(-1, 3, 4, None)])
                vs = v[4].real
                check(r, 'V_s', max(-13, min(13, vs)), 1e-9, 1e-12)
            inp = {'#Entrées': n3, 'V_1': V[0], 'R_1': R[0], 'V_2': V[1], 'R_2': R[1], 'R_f': Rf}
            if n3:
                inp.update({'V_3': V[2], 'R_3': R[2]})
            case('Sommateur inverseur', inp, chk)
    # soustracteur (0 V1, 1 V2, 2 -, 3 +, 4 sortie)
    for _ in range(5):
        V1, V2 = random.uniform(-2, 2), random.uniform(-2, 2)
        R1, R2, R3, R4 = [lu(1e3, 1e5) for _ in range(4)]

        def chk(r, V1=V1, V2=V2, R1=R1, R2=R2, R3=R3, R4=R4):
            v = nodal(5, [(0, 2, R1), (2, 4, R2), (1, 3, R3), (3, -1, R4)], {0: V1, 1: V2}, [(3, 2, 4, None)])
            vs = v[4].real
            check(r, 'V_s', max(-13, min(13, vs)), 1e-9, 1e-12)
            # gain de mode commun : V1 = V2 = 1
            vc = nodal(5, [(0, 2, R1), (2, 4, R2), (1, 3, R3), (3, -1, R4)], {0: 1, 1: 1}, [(3, 2, 4, None)])[4].real
            check(r, 'A_{mc}', vc, 1e-9, 1e-12)
        case('Soustracteur', {'V_1': V1, 'V_2': V2, 'R_1': R1, 'R_2': R2, 'R_3': R3, 'R_4': R4}, chk)
    # integrateur et derivateur
    for opt in (0, 1):
        for _ in range(3):
            R1, C, R2, f = lu(1e3, 1e5), lu(1e-9, 1e-6), lu(1e4, 1e7), lu(1, 1e5)

            def chk(r, R1=R1, C=C, R2=R2, f=f, opt=opt):
                w = 2 * math.pi * f
                z2 = par(R2, zc(C, w)) if opt else zc(C, w)
                H = -z2 / R1
                check(r, '|H|', abs(H))
                check(r, '\\varphi', deg(H), 1e-9, 1e-7)
                check(r, '\\tau', R1 * C)
                if opt:
                    check(r, 'A_0', -R2 / R1)
                    check(r, 'f_c', 1 / (2 * math.pi * R2 * C))
            inp = {'R_1': R1, 'C': C, '?R_2': opt, 'f': f}
            if opt:
                inp['R_2'] = R2
            case('Intégrateur', inp, chk)
            C1 = C

            def chk2(r, R1=R1, C1=C1, R2=R2, f=f, opt=opt):
                w = 2 * math.pi * f
                z1 = (R1 + zc(C1, w)) if opt else zc(C1, w)
                H = -R2 / z1
                check(r, '|H|', abs(H))
                check(r, '\\varphi', deg(H), 1e-9, 1e-7)
                check(r, '\\tau', R2 * C1)
            inp2 = {'C_1': C1, '?R_1': opt, 'R_2': R2, 'f': f}
            if opt:
                inp2['R_1'] = R1
            case('Dérivateur', inp2, chk2)
    # triggers de Schmitt : seuils trouves en resolvant V+ = V- pour chaque etat
    for inv in (True, False):
        for _ in range(5):
            R1, R2 = lu(1e3, 1e5), lu(1e3, 1e5)
            Vref, Vh, Vl = random.uniform(-3, 3), random.uniform(5, 15), random.uniform(-15, -1)

            def chk(r, R1=R1, R2=R2, Vref=Vref, Vh=Vh, Vl=Vl, inv=inv):
                def vplus(ve, vs):
                    if inv:   # + : R1 vers Vref, R2 vers Vs
                        return (Vref / R1 + vs / R2) / (1 / R1 + 1 / R2)
                    return (ve / R1 + vs / R2) / (1 / R1 + 1 / R2)
                def thr(vs):
                    if inv:   # Ve sur l'entree - : seuil = V+
                        return vplus(0, vs)
                    return bisect(lambda ve: vplus(ve, vs) - Vref, -1e4, 1e4)
                ths = sorted([thr(Vh), thr(Vl)])
                check(r, 'V_{T+}', ths[1], 1e-9, 1e-9)
                check(r, 'V_{T-}', ths[0], 1e-9, 1e-9)
                check(r, '\\Delta V', ths[1] - ths[0], 1e-9, 1e-9)
            case('Trigger de Schmitt inverseur' if inv else 'Trigger de Schmitt non inverseur',
                 {'R_1': R1, 'R_2': R2, 'V_{ref}': Vref, 'V_{sat}^{+}': Vh, 'V_{sat}^{-}': Vl}, chk)
    # amplificateur d'instrumentation (0 V1, 1 V2, 2 -A1, 3 -A2, 4 A1, 5 A2, 6 -B, 7 +B, 8 sortie)
    for _ in range(4):
        V1, V2 = random.uniform(-1, 1), random.uniform(-1, 1)
        RG, R1, R2, R3 = lu(100, 1e4), lu(1e3, 5e4), lu(1e3, 5e4), lu(1e3, 5e4)

        def chk(r, V1=V1, V2=V2, RG=RG, R1=R1, R2=R2, R3=R3):
            el = [(2, 4, R1), (2, 3, RG), (3, 5, R1), (4, 6, R2), (6, 8, R3), (5, 7, R2), (7, -1, R3)]
            v = nodal(9, el, {0: V1, 1: V2}, [(0, 2, 4, None), (1, 3, 5, None), (7, 6, 8, None)])
            check(r, 'V_{A1}', v[4].real, 1e-9, 1e-12)
            check(r, 'V_{A2}', v[5].real, 1e-9, 1e-12)
            check(r, 'V_s', v[8].real, 1e-9, 1e-12)
            check(r, 'G', (1 + 2 * R1 / RG) * R3 / R2)
        case("Amplificateur d'instrumentation", {'V_1': V1, 'V_2': V2, 'R_G': RG, 'R_1': R1, 'R_2': R2, 'R_3': R3}, chk)
    case('Convertisseur courant-tension', {'I_e': 2e-4, 'R': 22e3, '?C': 1},
         lambda r: (check(r, 'V_s', -4.4), check(r, 'f_c', 1 / (2 * math.pi * 22e3 * 100e-12))))
    case('Suiveur', {'V_e': 3.3, 'R_L': 100}, lambda r: (check(r, 'V_s', 3.3), check(r, 'I_s', 0.033),
                                                         check_msg(r, '20 mA')))


# ==========================================================================
# Transistors bipolaires : resolution directe
# ==========================================================================
def bjt_solve(vcc, vth, rth, rc, re, beta, vbe, vces):
    """Point de repos par resolution lineaire (numpy) ; renvoie (ib, ic, vce, mode)."""
    if vth <= vbe:
        return 0.0, 0.0, vcc, 0
    # actif : inconnues ib ; ic = beta ib
    ib = (vth - vbe) / (rth + (beta + 1) * re)
    ic = beta * ib
    vce = vcc - rc * ic - re * (ib + ic)
    if vce >= vces:
        return ib, ic, vce, 1
    # sature : deux equations lineaires en (ib, ic)
    A = np.array([[rth + re, re], [re, rc + re]])
    b = np.array([vth - vbe, vcc - vces])
    ib, ic = np.linalg.solve(A, b)
    return ib, ic, vces, 2


def bjt_cases():
    for _ in range(12):
        npn = random.randint(0, 1) == 0
        vcc, r1, r2 = random.uniform(5, 24), lu(5e3, 2e5), lu(1e3, 1e5)
        rc, re, beta, vbe, vces = lu(100, 2e4), random.choice([0.0, lu(10, 5e3)]), random.uniform(50, 400), 0.65, 0.2
        dec = random.randint(0, 1)

        def chk(r, npn=npn, vcc=vcc, r1=r1, r2=r2, rc=rc, re=re, beta=beta, vbe=vbe, vces=vces, dec=dec):
            vthG = vcc * r2 / (r1 + r2)
            vth = vthG if npn else vcc - vthG
            ib, ic, vce, mode = bjt_solve(vcc, vth, par(r1, r2), rc, re, beta, vbe, vces)
            check(r, 'I_B', ib, 1e-9, 1e-15)
            check(r, 'I_C', ic, 1e-9, 1e-15)
            check(r, 'V_{CE}' if npn else 'V_{EC}', vce, 1e-9, 1e-12)
            check(r, 'V_C', (vcc - rc * ic) if npn else rc * ic, 1e-9, 1e-12)
            check_msg(r, ['Bloqué', 'Linéaire' if vce < 1 else 'linéaire', 'Saturé'][mode])
            if mode == 1:
                # petits signaux : schema en pi, v_b = 1
                gm = ic / 0.026
                rpi = beta / gm
                rea = 0 if dec else re
                # v_b - v_e = rpi ib ; v_e = (beta+1) ib rea ; v_c = -rc beta ib
                ibs = 1 / (rpi + (beta + 1) * rea)
                check(r, 'A_v', -rc * beta * ibs, 1e-9)
                check(r, 'Z_e', par(par(r1, r2), 1 / ibs), 1e-9)
        inp = {'#Transistor': 0 if npn else 1, 'V_{CC}': vcc, 'R_1': r1, 'R_2': r2, 'R_C': rc, 'R_E': re,
               '\\beta': beta, '?C_E': dec}
        case('Polarisation par pont diviseur', inp, chk)
    for _ in range(8):
        npn = random.randint(0, 1) == 0
        vcc, rb, rc, re, beta = random.uniform(5, 24), lu(1e4, 2e6), lu(100, 1e4), random.choice([0.0, lu(10, 1e3)]), random.uniform(50, 400)

        def chk(r, npn=npn, vcc=vcc, rb=rb, rc=rc, re=re, beta=beta):
            ib, ic, vce, mode = bjt_solve(vcc, vcc, rb, rc, re, beta, 0.65, 0.2)
            check(r, 'I_B', ib, 1e-9, 1e-15)
            check(r, 'I_C', ic, 1e-9, 1e-15)
            check(r, 'V_{CE}' if npn else 'V_{EC}', vce, 1e-9, 1e-12)
        case('Polarisation par résistance de base', {'#Transistor': 0 if npn else 1, 'V_{CC}': vcc, 'R_B': rb,
                                                     'R_C': rc, 'R_E': re, '\\beta': beta}, chk)
    for _ in range(5):
        vcc, rl, vin, beta, k = random.uniform(5, 24), lu(20, 2e3), random.choice([3.3, 5.0]), random.uniform(50, 300), random.uniform(2, 5)

        def chk(r, vcc=vcc, rl=rl, vin=vin, beta=beta, k=k):
            ic = (vcc - 0.2) / rl
            ibm = ic / beta
            rbmax = (vin - 0.75) / (k * ibm)
            check(r, 'I_C', ic)
            check(r, 'R_{B\\max}', rbmax)
            check(r, 'R_B', efloor(E24, rbmax))
        case('Transistor en commutation', {'V_{CC}': vcc, 'R_L': rl, 'V_{in}': vin, '\\beta_{\\min}': beta, 'k': k}, chk)


# ==========================================================================
# Transistors a effet de champ : resolution par polynomes
# ==========================================================================
def mos_point(vdd, vg, rd, rs, vth, k):
    """Point de repos par racines de polynomes (numpy) dans chaque regime."""
    if vg <= vth:
        return 0.0, 0
    # saturation : I = k (vg - rs I - vth)^2
    u0 = vg - vth
    cands = []
    for I in np.roots([k * rs * rs, -(2 * k * rs * u0 + 1), k * u0 * u0]):
        I = I.real
        if I >= 0 and vg - rs * I - vth >= 0:
            vds = vdd - I * (rd + rs)
            if vds >= vg - rs * I - vth:
                cands.append((I, 2))
    if cands:
        return min(cands)[0], 2
    # ohmique : I = k (2 u vds - vds^2), u = u0 - rs I, vds = vdd - (rd+rs) I
    a = rd + rs
    # 2 (u0 - rs I)(vdd - a I) - (vdd - a I)^2
    p = np.polynomial.polynomial
    u = [u0, -rs]
    v = [vdd, -a]
    poly = p.polysub(p.polymul([0, 0], [1]), [0])
    expr = p.polysub(p.polymul([2 * k], p.polymul(u, v)), p.polymul([k], p.polymul(v, v)))
    eq = p.polysub(expr, [0, 1])
    for I in p.polyroots(eq):
        if abs(I.imag) < 1e-12 and I.real >= 0:
            I = I.real
            vds = vdd - a * I
            if 0 <= vds <= u0 - rs * I + 1e-12:
                return I, 1
    return float('nan'), -1


def fet_cases():
    for _ in range(12):
        n = random.randint(0, 1) == 0
        vdd, r1, r2 = random.uniform(5, 24), lu(1e5, 5e6), lu(1e5, 5e6)
        rd, rs, vth, k = lu(100, 1e4), random.choice([0.0, lu(10, 5e3)]), random.uniform(0.5, 3), lu(1e-4, 0.1)

        def chk(r, n=n, vdd=vdd, r1=r1, r2=r2, rd=rd, rs=rs, vth=vth, k=k):
            vgG = vdd * r2 / (r1 + r2)
            vg = vgG if n else vdd - vgG
            I, mode = mos_point(vdd, vg, rd, rs, vth, k)
            check(r, 'I_D', I, 1e-9, 1e-14)
            check(r, 'V_{GS}' if n else 'V_{SG}', vg - rs * I, 1e-9, 1e-12)
            check(r, 'V_{DS}' if n else 'V_{SD}', vdd - (rd + rs) * I, 1e-9, 1e-12)
            check_msg(r, ['Bloqué', 'ohmique', 'Saturation'][mode])
            if mode == 2:
                gm = 2 * math.sqrt(k * I)
                check(r, 'g_m', gm, 1e-8)
        case('MOSFET : polarisation', {'#Canal': 0 if n else 1, 'V_{DD}': vdd, 'R_1': r1, 'R_2': r2, 'R_D': rd,
                                       'R_S': rs, 'V_{th}' if n else '|V_{th}|': vth, 'K': k}, chk)
    for _ in range(10):
        n = random.randint(0, 1) == 0
        div = random.randint(0, 1)
        vdd, rd, rs, idss, vp = random.uniform(9, 24), lu(500, 5e3), lu(100, 2e3), lu(2e-3, 20e-3), random.uniform(1, 6)
        r1, r2 = lu(1e5, 5e6), lu(1e4, 5e5)

        def chk(r, n=n, div=div, vdd=vdd, rd=rd, rs=rs, idss=idss, vp=vp, r1=r1, r2=r2):
            vgG = vdd * r2 / (r1 + r2) if div else (0 if n else vdd)
            vg = vgG if n else vdd - vgG
            # saturation : I = idss (1 + (vg - rs I)/vp)^2
            a = 1 + vg / vp
            cands = []
            for I in np.roots([idss * rs * rs / vp ** 2, -(2 * idss * a * rs / vp + 1), idss * a * a]):
                I = I.real
                vgs = vg - rs * I
                if I >= 0 and 1 + vgs / vp >= 0 and vdd - (rd + rs) * I >= vgs + vp:
                    cands.append(I)
            if not cands:
                return
            I = min(cands)
            vgs = vg - rs * I
            check(r, 'I_D', I, 1e-9, 1e-14)
            check(r, 'V_{GS}', vgs if n else -vgs, 1e-9, 1e-12)
            check(r, 'g_m', 2 * idss / vp * (1 + vgs / vp), 1e-8)
        inp = {'#Canal': 0 if n else 1, '#Polarisation': div, 'V_{DD}': vdd, 'R_D': rd, 'R_S': rs,
               'I_{DSS}': idss, '|V_P|': vp}
        if div:
            inp.update({'R_1': r1, 'R_2': r2})
        case('JFET : polarisation', inp, chk)
    case('MOSFET en commutation', {'V_{DD}': 48, 'I_D': 10, 'R_{DS(on)}': 0.01, '\\alpha': 40, 'Q_g': 50e-9,
                                   'I_G': 1, 'f_s': 100e3, 'R_{th\\,ja}': 10, 'T_a': 25},
         lambda r: (check(r, 'P_{\\text{cond}}', 0.4 * 0.01 * 100), check(r, 'P_{\\text{com}}', 48 * 10 * 50e-9 * 100e3),
                    check(r, 'T_j', 25 + 10 * (0.4 + 2.4))))


# ==========================================================================
# Lois de base, alimentations
# ==========================================================================
def base_cases():
    U, I = 9.0, 0.03
    R, P = U / I, U * I
    pairs = [('U', 'I'), ('U', 'R'), ('I', 'R'), ('P', 'U'), ('P', 'I'), ('P', 'R')]
    vals = {'U': U, 'I': I, 'R': R, 'P': P}
    for k, (a, b) in enumerate(pairs):
        def chk(r, a=a, b=b):
            for x in 'UIRP':
                if x not in (a, b):
                    check(r, x, vals[x], 1e-12)
        case("Loi d'Ohm et puissance", {'#Données': k, a: vals[a], b: vals[b]}, chk)
    for load in (0, 1):
        Ve, R1, R2, RL = 15.0, lu(1e3, 1e5), lu(1e3, 1e5), lu(1e3, 1e5)

        def chk(r, Ve=Ve, R1=R1, R2=R2, RL=RL, load=load):
            v = nodal(2, [(0, 1, R1), (1, -1, R2)] + ([(1, -1, RL)] if load else []), {0: Ve})
            check(r, 'V_s', v[1].real)
            check(r, 'R_{th}', par(R1, R2))
        inp = {'#Charge': load, 'V_e': Ve, 'R_1': R1, 'R_2': R2}
        if load:
            inp['R_L'] = RL
        case('Diviseur de tension', inp, chk)

        def chk2(r, Ve=Ve, R1=R1, RL=RL, load=load):
            vs = 3.3
            r2 = bisect(lambda x: nodal(2, [(0, 1, R1), (1, -1, x)] + ([(1, -1, RL)] if load else []), {0: Ve})[1].real - vs, 1e-3, 1e9)
            if load and R1 * vs / (Ve - vs) >= RL:
                check(r, 'R_2', float('nan'))
                return
            check(r, 'R_2', r2, 1e-7)
        inp = {'#Calculer': 1, '#Charge': load, 'V_e': Ve, 'R_1': R1, 'V_s': 3.3}
        if load:
            inp['R_L'] = RL
        case('Diviseur de tension', inp, chk2)
    for _ in range(3):
        E, R = random.uniform(1, 20), [lu(100, 1e4) for _ in range(4)]

        def chk(r, E=E, R=R):
            v = nodal(3, [(0, 1, R[0]), (1, -1, R[1]), (0, 2, R[2]), (2, -1, R[3])], {0: E})
            check(r, 'V_{AB}', (v[1] - v[2]).real, 1e-9, 1e-12)
            check(r, 'R_4\\ (\\text{équil.})', R[1] * R[2] / R[0])
        case('Pont de Wheatstone', {'E': E, 'R_1': R[0], 'R_2': R[1], 'R_3': R[2], 'R_4': R[3]}, chk)
    for col, vf in enumerate([2.0, 2.1, 2.2, 3.1, 3.2, 1.3]):
        def chk(r, vf=vf):
            rr = (12 - 2 * vf) / 0.015
            check(r, 'R', rr)
            check(r, '#E24 au-dessus', eceil(E24, rr))
        case("Résistance d'une LED", {'#LED': col, 'V_{CC}': 12, 'I_F': 0.015, 'n': 2}, chk)
    case("Résistance d'une LED", {'#LED': 3, 'V_{CC}': 5, 'n': 2}, lambda r: check_msg(r, 'trop faible'))
    for typ in (0, 1):
        k = 20 if typ == 0 else 10
        x = lu(1e-3, 1e3)
        case('Décibels', {'#Rapport': typ, '#Sens': 0, 'V_s/V_e' if typ == 0 else 'P_s/P_e': x},
             lambda r, x=x, k=k: check(r, 'G', k * math.log10(x)))
        g = random.uniform(-60, 60)
        case('Décibels', {'#Rapport': typ, '#Sens': 1, 'G': g},
             lambda r, g=g, k=k, typ=typ: check(r, 'V_s/V_e' if typ == 0 else 'P_s/P_e', 10 ** (g / k)))
    case('Niveaux dBm, dBV, dBu', {'U_{\\text{eff}}': 1.0, 'R': 50},
         lambda r: (check(r, '#En dBm', 10 * math.log10(1 / 50 / 1e-3)), check(r, '#En dBV', 0, 1e-12, 1e-12),
                    check(r, '#En dBu', 20 * math.log10(1 / math.sqrt(0.6)))))
    case('Dissipateur thermique', {'P': 20, 'T_{j\\max}': 150, 'T_a': 50, 'R_{th\\,jc}': 1, 'R_{th\\,cs}': 0.5,
                                   'R_{th\\,sa}': 2, 'R_{th\\,ja}': 40},
         lambda r: (check(r, 'R_{th\\,sa\\,\\max}', 100 / 20 - 1.5), check(r, 'T_j', 50 + 20 * 3.5)))
    case('Énergie stockée', {'C': 470e-6, 'U': 35, 'L': 0.1, 'I': 3},
         lambda r: (check(r, 'W_C', 0.5 * 470e-6 * 35 ** 2), check(r, 'W_L', 0.45), check(r, 'Q', 470e-6 * 35)))


def alim_cases():
    for _ in range(3):
        R1, R2 = random.choice([120, 240, 220]), lu(100, 5e3)

        def chk(r, R1=R1, R2=R2):
            # noeuds : 0 sortie, 1 ADJ ; V(0) - V(1) = 1,25 ; I_adj = 50 uA entre dans R2
            vs = 1.25 + R2 * (1.25 / R1 + 50e-6)
            check(r, 'V_s', vs)
        case('Régulateur LM317', {'R_1': R1, 'R_2': R2}, chk)
    case('Régulateur LM317', {'#Calculer': 1, 'R_1': 240, 'V_s': 12},
         lambda r: check(r, 'R_2', (12 - 1.25) / (1.25 / 240 + 50e-6)))
    for _ in range(3):
        vmin, vmax, vz, izm, ilm = random.uniform(9, 12), random.uniform(12, 18), random.uniform(3, 8), 5e-3, lu(1e-3, 0.1)

        def chk(r, vmin=vmin, vmax=vmax, vz=vz, izm=izm, ilm=ilm):
            rsm = (vmin - vz) / (ilm + izm)
            rs = efloor(E24, rsm)
            check(r, 'R_{s\\max}', rsm)
            check(r, 'R_s', rs)
            check(r, 'P_Z', vz * (vmax - vz) / rs)
        case('Régulation par diode Zener', {'V_{e\\,\\min}': vmin, 'V_{e\\,\\max}': vmax, 'V_Z': vz,
                                            'I_{Z\\min}': izm, 'I_{L\\max}': ilm}, chk)
    for typ in range(3):
        def chk(r, typ=typ):
            n = 2 if typ == 1 else 1
            fr = 50 if typ == 0 else 100
            vmax = 12 * math.sqrt(2) - n * 0.7
            check(r, 'V_{\\max}', vmax)
            check(r, '\\Delta V', 0.5 / (fr * 2200e-6))
            check(r, 'C_{\\min}', 0.5 / fr)
        case('Redressement et filtrage', {'#Montage': typ}, chk)


# ==========================================================================
# Code couleur, series E, marquages
# ==========================================================================
NAMES = ['Noir', 'Marron', 'Rouge', 'Orange', 'Jaune', 'Vert', 'Bleu', 'Violet', 'Gris', 'Blanc']


def code_cases():
    for _ in range(8):
        five = random.randint(0, 1)
        d = [random.randint(1, 9), random.randint(0, 9), random.randint(0, 9)]
        m = random.randint(0, 11)
        t = random.randint(0, 8)
        tol = [1, 2, 0.5, 0.25, 0.1, 0.05, 5, 10, 20][t]

        def chk(r, five=five, d=d, m=m, tol=tol):
            val = (d[0] * 100 + d[1] * 10 + d[2] if five else d[0] * 10 + d[1]) * 10.0 ** (m - 2)
            check(r, 'R', val)
            check(r, 'R_{\\min}', val * (1 - tol / 100))
            check(r, 'R_{\\max}', val * (1 + tol / 100))
        inp = {'#Anneaux': five, '#Chiffre 1': d[0], '#Chiffre 2': d[1], '#Multiplicateur': m, '#Tolérance': t}
        if five:
            inp['#Chiffre 3'] = d[2]
        case('Code couleur → valeur', inp, chk)
    for _ in range(8):
        five = random.randint(0, 1)
        R = lu(0.1, 1e8)

        def chk(r, five=five, R=R):
            nd = 3 if five else 2
            e = math.floor(math.log10(R)) - (nd - 1)
            m = round(R / 10 ** e)
            if m >= 10 ** nd:
                m, e = round(m / 10), e + 1
            check(r, 'R\\ (\\text{marquée})', m * 10.0 ** e, 1e-9)
            digits = str(m)
            for i, ch in enumerate(digits):
                check_msg(r, 'Anneau %d : %s (%s)' % (i + 1, NAMES[int(ch)], ch))
        case('Valeur → code couleur', {'R': R, '#Anneaux': five}, chk)
    for _ in range(10):
        x = lu(0.5, 1e7)

        def chk(r, x=x):
            for nm in ('E6', 'E12', 'E24', 'E48', 'E96'):
                n = enear(SERIES[nm], x)
                check(r, '#' + nm, n, 1e-9)
        case('Séries E (E6 à E96)', {'x': x}, chk)
    for code, val in ((472, 4700), (1002, 10000), (100, 10), (4701, 4700), (229, 2.2), (47, 47)):
        case('Marquages CMS et condensateurs', {'#Code': code}, lambda r, val=val: check(r, 'R', val, 1e-12))
    for code, val in ((104, 100e-9), (223, 22e-9), (102, 1e-9), (479, 4.7e-12)):
        case('Marquages CMS et condensateurs', {'#Composant': 1, '#Code': code}, lambda r, val=val: check(r, 'C', val, 1e-12))


# ==========================================================================
# Oscillateurs : simulations temporelles
# ==========================================================================
def osc_cases():
    for diode in (0, 1):
        RA, RB, C = lu(1e3, 1e5), lu(1e3, 1e5), lu(1e-9, 1e-5)

        def chk(r, RA=RA, RB=RB, C=C, diode=diode):
            # simulation : charge vers Vcc par (RA [+ RB]), decharge vers 0 par RB
            vcc = 1.0
            tc, td = (RA if diode else RA + RB) * C, RB * C
            # temps de charge de vcc/3 a 2vcc/3 et de decharge : dichotomie sur la solution exacte
            th = bisect(lambda t: vcc - (2 * vcc / 3) * math.exp(-t / tc) - 2 * vcc / 3, 0, 50 * tc)
            tl = bisect(lambda t: (2 * vcc / 3) * math.exp(-t / td) - vcc / 3, 0, 50 * td)
            check(r, 't_H', th, 1e-9)
            check(r, 't_L', tl, 1e-9)
            check(r, 'f', 1 / (th + tl), 1e-9)
        case('NE555 astable', {'R_A': RA, 'R_B': RB, 'C': C, '#Diode': diode}, chk)
    case('NE555 monostable', {'R': 1e5, 'C': 1e-5}, lambda r: check(r, 'T', math.log(3) * 1.0))
    case('NE555 monostable', {'#Calculer': 1, 'T': 2.0, 'C': 1e-5}, lambda r: check(r, 'R', 2.0 / (math.log(3) * 1e-5)))
    for _ in range(3):
        R, C, R1, R2 = lu(1e3, 1e5), lu(1e-9, 1e-6), lu(1e3, 1e5), lu(1e3, 1e5)

        def chk(r, R=R, C=C, R1=R1, R2=R2):
            # simulation du multivibrateur (RK4) : periode mesuree
            vs, b = 12.0, R1 / (R1 + R2)
            tau = R * C
            u, out = 0.0, vs
            h = tau / 2000
            t, crossings = 0.0, []
            while len(crossings) < 5 and t < 200 * tau:
                fdu = lambda x: (out - x) / tau
                k1 = fdu(u); k2 = fdu(u + h / 2 * k1); k3 = fdu(u + h / 2 * k2); k4 = fdu(u + h * k3)
                un = u + h / 6 * (k1 + 2 * k2 + 2 * k3 + k4)
                th = b * out
                if (out > 0 and un >= th) or (out < 0 and un <= th):
                    # instant exact du franchissement (interpolation lineaire)
                    frac = (th - u) / (un - u)
                    crossings.append(t + frac * h)
                    u = th
                    t += frac * h
                    out = -out
                    continue
                u = un
                t += h
            T = crossings[4] - crossings[2]
            check(r, 'T', T, 1e-4)
        case('Multivibrateur astable à AOP', {'R': R, 'C': C, 'R_1': R1, 'R_2': R2}, chk)
    for _ in range(2):
        R, C = lu(1e3, 1e5), lu(1e-10, 1e-6)

        def chk(r, R=R, C=C):
            # f0 : frequence ou le reseau de Wien a un dephasage nul (noeuds : 0 sortie, 1 milieu, 2 +)
            def beta(f):
                w = 2 * math.pi * f
                v = nodal(3, [(0, 1, R), (1, 2, zc(C, w)), (2, -1, par(R, zc(C, w)))], {0: 1.0})
                return v[2]
            f0 = bisect(lambda f: cmath.phase(beta(f)), 1 / (2 * math.pi * R * C) / 10, 10 / (2 * math.pi * R * C))
            check(r, 'f_0', f0, 1e-8)
            check(r, 'A', 1 + 22e3 / 10e3)
        case('Oscillateur à pont de Wien', {'R': R, 'C': C, 'R_f': 22e3, 'R_g': 10e3}, chk)
    case('Oscillateurs Colpitts et Hartley', {'L': 1e-4, 'C_1': 1e-9, 'C_2': 1e-8},
         lambda r: check(r, 'f_0', 1 / (2 * math.pi * math.sqrt(1e-4 * 1e-9 * 1e-8 / 1.1e-8))))
    case('Oscillateurs Colpitts et Hartley', {'#Type': 1, 'L_1': 1e-5, 'L_2': 1e-4, 'M': 1e-6, 'C': 1e-9},
         lambda r: check(r, 'f_0', 1 / (2 * math.pi * math.sqrt(1.12e-4 * 1e-9))))


def main():
    build()
    assoc_cases()
    rlc_cases()
    filter_cases()
    aop_cases()
    bjt_cases()
    fet_cases()
    base_cases()
    alim_cases()
    code_cases()
    osc_cases()
    import verify_ext
    verify_ext.register(sys.modules[__name__])
    lines = []
    for title, inp, _, _ in cases:
        lines.append('\t'.join([title] + [('%s=%s' % (k, v)) if isinstance(v, str) else ('%s=%.17g' % (k, v))
                                           for k, v in inp.items()]))
    out = subprocess.run([BIN], input='\n'.join(lines) + '\n', capture_output=True, text=True).stdout
    blocks, cur = [], []
    for l in out.split('\n'):
        if l == 'END':
            blocks.append(cur)
            cur = []
        elif l:
            cur.append(l)
    if len(blocks) != len(cases):
        print('ERREUR : %d blocs pour %d cas' % (len(blocks), len(cases)))
        sys.exit(2)
    for (title, inp, chk, _), blk in zip(cases, blocks):
        CUR[0] = title
        res = Res(blk)
        for e in res.err:
            stats['fail'] += 1
            print('  ENTREE  %s : %s' % (title, e))
        chk(res)
    print('%d vérifications, %d écart(s), %d cas' % (stats['ok'] + stats['fail'], stats['fail'], len(cases)))
    sys.exit(1 if stats['fail'] else 0)


main()
