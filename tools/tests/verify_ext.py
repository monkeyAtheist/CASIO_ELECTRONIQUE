"""Verifications independantes des calculateurs ajoutes en v2 (appele par
verify_calcs.py) : petits signaux (analyse nodale modifiee avec sources de
tension nulles pour les courts-circuits), quadripoles (formules classiques de
passage), logique (tables de verite, recouvrements minimaux par recherche
exhaustive), timers, moteurs, RF, traitement du signal (scipy, FFT numpy,
integration numerique)."""
import cmath
import itertools
import math
import random

import numpy as np
from scipy import special

V = None   # module verify_calcs (case, check, check_msg, lu, par...)
C0 = 299792458.0


def register(mod):
    global V
    V = mod
    amp_cases()
    quad_cases()
    mag_cases()
    logic_cases()
    mcu_cases()
    motor_cases()
    rf_cases()
    sig_cases()


def chk(r, label, val, rel=1e-9, absol=1e-12):
    V.check(r, label, val, rel, absol)


# ==========================================================================
# Analyse nodale modifiee (MNA) complexe
# ==========================================================================
def mna(elems, w, mid, vsrcs, isrcs, probe):
    """elems : (genre, a, b, valeur[, ctrl]) ; genres R, CK (liaison), CD
    (capacite du transistor), GM (a -> b commande par ctrl = (p, n)).
    vsrcs : (a, b, V) avec V_a - V_b = V ; isrcs : (a, I) injecte en a.
    Noeud 'g' = masse. Renvoie les tensions des noeuds demandes."""
    nodes = []
    for e in elems:
        for n in (e[1], e[2]):
            if n != 'g' and n not in nodes:
                nodes.append(n)
        if e[0] == 'GM':
            for n in e[4]:
                if n != 'g' and n not in nodes:
                    nodes.append(n)
    for a, b, _ in vsrcs:
        for n in (a, b):
            if n != 'g' and n not in nodes:
                nodes.append(n)
    for a, _ in isrcs:
        if a != 'g' and a not in nodes:
            nodes.append(a)
    vs = list(vsrcs)
    for e in elems:
        if (e[0] == 'CK' and mid) or e[0] == 'S':
            vs.append((e[1], e[2], 0.0))
    n, m = len(nodes), len(vs)
    A = np.zeros((n + m, n + m), dtype=complex)
    z = np.zeros(n + m, dtype=complex)
    idx = {nd: i for i, nd in enumerate(nodes)}

    def stamp(a, b, y):
        ia, ib = idx.get(a), idx.get(b)
        if ia is not None:
            A[ia, ia] += y
        if ib is not None:
            A[ib, ib] += y
        if ia is not None and ib is not None:
            A[ia, ib] -= y
            A[ib, ia] -= y
    for e in elems:
        k = e[0]
        if k == 'R':
            stamp(e[1], e[2], 1 / e[3])
        elif k == 'CK':
            if not mid:
                stamp(e[1], e[2], 1j * w * e[3])
        elif k == 'CD':
            if not mid and e[3] > 0:
                stamp(e[1], e[2], 1j * w * e[3])
        elif k == 'GM':
            gm, (p, q) = e[3], e[4]
            for node, sg in ((e[1], 1), (e[2], -1)):
                i = idx.get(node)
                if i is None:
                    continue
                if p in idx:
                    A[i, idx[p]] += sg * gm
                if q in idx:
                    A[i, idx[q]] -= sg * gm
    for k, (a, b, v) in enumerate(vs):
        r = n + k
        if a in idx:
            A[idx[a], r] += 1
            A[r, idx[a]] += 1
        if b in idx:
            A[idx[b], r] -= 1
            A[r, idx[b]] -= 1
        z[r] = v
    for a, i in isrcs:
        if a in idx:
            z[idx[a]] += i
    x = np.linalg.solve(A, z)
    return {nd: (x[idx[nd]] if nd in idx else 0.0) for nd in probe}


def amp_net(t, p, with_rg=True, with_rl=True, skip=None):
    """Schema petits signaux independant (t : 'CE', 'CC', 'CB')."""
    E = []
    if with_rg:
        E.append(('R', 'src', 'in', p['rg']) if p['rg'] > 0 else ('S', 'src', 'in', 0))
    E.append(('CK', 'in', 'e' if t == 'CB' else 'b', p['ce'], 'ce'))
    if p['r1'] > 0:
        E.append(('R', 'b', 'g', p['r1']))
    E.append(('R', 'b', 'g', p['r2']))
    if t == 'CB':
        E.append(('CK', 'b', 'g', p['cb'], 'cb'))
    coll = 'g' if t == 'CC' else 'c'
    if t != 'CC':
        E.append(('R', 'c', 'g', p['rc']))
    if t == 'CE':
        if p['re1'] > 0:
            E.append(('R', 'e', 'x', p['re1']))
            xn = 'x'
        else:
            xn = 'e'
        E.append(('R', xn, 'g', p['re2']))
        if p['hasce']:
            E.append(('CK', xn, 'g', p['cE'], 'cE'))
    else:
        E.append(('R', 'e', 'g', p['re1']))
    E.append(('CK', 'e' if t == 'CC' else 'c', 'out', p['cs'], 'cs'))
    if with_rl:
        E.append(('R', 'out', 'g', p['rl']))
    if p['rpi'] > 0:
        E.append(('R', 'b', 'e', p['rpi']))
    if p['ro'] > 0:
        E.append(('R', coll, 'e', p['ro']))
    E.append(('CD', 'b', 'e', p['cpi']))
    E.append(('CD', 'b', coll, p['cmu']))
    E.append(('GM', coll, 'e', p['gm'], ('b', 'e')))
    if skip:
        E = [e for e in E if not (e[0] == 'CK' and e[4] == skip)]
    return E


def amp_perf(t, p):
    res = {}
    E = amp_net(t, p)
    v = mna(E, 0, True, [('src', 'g', 1.0)], [], ['in', 'out'])
    res['avg'] = v['out'].real
    res['av'] = (v['out'] / v['in']).real
    v = mna(amp_net(t, p, with_rg=False), 0, True, [], [('in', 1.0)], ['in'])
    res['ze'] = v['in'].real
    v = mna(amp_net(t, p, with_rl=False), 0, True, [('src', 'g', 0.0)], [('out', 1.0)], ['out'])
    res['zs'] = v['out'].real
    # resistance vue par chaque condensateur de liaison
    for name, a, b in (('ce', 'in', 'e' if t == 'CB' else 'b'), ('cs', 'e' if t == 'CC' else 'c', 'out'),
                       ('cE', 'x' if p['re1'] > 0 else 'e', 'g'), ('cb', 'b', 'g')):
        if name == 'cE' and not (t == 'CE' and p['hasce']):
            continue
        if name == 'cb' and t != 'CB':
            continue
        isr = [(a, 1.0)] + ([(b, -1.0)] if b != 'g' else [])
        v = mna(amp_net(t, p, skip=name), 0, True, [('src', 'g', 0.0)], isr, [a] + ([b] if b != 'g' else []))
        R = (v[a] - (v[b] if b != 'g' else 0)).real
        res['f_' + name] = 1 / (2 * math.pi * R * p[name])

    def mag(f):
        return abs(mna(E, 2 * math.pi * f, False, [('src', 'g', 1.0)], [], ['out'])['out'])
    tgt = abs(res['avg']) / math.sqrt(2)
    ls = np.arange(-3, 11.0001, 0.125)
    mags = [mag(10 ** l) for l in ls]
    fb = fh = None
    for i in range(1, len(ls)):
        a, b = mags[i - 1] >= tgt, mags[i] >= tgt
        if a != b:
            lo, hi = ls[i - 1], ls[i]
            for _ in range(60):
                md = (lo + hi) / 2
                if (mag(10 ** md) >= tgt) == a:
                    lo = md
                else:
                    hi = md
            f = 10 ** ((lo + hi) / 2)
            if fb is None and b:
                fb = f
            elif fb is not None and not b:
                fh = f
                break
    res['fb'], res['fh'] = fb, fh
    return res


def amp_cases():
    titles = {('CE', False): 'Émetteur commun : petits signaux', ('CC', False): 'Collecteur commun : petits signaux',
              ('CB', False): 'Base commune : petits signaux', ('CE', True): 'Source commune : petits signaux',
              ('CC', True): 'Drain commun : petits signaux', ('CB', True): 'Grille commune : petits signaux'}
    for (t, fet), title in titles.items():
        for it in range(6):
            p = {'rg': random.choice([0.0, V.lu(10, 2e3)]), 'rl': V.lu(1e3, 1e5), 'ce': V.lu(1e-7, 1e-4),
                 'cs': V.lu(1e-7, 1e-4), 'cE': V.lu(1e-6, 1e-3), 'cb': V.lu(1e-6, 1e-3)}
            p['hasce'] = random.random() < 0.7
            inp = {'R_g': p['rg'], 'R_L': p['rl'], 'C_e': p['ce'], 'C_s': p['cs']}
            if not fet:
                vcc = random.uniform(9, 24)
                beta = random.uniform(80, 400)
                vbe, vt = 0.65, 0.026
                va = random.choice([0.0, V.lu(30, 200)])
                cpi, cmu = random.choice([(0.0, 0.0), (V.lu(5e-12, 50e-12), V.lu(1e-12, 8e-12))])
                r1, r2 = V.lu(1e4, 1e5), V.lu(3e3, 3e4)
                rc = V.lu(500, 1e4)
                re1 = random.choice([0.0, V.lu(20, 500)]) if t == 'CE' else V.lu(300, 5e3)
                re2 = V.lu(200, 3e3) if t == 'CE' else 0.0
                rth, vth = r1 * r2 / (r1 + r2), vcc * r2 / (r1 + r2)
                red = re1 + re2
                ib = (vth - vbe) / (rth + (beta + 1) * red)
                ic = beta * ib if ib > 0 else 0.0
                rcd = 0.0 if t == 'CC' else rc
                vce = vcc - rcd * ic - red * (ic + ib) if ib > 0 else vcc
                inp.update({'V_{CC}': vcc, 'R_1': r1, 'R_2': r2, '\\beta': beta, 'V_A': va, 'C_\\pi': cpi, 'C_\\mu': cmu})
                if t != 'CC':
                    inp['R_C'] = rc
                if t == 'CE':
                    inp.update({'R_{E1}': re1, 'R_{E2}': re2, '?C_E': 0 if p['hasce'] else 1})
                    if p['hasce']:
                        inp['C_E'] = p['cE']
                else:
                    inp['R_E'] = re1
                if t == 'CB':
                    inp['C_B'] = p['cb']
                ok = ib > 0 and vce >= 0.2
                gm = ic / vt if ok else 0
                p.update({'r1': r1, 'r2': r2, 'rc': rc, 're1': re1, 're2': re2, 'gm': gm,
                          'rpi': beta / gm if ok else 0, 'ro': va / ic if (ok and va > 0) else 0, 'cpi': cpi, 'cmu': cmu})
                lab = {'i': 'I_C', 'v': 'V_{CE}', 'ro': 'r_o', 'fE': 'f_{C_E}', 'fB': 'f_{C_B}'}
            else:
                mos = random.random() < 0.5
                vdd = random.uniform(10, 20)
                lam = random.choice([0.0, V.lu(0.002, 0.05)])
                cpi, cmu = random.choice([(0.0, 0.0), (V.lu(2e-12, 20e-12), V.lu(0.5e-12, 5e-12))])
                rc = V.lu(500, 5e3)
                if mos:
                    k, vthr = V.lu(5e-3, 50e-3), random.uniform(1, 3)
                    r1, r2 = V.lu(1e6, 4e6), V.lu(5e5, 2e6)
                    vg = vdd * r2 / (r1 + r2)
                else:
                    idss, vp = V.lu(4e-3, 15e-3), -random.uniform(1, 4)
                    r1, r2 = 0.0, 1e6
                    vg = 0.0
                re1 = random.choice([0.0, V.lu(20, 200)]) if t == 'CE' else V.lu(200, 3e3)
                re2 = V.lu(200, 2e3) if t == 'CE' else 0.0
                rs = re1 + re2
                # point de repos : racines du polynome
                if mos:
                    u0 = vg - vthr
                    roots = np.roots([k * rs * rs, -(2 * k * rs * u0 + 1), k * u0 * u0]) if u0 > 0 else []
                    cand = [x.real for x in roots if abs(x.imag) < 1e-9 and x.real >= 0 and vg - rs * x.real - vthr >= 0]
                    idq = min(cand) if cand else 0.0
                    vgs = vg - rs * idq
                    gm = 2 * k * (vgs - vthr) if idq > 0 else 0
                    vov = vgs - vthr
                else:
                    # I = idss (1 + rs I / vp)^2 avec vgs = -rs I
                    a = idss * rs * rs / (vp * vp)
                    b = 2 * idss * rs / vp - 1
                    roots = np.roots([a, b, idss]) if rs > 0 else [idss]
                    cand = [x.real for x in np.atleast_1d(roots) if abs(np.imag(x)) < 1e-9 and x.real >= 0 and -rs * x.real >= vp]
                    idq = min(cand) if cand else 0.0
                    vgs = -rs * idq
                    gm = 2 * idss / abs(vp) * (1 - vgs / vp)
                    vov = vgs - vp
                rdd = 0.0 if t == 'CC' else rc
                vds = vdd - (rdd + rs) * idq
                ok = idq > 0 and vds >= vov
                inp.update({'V_{DD}': vdd, 'R_1': r1, 'R_2': r2, '\\lambda': lam, 'C_{gs}': cpi, 'C_{gd}': cmu,
                            '#Transistor': 1 if mos else 0})
                if mos:
                    inp.update({'K': k, 'V_{th}': vthr})
                else:
                    inp.update({'I_{DSS}': idss, 'V_P': vp})
                if t != 'CC':
                    inp['R_D'] = rc
                if t == 'CE':
                    inp.update({'R_{S1}': re1, 'R_{S2}': re2, '?C_S': 0 if p['hasce'] else 1})
                    if p['hasce']:
                        inp['C_S'] = p['cE']
                else:
                    inp['R_S'] = re1
                if t == 'CB':
                    inp['C_G'] = p['cb']
                ic = idq
                vce = vds
                p.update({'r1': r1, 'r2': r2, 'rc': rc, 're1': re1, 're2': re2, 'gm': gm if ok else 0, 'rpi': 0,
                          'ro': 1 / (lam * idq) if (ok and lam > 0) else 0, 'cpi': cpi, 'cmu': cmu})
                lab = {'i': 'I_D', 'v': 'V_{DS}', 'ro': 'r_{ds}', 'fE': 'f_{C_S}', 'fB': 'f_{C_G}'}
            if t != 'CE':
                p['hasce'] = False

            def c(r, t=t, p=dict(p), ic=ic, vce=vce, ok=ok, lab=lab, fet=fet):
                chk(r, lab['i'], ic, 1e-7, 1e-12)
                chk(r, lab['v'], vce, 1e-7, 1e-9)
                if not ok:
                    V.check_msg(r, 'Bloqué' if ic <= 0 else ('ohmique' if fet else 'Saturé'))
                    return
                chk(r, 'g_m', p['gm'], 1e-7)
                if not fet:
                    chk(r, 'r_\\pi', p['rpi'], 1e-7)
                if p['ro'] > 0:
                    chk(r, lab['ro'], p['ro'], 1e-7)
                q = amp_perf(t, p)
                chk(r, 'A_v', q['av'], 1e-7)
                chk(r, 'A_{vg}', q['avg'], 1e-7)
                chk(r, 'Z_e', q['ze'], 1e-7)
                chk(r, 'Z_s', q['zs'], 1e-7)
                chk(r, 'A_i', q['av'] * q['ze'] / p['rl'], 1e-7)
                chk(r, 'G_p', 10 * math.log10(abs(q['av'] * q['av'] * q['ze'] / p['rl'])), 1e-7)
                chk(r, 'f_{C_e}', q['f_ce'], 1e-7)
                chk(r, 'f_{C_s}', q['f_cs'], 1e-7)
                if 'f_cE' in q:
                    chk(r, lab['fE'], q['f_cE'], 1e-7)
                if 'f_cb' in q:
                    chk(r, lab['fB'], q['f_cb'], 1e-7)
                if q['fb'] is not None:
                    chk(r, 'f_b', q['fb'], 1e-6)
                if q['fh'] is not None:
                    chk(r, 'f_h', q['fh'], 1e-6)
                else:
                    chk(r, 'f_h', math.inf)
            V.case(title, inp, c)
    # paire differentielle
    for _ in range(6):
        vcc, vee, rc, ree, re = random.uniform(5, 15), random.uniform(5, 15), V.lu(1e3, 2e4), V.lu(2e3, 5e4), random.choice([0.0, V.lu(10, 300)])
        beta = random.uniform(80, 300)
        tail = random.randint(0, 1)
        i0, r0 = V.lu(1e-4, 2e-3), V.lu(1e5, 1e7)

        def c(r, vcc=vcc, vee=vee, rc=rc, ree=ree, re=re, beta=beta, tail=tail, i0=i0, r0=r0):
            # analyse nodale complete en petits signaux (modele en pi)
            ie = (vee - 0.65) / (2 * ree + re) if tail == 0 else i0 / 2
            rt = ree if tail == 0 else r0
            ic = beta / (beta + 1) * ie
            gm = ic / 0.026
            rpi = beta / gm
            chk(r, 'I_C', ic)
            chk(r, 'V_C', vcc - rc * ic)

            def solve(v1, v2):
                E = [('R', 'c1', 'g', rc), ('R', 'c2', 'g', rc), ('R', 'b1', 'e1', rpi), ('R', 'b2', 'e2', rpi),
                     ('GM', 'c1', 'e1', gm, ('b1', 'e1')), ('GM', 'c2', 'e2', gm, ('b2', 'e2')), ('R', 't', 'g', rt)]
                for k in (1, 2):
                    E.append(('R', 'e%d' % k, 't', re) if re > 0 else ('S', 'e%d' % k, 't', 0))
                return mna(E, 0, True, [('b1', 'g', v1), ('b2', 'g', v2)], [], ['c1', 'c2'])
            d = solve(0.5, -0.5)
            cm = solve(1.0, 1.0)
            chk(r, 'A_d', (d['c1'] - d['c2']).real, 1e-6)
            chk(r, 'A_{d1}', d['c1'].real, 1e-6)
            chk(r, 'A_{mc}', cm['c1'].real, 1e-6)
            chk(r, 'TRMC', 20 * math.log10(abs(d['c1'] / cm['c1'])), 1e-6)
        inp = {'V_{CC}': vcc, 'V_{EE}': vee, 'R_C': rc, 'R_E': re, '#Queue': tail, '\\beta': beta}
        if tail == 0:
            inp['R_{EE}'] = ree
        else:
            inp.update({'I_0': i0, 'R_0': r0})
        V.case('Paire différentielle', inp, c)
    for _ in range(4):
        ic, beta, va, ft, cmu = V.lu(1e-4, 1e-2), random.uniform(50, 400), V.lu(20, 200), V.lu(1e8, 1e9), V.lu(1e-12, 5e-12)

        def c(r, ic=ic, beta=beta, va=va, ft=ft, cmu=cmu):
            gm = ic / 0.026
            chk(r, 'g_m', gm)
            chk(r, 'r_\\pi', beta / gm)
            chk(r, 'r_o', va / ic)
            chk(r, 'C_\\pi', gm / (2 * math.pi * ft) - cmu)
            chk(r, 'f_\\beta', ft / beta)
            chk(r, 'h_{22e}', ic / va)
        V.case('Paramètres hybrides du bipolaire', {'I_C': ic, '\\beta': beta, 'V_A': va, 'f_T': ft, 'C_\\mu': cmu}, c)


# ==========================================================================
# Quadripoles : formules classiques de passage
# ==========================================================================
def z_to(Z, t, z0=50.0):
    z11, z12, z21, z22 = Z[0, 0], Z[0, 1], Z[1, 0], Z[1, 1]
    dz = z11 * z22 - z12 * z21
    if t == 'Z':
        return Z
    if t == 'Y':
        return np.linalg.inv(Z)
    if t == 'H':
        return np.array([[dz / z22, z12 / z22], [-z21 / z22, 1 / z22]])
    if t == 'G':
        return np.array([[1 / z11, -z12 / z11], [z21 / z11, dz / z11]])
    if t == 'T':
        return np.array([[z11 / z21, dz / z21], [1 / z21, z22 / z21]])
    if t == 'S':
        I = np.eye(2)
        return (Z / z0 - I) @ np.linalg.inv(Z / z0 + I)


def to_z(P, t, z0=50.0):
    p11, p12, p21, p22 = P[0, 0], P[0, 1], P[1, 0], P[1, 1]
    dp = p11 * p22 - p12 * p21
    if t == 'Z':
        return P
    if t == 'Y':
        return np.linalg.inv(P)
    if t == 'H':
        return np.array([[dp / p22, p12 / p22], [-p21 / p22, 1 / p22]])
    if t == 'G':
        return np.array([[1 / p11, -p12 / p11], [p21 / p11, dp / p11]])
    if t == 'T':
        return np.array([[p11 / p21, dp / p21], [1 / p21, p22 / p21]])
    if t == 'S':
        I = np.eye(2)
        return z0 * (I + P) @ np.linalg.inv(I - P)


NAMES = {'Z': ['Z_{11}', 'Z_{12}', 'Z_{21}', 'Z_{22}'], 'Y': ['Y_{11}', 'Y_{12}', 'Y_{21}', 'Y_{22}'],
         'H': ['h_{11}', 'h_{12}', 'h_{21}', 'h_{22}'], 'G': ['g_{11}', 'g_{12}', 'g_{21}', 'g_{22}'],
         'T': ['A', 'B', 'C', 'D'], 'S': ['S_{11}', 'S_{12}', 'S_{21}', 'S_{22}']}
TYPES = ['Z', 'Y', 'H', 'G', 'T', 'S']


def chkc(r, label, z, rel=1e-8):
    tol = rel * max(abs(z), 1e-300)
    V.check(r, label, z.real, 0, tol)
    V.check(r, label + '.im', z.imag, 0, tol)


def loaded_check(r, Z, rg, rl):
    T = z_to(Z, 'T')
    A, B, C, D = T[0, 0], T[0, 1], T[1, 0], T[1, 1]
    chkc(r, 'Z_e', (A * rl + B) / (C * rl + D), 1e-7)
    chkc(r, 'Z_s', (D * rg + B) / (C * rg + A), 1e-7)
    chkc(r, 'A_v', rl / (A * rl + B), 1e-7)
    chkc(r, 'A_{vg}', rl / (A * rl + B + rg * (C * rl + D)), 1e-7)


def quad_cases():
    for it in range(24):
        ts, td = random.randrange(6), random.randrange(6)
        cplx = it % 2
        # quadripole aleatoire (Z quelconque, eventuellement actif)
        Z = np.array([[V.lu(10, 1e3), V.lu(10, 500)], [V.lu(10, 500) * random.choice([1, -3]), V.lu(10, 1e3)]], dtype=complex)
        if cplx:
            Z = Z + 1j * np.array([[random.uniform(-300, 300) for _ in range(2)] for _ in range(2)])
        z0 = V.lu(20, 300)
        P = z_to(Z, TYPES[ts], z0)
        inp = {'#Données': ts, '#Valeurs': cplx, '#Afficher': td, 'R_g': V.lu(1, 1e3), 'R_L': V.lu(10, 1e4)}
        if ts == 5 or td == 5:
            inp['Z_0'] = z0
        for k in range(4):
            if cplx:
                inp['\\text{Re}\\,' + NAMES[TYPES[ts]][k]] = P.flat[k].real
                inp['\\text{Im}\\,' + NAMES[TYPES[ts]][k]] = P.flat[k].imag
            else:
                inp[NAMES[TYPES[ts]][k]] = P.flat[k].real

        def c(r, Z=Z, td=td, z0=z0, rg=inp['R_g'], rl=inp['R_L'], cplx=cplx, P=P, ts=ts):
            Zr = to_z(P if cplx else P.real.astype(complex), TYPES[ts], z0)
            Q = z_to(Zr, TYPES[td], z0)
            for k in range(4):
                chkc(r, NAMES[TYPES[td]][k], Q.flat[k], 1e-7)
            loaded_check(r, Zr, rg, rl)
        V.case('Conversion des matrices Z, Y, H, G, T, S', inp, c)
    # cellules T, Pi, L
    for it in range(12):
        topo = random.randrange(4)
        f = V.lu(10, 1e6)
        w = 2 * math.pi * f
        zs = []
        inp = {'#Cellule': topo, 'f': f, '#Afficher': random.randrange(5), 'R_g': V.lu(1, 1e3), 'R_L': V.lu(100, 1e5)}
        for k in range(3):
            tp = random.randrange(7)
            R, Cc, L = V.lu(10, 1e4), V.lu(1e-9, 1e-5), V.lu(1e-5, 1e-1)
            inp['?Z_%d' % (k + 1)] = tp
            inp['R_%d' % (k + 1)] = R
            inp['C_%d' % (k + 1)] = Cc
            inp['L_%d' % (k + 1)] = L
            zs.append(V.ztype(tp, R, Cc, L, w))
        # supprimer les entrees inutilisees selon le type
        for k in range(3):
            tp = inp['?Z_%d' % (k + 1)]
            if tp not in (0, 3, 4, 5, 6):
                del inp['R_%d' % (k + 1)]
            if tp not in (1, 3, 4):
                del inp['C_%d' % (k + 1)]
            if tp not in (2, 5, 6):
                del inp['L_%d' % (k + 1)]
        if topo >= 2:
            for key in ('?Z_3', 'R_3', 'C_3', 'L_3'):
                inp.pop(key, None)

        def c(r, topo=topo, zs=zs, td=inp['#Afficher'], rg=inp['R_g'], rl=inp['R_L']):
            z1, z2, z3 = zs
            if topo == 0:
                Z = np.array([[z1 + z2, z2], [z2, z2 + z3]])
            elif topo == 1:
                y1, y2, y3 = 1 / z1, 1 / z2, 1 / z3
                Z = np.linalg.inv(np.array([[y1 + y2, -y2], [-y2, y2 + y3]]))
            elif topo == 2:
                Z = np.array([[z1 + z2, z2], [z2, z2]])
            else:
                y1 = 1 / z1
                Z = np.linalg.inv(np.array([[y1 + 1 / z2, -1 / z2], [-1 / z2, 1 / z2]])) if True else None
            Q = z_to(Z, TYPES[td])
            for k in range(4):
                chkc(r, NAMES[TYPES[td]][k], Q.flat[k], 1e-6)
            loaded_check(r, Z, rg, rl)
        if topo == 3:
            # L inverse : Z non definie si Z2 serie seule ... on verifie via T
            def c(r, zs=zs, td=inp['#Afficher'], rg=inp['R_g'], rl=inp['R_L']):
                z1, z2 = zs[0], zs[1]
                T = np.array([[1, 0], [1 / z1, 1]]) @ np.array([[1, z2], [0, 1]])
                A, B, C, D = T.flat
                chkc(r, 'Z_e', (A * rl + B) / (C * rl + D), 1e-7)
                chkc(r, 'A_v', rl / (A * rl + B), 1e-7)
        V.case('Quadripôles en T, en Π et en L', inp, c)
    # attenuateurs
    for it in range(10):
        pi, diff = random.randint(0, 1), random.randint(0, 1)
        a = random.uniform(3, 40)
        z1, z2 = V.lu(25, 600), V.lu(25, 600)
        ser = random.randint(0, 1)

        def c(r, pi=pi, diff=diff, a=a, z1=z1, z2=z2, ser=ser):
            zz2 = z2 if diff else z1
            K = 10 ** (a / 20)
            amin = 20 * math.log10(math.sqrt(max(z1, zz2) / min(z1, zz2)) + math.sqrt(max(z1, zz2) / min(z1, zz2) - 1))
            if a < amin - 1e-9:
                V.check_msg(r, 'trop faible')
                return
            r3 = 2 * K * math.sqrt(z1 * zz2) / (K * K - 1)
            r1 = z1 * (K * K + 1) / (K * K - 1) - r3
            r2 = zz2 * (K * K + 1) / (K * K - 1) - r3
            if pi:
                sp = r1 * r2 + r2 * r3 + r3 * r1
                r1, r2, r3 = sp / r2, sp / r1, sp / r3
            chk(r, 'R_1', r1, 1e-9)
            chk(r, 'R_3', r3, 1e-9)
            # valeurs normalisees puis attenuation par analyse nodale
            n1, n2, n3 = (V.enear(V.E24 if ser == 0 else V.E96, x) for x in (r1, r2, r3))
            if not pi:
                E = [('R', '1', 'm', n1), ('R', 'm', '2', n2), ('R', 'm', 'g', n3), ('R', '2', 'g', zz2)]
            else:
                E = [('R', '1', 'g', n1), ('R', '1', '2', n3), ('R', '2', 'g', n2), ('R', '2', 'g', zz2)]
            v = mna(E, 0, True, [], [('1', 1.0)], ['1', '2'])
            p1, p2 = v['1'].real, abs(v['2']) ** 2 / zz2
            chk(r, 'Z_e', v['1'].real, 1e-9)
            chk(r, 'A', 10 * math.log10(p1 / p2), 1e-9)
        inp = {'#Montage': pi, '#Impédances': diff, 'A': a, '#Arrondi': ser}
        inp['Z_1' if diff else 'Z_0'] = z1
        if diff:
            inp['Z_2'] = z2
        V.case('Atténuateurs en T et en Π', inp, c)


# ==========================================================================
# Transformateurs et bobines
# ==========================================================================
MU0 = 1.25663706212e-6


def mag_cases():
    for _ in range(4):
        n1, n2, v1, rl = random.randint(50, 2000), random.randint(5, 500), V.lu(5, 400), V.lu(1, 1e3)

        def c(r, n1=n1, n2=n2, v1=v1, rl=rl):
            m = n2 / n1
            chk(r, 'V_2', m * v1)
            chk(r, 'I_1', m * m * v1 / rl)
            chk(r, "R_L'", rl / (m * m))
        V.case('Transformateur idéal', {'N_1': n1, 'N_2': n2, 'V_1': v1, 'R_L': rl}, c)
    for _ in range(6):
        v1, f, m, rs, ls, rf, lmu = V.lu(10, 400), random.choice([50.0, 60.0, 400.0]), V.lu(0.02, 2), V.lu(0.01, 5), V.lu(1e-5, 1e-2), V.lu(1e3, 1e5), V.lu(1, 100)
        tp, R, Cc, L = random.choice([0, 3, 5, 6]), V.lu(1, 100), V.lu(1e-5, 1e-3), V.lu(1e-3, 0.1)

        def c(r, v1=v1, f=f, m=m, rs=rs, ls=ls, rf=rf, lmu=lmu, tp=tp, R=R, Cc=Cc, L=L):
            w = 2 * math.pi * f
            zl = V.ztype(tp, R, Cc, L, w)
            zs = rs + 1j * w * ls
            i2 = m * v1 / (zs + zl)
            p2 = (abs(i2) ** 2) * zl.real
            pj = rs * abs(i2) ** 2
            pf = v1 * v1 / rf
            chk(r, 'V_2', abs(zl * i2))
            chk(r, 'I_2', abs(i2))
            chk(r, '\\eta', p2 / (p2 + pj + pf) * 100)
            i1 = m * i2 + v1 / rf + v1 / (1j * w * lmu)
            chk(r, 'I_1', abs(i1))
            chk(r, '\\cos\\varphi_1', i1.real / abs(i1))
        inp = {'V_1': v1, 'f': f, 'm': m, 'R_s': rs, 'L_s': ls, 'R_f': rf, 'L_\\mu': lmu, '?Z_L': tp, 'R_L': R}
        if tp in (3, 4):
            inp['C_L'] = Cc
        if tp in (5, 6):
            inp['L_L'] = L
        V.case('Transformateur réel (modèle de Kapp)', inp, c)
    for _ in range(3):
        v1, v2, S, f, B, ae, J = V.lu(100, 400), V.lu(5, 50), V.lu(5, 500), 50.0, random.uniform(1, 1.4), V.lu(2, 30), random.uniform(2, 4)

        def c(r, v1=v1, v2=v2, S=S, f=f, B=B, ae=ae, J=J):
            nv = 1 / (4.44 * f * B * ae * 1e-4)
            chk(r, 'N_1', math.ceil(v1 * nv - 1e-9))
            chk(r, 'N_2', math.ceil(v2 * nv * 1.08 - 1e-9))
            i2 = S / v2
            chk(r, 'd_2', math.sqrt(4 * i2 / J / math.pi) * 1e-3)
        V.case('Dimensionnement d\'un transformateur', {'V_1': v1, 'V_2': v2, 'S': S, 'f': f, 'B_{\\max}': B, 'A_e': ae, 'J': J}, c)
    for _ in range(5):
        l1, l2, k = V.lu(1e-6, 1e-1), V.lu(1e-6, 1e-1), random.uniform(0.05, 0.999)
        i1, i2 = random.uniform(-1, 1), random.uniform(-1, 1)

        def c(r, l1=l1, l2=l2, k=k, i1=i1, i2=i2):
            m = k * math.sqrt(l1 * l2)
            chk(r, 'M', m)
            chk(r, 'L_{s+}', l1 + l2 + 2 * m)
            chk(r, 'L_{p-}', (l1 * l2 - m * m) / (l1 + l2 + 2 * m))
            chk(r, 'L_{p+}', (l1 * l2 - m * m) / (l1 + l2 - 2 * m))
            # energie : integrale de v i dt en montant les courants ensemble
            chk(r, 'W', 0.5 * l1 * i1 * i1 + 0.5 * l2 * i2 * i2 + m * i1 * i2, 1e-9, 1e-15)
        V.case('Inductances couplées', {'L_1': l1, 'L_2': l2, 'k': k, 'I_1': i1, 'I_2': i2}, c)
    for _ in range(5):
        al, n, f, cp, R = V.lu(1e-7, 1e-5), random.randint(2, 40), V.lu(1e4, 3e7), random.choice([0.0, V.lu(1e-12, 50e-12)]), random.choice([50.0, 25.0, 100.0])

        def c(r, al=al, n=n, f=f, cp=cp, R=R):
            L = al * n * n
            w = 2 * math.pi * f
            z = 1j * w * L
            if cp > 0:
                z = 1 / (1 / z + 1j * w * cp)
            chk(r, 'L_{mc}', L)
            chk(r, '|Z_{mc}|', abs(z))
            chk(r, 'IL', 20 * math.log10(abs(1 + z / (2 * R))))
        V.case('Inductance de mode commun', {'A_L': al, 'N': n, 'f': f, 'C_p': cp, 'R': R}, c)
    for _ in range(6):
        t, goal = random.randrange(3), random.randrange(2)
        n, L = random.randint(5, 200), V.lu(1e-6, 1e-2)
        d, ln, mur, h, de, di, al = V.lu(5e-3, 5e-2), V.lu(5e-3, 0.2), V.lu(10, 5000), V.lu(3e-3, 2e-2), 30e-3, V.lu(5e-3, 25e-3), V.lu(1e-8, 1e-5)

        def c(r, t=t, goal=goal, n=n, L=L, d=d, ln=ln, mur=mur, h=h, de=de, di=di, al=al):
            if t == 0:
                k = MU0 * math.pi * d * d / 4 / (ln + 0.45 * d)
            elif t == 1:
                k = MU0 * mur * h * math.log(de / di) / (2 * math.pi)
            else:
                k = al
            nn = n if goal == 0 else math.ceil(math.sqrt(L / k) - 1e-9)
            chk(r, 'L', k * nn * nn)
            if goal == 1:
                chk(r, 'N', nn)
        inp = {'#Bobine': t, '#Calculer': goal}
        inp['N' if goal == 0 else 'L'] = n if goal == 0 else L
        if t == 0:
            inp.update({'d': d, '\\ell': ln})
        elif t == 1:
            inp.update({'\\mu_r': mur, 'h': h, 'D_e': de, 'D_i': di})
        else:
            inp['A_L'] = al
        V.case('Inductance d\'une bobine', inp, c)


# ==========================================================================
# Logique : expressions, recouvrements minimaux
# ==========================================================================
def py_eval(expr, a, b, c, d):
    """Evaluation independante (traduction en Python)."""
    s = expr
    out = ''
    i = 0
    toks = []
    for ch in s:
        toks.append(ch)
    # analyse descendante simple
    pos = [0]

    def peek():
        return toks[pos[0]] if pos[0] < len(toks) else ''

    def eat():
        pos[0] += 1
        return toks[pos[0] - 1]
    vals = {'A': a, 'B': b, 'C': c, 'D': d}

    def prim():
        ch = eat()
        if ch in vals:
            return vals[ch]
        if ch in '01':
            return int(ch)
        if ch == '(':
            v = expr_()
            eat()
            return v
        raise ValueError(ch)

    def unary():
        if peek() == '!':
            eat()
            return 1 - unary()
        return prim()

    def and_():
        v = unary()
        while True:
            if peek() == '.':
                eat()
                v &= unary()
            elif peek() and (peek() in 'ABCD01(!'):
                v &= unary()
            else:
                return v

    def xor_():
        v = and_()
        while peek() == '^':
            eat()
            v ^= and_()
        return v

    def or_():
        v = xor_()
        while peek() == '+':
            eat()
            v |= xor_()
        return v

    def expr_():
        v = or_()
        while peek() in ('@', '%') and peek():
            op = eat()
            w = or_()
            v = 1 - (v & w) if op == '@' else 1 - (v | w)
        return v
    return expr_()


def parse_tex_sop(tex, n):
    """'\\overline{A}B+C\\,\\overline{D}' -> liste de cubes (dict var -> 0/1)."""
    tex = tex.replace('\\,', '')
    if tex == '0':
        return []
    terms = []
    for t in tex.split('+'):
        cube = {}
        i = 0
        while i < len(t):
            if t.startswith('\\overline{', i):
                cube[t[i + 10]] = 0
                i += 12
            elif t[i] in 'ABCD':
                cube[t[i]] = 1
                i += 1
            elif t[i] == '1':
                i += 1
            else:
                raise ValueError(t)
        terms.append(cube)
    return terms


def parse_tex_pos(tex):
    tex = tex.replace('\\,', '')
    if tex == '1':
        return []
    if tex == '0':
        return [{}]
    # facteurs : groupes entre parentheses, ou une seule somme sans parentheses
    if '(' not in tex:
        bodies = [tex] if '+' in tex else []
        if not bodies:
            # produit de litteraux isoles
            i = 0
            while i < len(tex):
                if tex.startswith('\\overline{', i):
                    bodies.append(tex[i:i + 12])
                    i += 12
                else:
                    bodies.append(tex[i])
                    i += 1
    else:
        bodies = []
        i = 0
        while i < len(tex):
            if tex[i] == '(':
                j = tex.index(')', i)
                bodies.append(tex[i + 1:j])
                i = j + 1
            elif tex.startswith('\\overline{', i):
                bodies.append(tex[i:i + 12])
                i += 12
            else:
                bodies.append(tex[i])
                i += 1
    facs = []
    for body in bodies:
        fac = {}
        for lit in body.split('+'):
            if lit.startswith('\\overline{'):
                fac[lit[10]] = 0
            else:
                fac[lit] = 1
        facs.append(fac)
    return facs


def min_cost(n, on, dc, zeros=False):
    """Cout minimal (termes, litteraux) : impliquants premiers (enumeration de
    tous les cubes) puis recherche arborescente exhaustive avec elagage."""
    N = 1 << n
    cubes = []
    for vals in itertools.product((0, 1, None), repeat=n):
        cov = frozenset(m for m in range(N)
                        if all(v is None or ((m >> (n - 1 - k)) & 1) == v for k, v in enumerate(vals)))
        if cov <= (on | dc) and cov & on:
            cubes.append((cov, sum(v is not None for v in vals)))
    primes = [c for c in cubes if not any(c[0] < d[0] for d in cubes)]
    best = [None]

    def rec(need, nt, nl):
        if not need:
            if best[0] is None or (nt, nl) < best[0]:
                best[0] = (nt, nl)
            return
        if best[0] is not None and (nt + 1, nl) > best[0]:
            return
        m = min(need)
        for cov, l in primes:
            if m in cov:
                rec(need - cov, nt + 1, nl + l)
    rec(frozenset(on), 0, 0)
    return best[0] or (0, 0)


def logic_cases():
    VN = 'ABCD'
    exprs = ['!(A.B)+C', 'A^B^C', 'AB+!AC+BC', '!(A+B)@C', '(A+B)(!A+C)', 'A!B+!AB', 'ABCD+!A!B!C!D', '!(A%B)',
             'A+!A', '0', 'A.!A', '(A^B)(C^D)', '!(!(A.B).!(C.D))', 'AB+CD+!A!C', 'A^1', 'B!C+!BD+A!D']
    for e in exprs + [None] * 6:
        if e is None:
            # expression aleatoire
            ops = ['+', '.', '^', '']
            parts = []
            for _ in range(random.randint(2, 5)):
                lit = random.choice(VN[:random.randint(2, 4)])
                if random.random() < 0.4:
                    lit = '!' + lit
                parts.append(lit)
            e = parts[0]
            for p_ in parts[1:]:
                e += random.choice(ops) + p_
            if random.random() < 0.3:
                e = '!(' + e + ')'
        g_ = random.choice(['', e, '!!' + e, 'A+B'])

        def c(r, e=e, g_=g_):
            used = [ch for ch in 'ABCD' if ch in e or ch in g_]
            n = max(2, ('ABCD'.index(max(used)) + 1) if used else 2)
            on = set()
            for m in range(1 << n):
                bits = [(m >> (n - 1 - k)) & 1 for k in range(n)] + [0] * (4 - n)
                if py_eval(e, *bits):
                    on.add(m)
            # liste des mintermes
            sm = 'F = Σm(' + ','.join(str(m) for m in sorted(on)) + ')'
            V.check_msg(r, sm)
            # forme minimale : equivalence et minimalite
            sop = [v for k, v in r.msg.items() if k == 1][0]
            pos = [v for k, v in r.msg.items() if k == 2][0]
            cubes = parse_tex_sop(sop, n)
            got = set(m for m in range(1 << n) if any(all(((m >> (n - 1 - 'ABCD'.index(var))) & 1) == val for var, val in cb.items()) for cb in cubes))
            V.check(r, 'SOP ok', 1.0 if got == on else 0.0, 0, 0) if False else None
            ok = got == on
            V.stats['ok' if ok else 'fail'] += 1
            if not ok:
                print('  ECART   %s : SOP %s pour %s' % (V.CUR[0], sop, e))
            cost = (len(cubes), sum(len(cb) for cb in cubes)) if on else (0, 0)
            best = min_cost(n, on, set())
            okc = cost == best or (not on)
            V.stats['ok' if okc else 'fail'] += 1
            if not okc:
                print('  ECART   %s : cout %s, minimum %s (%s)' % (V.CUR[0], cost, best, e))
            facs = parse_tex_pos(pos)
            gotp = set(m for m in range(1 << n) if all(any(((m >> (n - 1 - 'ABCD'.index(var))) & 1) == val for var, val in fc.items()) for fc in facs))
            okp = gotp == on
            V.stats['ok' if okp else 'fail'] += 1
            if not okp:
                print('  ECART   %s : POS %s pour %s' % (V.CUR[0], pos, e))
            if g_:
                ong = set()
                for m in range(1 << n):
                    bits = [(m >> (n - 1 - k)) & 1 for k in range(n)] + [0] * (4 - n)
                    if py_eval(g_, *bits):
                        ong.add(m)
                V.check_msg(r, 'F ≡ G' if ong == on else 'F ≠ G')
        V.case('Table de vérité et équivalence', {'$0': e, '$1': g_}, c)
    # Karnaugh avec indifferents
    for it in range(30):
        n = random.choice([2, 3, 4, 4])
        vals = [random.choice([0, 0, 1, 1, 2]) for _ in range(1 << n)]
        inp = {'#Variables': n - 2}
        for m, v in enumerate(vals):
            inp['?m_{%d}' % m] = v

        def c(r, n=n, vals=vals):
            on = set(m for m, v in enumerate(vals) if v == 1)
            dc = set(m for m, v in enumerate(vals) if v == 2)
            off = set(range(1 << n)) - on - dc
            sop, pos = r.msg[0], r.msg[1]
            cubes = parse_tex_sop(sop, n)
            got = set(m for m in range(1 << n) if any(all(((m >> (n - 1 - 'ABCD'.index(var))) & 1) == val for var, val in cb.items()) for cb in cubes))
            ok = on <= got and not (got & off)
            cost = (len(cubes), sum(len(cb) for cb in cubes)) if on else (0, 0)
            best = min_cost(n, on, dc) if on else (0, 0)
            facs = parse_tex_pos(pos)
            gotp = set(m for m in range(1 << n) if all(any(((m >> (n - 1 - 'ABCD'.index(var))) & 1) == val for var, val in fc.items()) for fc in facs))
            okp = on <= gotp and not (gotp & off)
            costp = (len(facs), sum(len(fc) for fc in facs)) if off else (0, 0)
            bestp = min_cost(n, off, dc) if off else (0, 0)
            for good, what in ((ok, 'SOP'), (cost == best, 'cout SOP %s/%s' % (cost, best)), (okp, 'POS'),
                               (costp == bestp, 'cout POS %s/%s' % (costp, bestp))):
                V.stats['ok' if good else 'fail'] += 1
                if not good:
                    print('  ECART   %s : %s %s / %s vals=%s' % (V.CUR[0], what, sop, pos, vals))
        V.case('Tableau de Karnaugh', inp, c)
    # portes logiques : toutes les realisations, toutes les entrees
    fn = [lambda a, b: 1 - a, lambda a, b: a & b, lambda a, b: a | b, lambda a, b: 1 - (a & b), lambda a, b: 1 - (a | b),
          lambda a, b: a ^ b, lambda a, b: 1 - (a ^ b)]
    for gte in range(7):
        for real in range(3):
            for a in (0, 1):
                for b in (0, 1):
                    if gte == 0 and b:
                        continue
                    inp = {'#Porte': gte, '#Réalisation': real, '#Entrée A': a}
                    if gte:
                        inp['#Entrée B'] = b
                    V.case('Portes logiques et équivalences', inp, lambda r, v=fn[gte](a, b): V.check_msg(r, 'Sortie S = %d' % v))
    # bases
    for _ in range(12):
        nb = random.choice([4, 8, 12, 16, 24, 32])
        sg = random.randint(0, 1)
        lo, hi = (-(1 << (nb - 1)), (1 << (nb - 1)) - 1) if sg else (0, (1 << nb) - 1)
        N = random.randint(lo, hi)

        def c(r, nb=nb, sg=sg, N=N):
            p = N & ((1 << nb) - 1)
            bins = format(p, '0%db' % nb)
            groups = []
            while bins:
                groups.insert(0, bins[-4:])
                bins = bins[:-4]
            V.check_msg(r, '\\text{' + '}\\;\\text{'.join(groups) + '}')
            V.check_msg(r, '\\text{0x' + format(p, '0%dX' % ((nb + 3) // 4)) + '}')
            g = p ^ (p >> 1)
            gb = format(g, '0%db' % nb)
            groups = []
            while gb:
                groups.insert(0, gb[-4:])
                gb = gb[:-4]
            V.check_msg(r, '\\text{' + '}\\;\\text{'.join(groups) + '}')
            chk(r, 'N_u', p)
            chk(r, 'N_s', p - (1 << nb) if p >> (nb - 1) else p)
        V.case('Bases et codes binaires', {'N': N, 'n': nb, '#Codage': sg}, c)


# ==========================================================================
# Timers et microcontroleurs
# ==========================================================================
def mcu_cases():
    for _ in range(10):
        fclk = random.choice([1e6, 8e6, 16e6, 20e6, 48e6, 72e6])
        d = [random.choice([1, 2, 4, 8, 64]) for _ in range(3)]
        n = random.choice([8, 10, 16, 32])
        cmp = random.randint(0, 1)
        off = random.randint(0, (1 << n) - 1)
        top = random.randint(off, (1 << n) - 1)
        k = random.randint(1, 10)

        def c(r, fclk=fclk, d=d, n=n, cmp=cmp, off=off, top=top, k=k):
            ft = fclk / (d[0] * d[1] * d[2])
            ticks = (top - off + 1) if cmp else (2 ** n - off)
            chk(r, 'f_t', ft)
            chk(r, '#ticks', ticks)
            chk(r, 'T', ticks / ft)
            chk(r, 'k\\,T', k * ticks / ft)
        inp = {'f_{clk}': fclk, '\\div_1': d[0], '\\div_2': d[1], '\\div_3': d[2], 'n': n, '#Mode': cmp, '#offset': off, 'k': k}
        if cmp:
            inp['#plafond'] = top
        V.case('Timer : délai entre interruptions', inp, c)
    # exemple de l'utilisateur
    V.case('Timer : délai entre interruptions', {'f_{clk}': 16e6, '\\div_1': 2, '\\div_2': 4, '\\div_3': 1, 'n': 8, '#offset': 57},
           lambda r: (chk(r, '#ticks', 199), chk(r, 'T', 99.5e-6)))
    fams = {0: [1, 8, 64, 256, 1024], 1: [1, 8, 32, 64, 128, 256, 1024], 2: [1 << i for i in range(9)], 4: [1 << i for i in range(11)]}
    for _ in range(14):
        fclk = random.choice([1e6, 8e6, 16e6, 72e6, 84e6])
        T = V.lu(1e-6, 2.0)
        n = random.choice([8, 16])
        fam = random.choice([0, 1, 2, 3, 4])
        cmp = random.randint(0, 1)

        def c(r, fclk=fclk, T=T, n=n, fam=fam, cmp=cmp):
            full = 2 ** n
            Nt = T * fclk
            k = 1
            if fam == 3:
                pmin = max(1, math.ceil(Nt / full - 1e-12))
                if pmin > 65536:
                    k = math.ceil(Nt / (65536 * full) - 1e-12)
                    pmin = min(65536, math.ceil(Nt / k / full - 1e-12))
                best = None
                for q in range(pmin, min(65536, pmin + 2000) + 1):
                    t = math.floor(Nt / k / q + 0.5)
                    if 1 <= t <= full:
                        e = abs(t * q - Nt / k)
                        if best is None or e < best[0] - 1e-9 * Nt / k:
                            best = (e, q, t)
                        if e <= 1e-9 * Nt / k:
                            break
                p, ticks = best[1], best[2]
            else:
                lst = fams[fam]
                p = ticks = None
                for q in lst:
                    t = math.floor(Nt / q + 0.5)
                    if 1 <= t <= full:
                        p, ticks = q, t
                        break
                if p is None:
                    p = lst[-1]
                    if Nt / p < 1:
                        ticks = 1
                    else:
                        k = math.ceil(Nt / (p * full) - 1e-12)
                        ticks = math.floor(Nt / k / p + 0.5)
            ticks = min(max(ticks, 1), full)
            chk(r, 'N', p)
            chk(r, '#ticks', ticks)
            chk(r, 'k', k)
            reg = ticks - 1 if cmp else full - ticks
            lab = ('ARR' if fam == 3 else '#plafond') if cmp else '#offset'
            chk(r, lab, reg)
            tr = ticks * p / fclk
            chk(r, '\\varepsilon', (tr * k - T) / T * 100, 1e-9, 1e-9)
        V.case('Timer : réglage d\'un délai', {'f_{clk}': fclk, 'T': T, 'n': n, '#Prédiviseurs': fam, '#Mode': cmp}, c)
    for _ in range(6):
        fclk, p, top, duty = random.choice([8e6, 16e6]), random.choice([1, 8, 64]), random.choice([99, 255, 1023, 39999]), random.uniform(0, 100)
        mode = random.randint(0, 1)

        def c(r, fclk=fclk, p=p, top=top, duty=duty, mode=mode):
            if mode == 0:
                f = fclk / (p * (top + 1))
                ocr = min(max(math.floor(duty / 100 * (top + 1) + 0.5) - 1, -1), top)
                d = (ocr + 1) / (top + 1) * 100
            else:
                f = fclk / (2 * p * top)
                ocr = min(max(math.floor(duty / 100 * top + 0.5), 0), top)
                d = ocr / top * 100
            chk(r, 'f_{PWM}', f)
            chk(r, '#OCR', max(ocr, 0))
            chk(r, '\\alpha', d, 1e-9, 1e-9)
        V.case('PWM', {'f_{clk}': fclk, 'N': p, '#Mode': mode, '#TOP': top, '\\alpha': duty}, c)
    for _ in range(6):
        fclk, baud, ovs = random.choice([1e6, 8e6, 11.0592e6, 16e6, 20e6]), random.choice([2400, 9600, 19200, 57600, 115200]), random.randint(0, 1)

        def c(r, fclk=fclk, baud=baud, ovs=ovs):
            k = 8 if ovs else 16
            u = max(0, math.floor(fclk / (k * baud) - 1 + 0.5))
            br = fclk / (k * (u + 1))
            chk(r, '#UBRR', u)
            chk(r, '\\varepsilon', (br - baud) / baud * 100, 1e-9, 1e-12)
        V.case('Liaison série (UART)', {'f_{clk}': fclk, '#débit': baud, '#Suréchantillonnage': ovs}, c)


# ==========================================================================
# Optocoupleurs et moteurs
# ==========================================================================
def motor_cases():
    for _ in range(5):
        vin, vf, i_f, ctr, vcc, k = random.choice([3.3, 5.0, 12.0]), random.uniform(1.1, 1.4), V.lu(2e-3, 20e-3), random.uniform(20, 300), random.choice([3.3, 5.0, 12.0]), random.uniform(1.5, 3)

        def c(r, vin=vin, vf=vf, i_f=i_f, ctr=ctr, vcc=vcc, k=k):
            rin = V.enear(V.E24, (vin - vf) / i_f)
            ifr = (vin - vf) / rin
            rlmin = (vcc - 0.3) / (ctr / 100 * ifr)
            chk(r, 'I_F', ifr)
            chk(r, 'R_{L\\min}', rlmin)
            chk(r, 'R_L', V.eceil(V.E24, k * rlmin))
        V.case('Optocoupleur', {'V_{in}': vin, 'V_F': vf, 'I_F': i_f, 'CTR': ctr, 'V_{CC}': vcc, 'k': k}, c)
    for _ in range(5):
        u, R, K, cr, i0 = V.lu(3, 48), V.lu(0.2, 10), V.lu(0.005, 0.2), V.lu(1e-3, 0.2), V.lu(0.01, 0.5)

        def c(r, u=u, R=R, K=K, cr=cr, i0=i0):
            i = i0 + cr / K
            w = (u - R * i) / K
            chk(r, 'I', i)
            chk(r, 'N', w * 60 / (2 * math.pi))
            chk(r, '\\eta', cr * w / (u * i) * 100)
            # puissance utile maximale : recherche numerique
            ws = np.linspace(0, (u - R * i0) / K, 20001)
            pu = (K * ((u - K * ws) / R - i0)) * ws
            chk(r, 'P_{u,\\max}', pu.max(), 1e-6)
        V.case('Moteur à courant continu', {'U': u, 'R': R, 'K': K, 'C_r': cr, 'I_0': i0}, c)
    for _ in range(5):
        ang, mu, f, pitch, L, R, In, Vs = random.choice([0.9, 1.8, 7.5]), random.randrange(9), V.lu(10, 1e5), random.choice([0.0, 2e-3, 8e-3]), V.lu(1e-3, 2e-2), V.lu(0.5, 5), V.lu(0.3, 3), random.choice([12.0, 24.0, 48.0])

        def c(r, ang=ang, mu=mu, f=f, pitch=pitch, L=L, R=R, In=In, Vs=Vs):
            spr = 360 / ang * (1 << mu)
            chk(r, 'N', f / spr * 60)
            if R * In < Vs:
                tm = -L / R * math.log(1 - R * In / Vs)
                chk(r, 'f_{\\max}', 1 / (2 * tm))
        V.case('Moteur pas à pas', {'\\theta': ang, '#Micropas': mu, 'f': f, 'p': pitch, 'L': L, 'R': R, 'I_n': In, 'V': Vs}, c)
    for _ in range(4):
        u, f, p, n, pu, eta, cosp, cp = random.choice([230.0, 400.0, 690.0]), random.choice([50.0, 60.0]), random.randint(1, 4), 0, V.lu(100, 1e5), random.uniform(70, 96), random.uniform(0.6, 0.95), random.randint(0, 1)
        n = 60 * f / p * random.uniform(0.9, 0.99)

        def c(r, u=u, f=f, p=p, n=n, pu=pu, eta=eta, cosp=cosp, cp=cp):
            ns = 60 * f / p
            chk(r, 'g', (ns - n) / ns * 100)
            chk(r, 'C_u', pu / (n * 2 * math.pi / 60))
            I = pu / (eta / 100) / (math.sqrt(3) * u * cosp)
            chk(r, 'I', I)
            chk(r, 'J', I if cp == 0 else I / math.sqrt(3))
        V.case('Moteur asynchrone triphasé', {'U': u, 'f': f, 'p': p, '#Couplage': cp, 'N': n, 'P_u': pu, '\\eta': eta, '\\cos\\varphi': cosp}, c)


# ==========================================================================
# Antennes et radiofrequences
# ==========================================================================
def rf_cases():
    for _ in range(4):
        f, k = V.lu(1e3, 1e11), random.choice([1.0, 0.66, 0.85])
        V.case('Longueur d\'onde', {'f': f, 'k': k}, lambda r, f=f, k=k: (chk(r, '\\lambda_0', C0 / f), chk(r, '\\lambda/4', k * C0 / f / 4)))
    for t in range(4):
        f, K = V.lu(1e6, 3e9), random.uniform(0.9, 0.98)
        V.case('Antennes filaires', {'#Antenne': t, 'f': f, 'K': K},
               lambda r, f=f, K=K, t=t: chk(r, 'L', K * C0 / f / (2 if t <= 1 else 4)))
    for _ in range(6):
        pt, lt, gt, f, d, gr, lr, lo, s = random.uniform(-10, 40), random.uniform(0, 3), random.uniform(0, 20), V.lu(1e6, 1e10), V.lu(10, 1e6), random.uniform(0, 20), random.uniform(0, 3), random.uniform(0, 20), random.uniform(-140, -60)

        def c(r, pt=pt, lt=lt, gt=gt, f=f, d=d, gr=gr, lr=lr, lo=lo, s=s):
            fsl = 20 * math.log10(4 * math.pi * d * f / C0)
            pr = pt - lt + gt - fsl - lo + gr - lr
            chk(r, 'FSL', fsl)
            chk(r, 'P_r', pr)
            chk(r, 'M', pr - s, 1e-9, 1e-9)
            # portee : distance ou P_r = S (recherche numerique)
            g = lambda l: pt - lt + gt - 20 * math.log10(4 * math.pi * 10 ** l * f / C0) - lo + gr - lr - s
            lo_, hi_ = -3.0, 12.0
            for _ in range(200):
                md = (lo_ + hi_) / 2
                if g(md) > 0:
                    lo_ = md
                else:
                    hi_ = md
            chk(r, 'd_{\\max}', 10 ** lo_, 1e-9)
        V.case('Bilan de liaison', {'P_e': pt, 'L_e': lt, 'G_e': gt, 'f': f, 'd': d, 'G_r': gr, 'L_r': lr, 'L_{sup}': lo, 'S': s}, c)
    for _ in range(6):
        z0, rl, xl, ll = random.choice([50.0, 75.0, 300.0]), V.lu(1, 1e3), random.uniform(-500, 500), random.uniform(0, 1)

        def c(r, z0=z0, rl=rl, xl=xl, ll=ll):
            zl = complex(rl, xl)
            g = (zl - z0) / (zl + z0)
            chk(r, '|\\Gamma|', abs(g))
            chk(r, 'ROS', (1 + abs(g)) / (1 - abs(g)))
            chk(r, 'RL', -20 * math.log10(abs(g)))
            # impedance ramenee : propagation des ondes sur la ligne
            gin = g * cmath.exp(-2j * 2 * math.pi * ll)
            chkc(r, 'Z_e', z0 * (1 + gin) / (1 - gin), 1e-7)
        V.case('ROS et abaque de Smith', {'Z_0': z0, 'R_L': rl, 'X_L': xl, '\\ell/\\lambda': ll}, c)
    for _ in range(6):
        rs, rl, f, typ = V.lu(5, 2e3), V.lu(5, 2e3), V.lu(1e5, 1e9), random.randint(0, 1)

        def c(r, rs=rs, rl=rl, f=f, typ=typ):
            rh, rlo = max(rs, rl), min(rs, rl)
            q = math.sqrt(rh / rlo - 1)
            w = 2 * math.pi * f
            xs, xp = q * rlo, rh / q
            chk(r, 'Q', q)
            if typ == 0:
                chk(r, 'L_s', xs / w)
                chk(r, 'C_p', 1 / (w * xp))
            # transfert de puissance : 1 a f, -3 dB par recherche numerique
            def gain(ff):
                ww = 2 * math.pi * ff
                zs = 1j * ww * xs / w if typ == 0 else 1 / (1j * ww / (w * xs))
                zp = 1 / (1j * ww / (w * xp)) if typ == 0 else 1j * ww * xp / w
                E = [('R', 's', 'a', rs)]
                if rs < rl:
                    E += [('Z', 'a', 'b', zs), ('Z', 'b', 'g', zp), ('R', 'b', 'g', rl)]
                    out = 'b'
                else:
                    E += [('Z', 'a', 'g', zp), ('Z', 'a', 'b', zs), ('R', 'b', 'g', rl)]
                    out = 'b'
                EE = [('R', a, b, v) for k, a, b, v in E]
                v = mna(EE, 0, True, [('s', 'g', 1.0)], [], [out])[out]
                return abs(v) * 2 * math.sqrt(rs / rl)
            V.check(r, 'P', 0, 0, 1) if False else None
            g0 = gain(f)
            V.stats['ok' if abs(g0 - 1) < 1e-9 else 'fail'] += 1
            if abs(g0 - 1) >= 1e-9:
                print('  ECART   adaptation : gain %g a f' % g0)
            if q > 0:
                tgt = 1 / math.sqrt(2)
                for side, lab in ((-1, 'f_1'), (1, 'f_2')):
                    lo_, hi_ = math.log10(f), math.log10(f) + side * 3
                    for _ in range(80):
                        md = (lo_ + hi_) / 2
                        if gain(10 ** md) >= tgt:
                            lo_ = md
                        else:
                            hi_ = md
                    if gain(10 ** (math.log10(f) + side * 3)) < tgt:
                        chk(r, lab, 10 ** lo_, 1e-6)
        V.case('Adaptation par réseau en L', {'R_s': rs, 'R_L': rl, 'f': f, '#Réseau': typ}, c)
    for _ in range(4):
        T, B, NF, snr, R, g1, nf2 = random.uniform(200, 400), V.lu(1e3, 1e8), random.uniform(0, 15), random.uniform(-20, 20), V.lu(10, 1e6), random.uniform(0, 30), random.uniform(0, 20)

        def c(r, T=T, B=B, NF=NF, snr=snr, R=R, g1=g1, nf2=nf2):
            kb = 1.380649e-23
            n = 10 * math.log10(kb * T * B / 1e-3)
            chk(r, 'N', n)
            chk(r, 'v_n', math.sqrt(4 * kb * T * R * B))
            chk(r, 'S', n + NF + snr, 1e-9, 1e-9)
            f = 10 ** (NF / 10) + (10 ** (nf2 / 10) - 1) / 10 ** (g1 / 10)
            chk(r, 'NF_{tot}', 10 * math.log10(f))
        V.case('Bruit thermique et sensibilité', {'T': T, 'B': B, 'NF': NF, 'SNR_{\\min}': snr, 'R': R, 'G_1': g1, 'NF_2': nf2}, c)
    for _ in range(3):
        h1, h2, d, f = V.lu(1, 300), V.lu(1, 300), V.lu(100, 1e5), V.lu(1e8, 1e10)

        def c(r, h1=h1, h2=h2, d=d, f=f):
            Re = 6371e3 * 4 / 3
            chk(r, 'd_{radio}', math.sqrt(2 * Re * h1) + math.sqrt(2 * Re * h2))
            lam = C0 / f
            chk(r, 'r_1', math.sqrt(lam * (d / 2) * (d / 2) / d))
        V.case('Horizon radio et zone de Fresnel', {'h_1': h1, 'h_2': h2, 'd': d, 'f': f}, c)


# ==========================================================================
# Traitement du signal
# ==========================================================================
def sig_cases():
    for _ in range(8):
        fs, f = V.lu(100, 1e6), V.lu(1, 1e7)

        def c(r, fs=fs, f=f):
            # frequence apparente : frequence de la sinusoide qui passe par les echantillons (FFT)
            fa = min(abs(f - k * fs) for k in range(0, int(f / fs) + 3))
            chk(r, 'f_a', fa, 1e-9, 1e-9 * fs)
        V.case('Échantillonnage et repliement', {'f_e': fs, 'f': f}, c)
    for _ in range(8):
        n, vmin, vmax, vin, rnd = random.randint(4, 24), random.choice([0.0, -5.0, -2.5]), random.choice([3.3, 5.0, 10.0]), random.uniform(-6, 11), random.randint(0, 1)

        def c(r, n=n, vmin=vmin, vmax=vmax, vin=vin, rnd=rnd):
            q = (vmax - vmin) / 2 ** n
            x = (vin - vmin) / q
            code = math.floor(x + 0.5) if rnd else math.floor(x)
            code = min(max(code, 0), 2 ** n - 1)
            chk(r, 'q', q)
            chk(r, 'N', code)
            chk(r, 'SNR', 6.0206 * n + 1.7609)
        V.case('Convertisseur analogique-numérique', {'n': n, 'V_{ref-}': vmin, 'V_{ref+}': vmax, 'V_e': vin, '#Quantification': rnd}, c)
    coefs = [[1], [0.5, 0.5], [0.54, 0.46], [0.42, 0.5, 0.08], [0.35875, 0.48829, 0.14128, 0.01168],
             [0.21557895, 0.41663158, 0.277263158, 0.083578947, 0.006947368]]
    for w, a in enumerate(coefs):
        def c(r, a=a):
            # fenetre echantillonnee (N grand) et FFT avec bourrage de zeros
            N, pad = 4096, 64
            nn = np.arange(N)
            win = sum((-1) ** k * a[k] * np.cos(2 * np.pi * k * nn / N) for k in range(len(a)))
            chk(r, 'ENBW', N * np.sum(win ** 2) / np.sum(win) ** 2, 1e-9)
            W = np.abs(np.fft.rfft(win, N * pad))
            W /= W[0]
            # festonnage : a mi-canal
            chk(r, '#festonnage', -20 * math.log10(W[pad // 2]), 1e-3)
            # lobe secondaire : maximum apres le premier zero (nombre de termes)
            k0 = len(a) * pad
            chk(r, '#lobe secondaire', 20 * math.log10(W[k0:k0 + 40 * pad].max()), 2e-3)
        V.case('Acquisition et FFT', {'f_e': 48e3, 'N': 1024, 'f': 1e3, '#Fenêtre': w}, c)
    for _ in range(5):
        t, fc, fmd, vc, m, R, rd = random.randint(0, 2), V.lu(1e4, 1e7), V.lu(100, 1e4), V.lu(0.1, 10), random.uniform(0.05, 0.95), random.choice([50.0, 75.0]), V.lu(1e3, 1e5)

        def c(r, t=t, fc=fc, fmd=fmd, vc=vc, m=m, R=R, rd=rd):
            # puissance moyenne : carre moyen du signal module (raies independantes)
            amps = {0: [vc, m * vc / 2, m * vc / 2], 1: [m * vc / 2, m * vc / 2], 2: [m * vc / 2]}[t]
            p = sum(x * x / 2 for x in amps) / R
            chk(r, 'P_t', p, 1e-9)
            if t == 0:
                chk(r, 'RC_{\\max}', math.sqrt(1 - m * m) / (2 * math.pi * fmd * m))
        inp = {'#Modulation': t, 'f_p': fc, 'f_m': fmd, 'V_p': vc, 'm': m, 'R': R}
        if t == 0:
            inp['R_d'] = rd
        V.case('Modulation d\'amplitude', inp, c)
    for _ in range(6):
        t, fc, fmd, df, beta = random.randint(0, 1), V.lu(1e6, 1e9), V.lu(100, 1e5), 0, random.uniform(0, 20)
        df = fmd * V.lu(0.01, 300)

        def c(r, t=t, fmd=fmd, df=df, beta=beta):
            b = df / fmd if t == 0 else beta
            chk(r, '\\beta', b)
            chk(r, 'B_C', 2 * fmd * (b + 1))
            for k in range(4):
                chk(r, 'J_%d' % k, special.jv(k, b), 1e-8, 1e-12)
            n = max([k for k in range(1, 80) if abs(special.jv(k, b)) >= 0.01] or [0])
            chk(r, 'n', n)
        inp = {'#Modulation': t, 'f_p': fc, 'f_m': fmd}
        inp['\\Delta f' if t == 0 else '\\Delta\\varphi'] = df if t == 0 else beta
        V.case('Modulation de fréquence et de phase', inp, c)
    for t in range(8):
        D, al, eb = V.lu(1e3, 1e8), random.uniform(0, 1), random.uniform(-2, 20)

        def c(r, t=t, D=D, al=al, eb=eb):
            M = [2, 2, 4, 8, 16, 64, 256, 2][t]
            k = math.log2(M)
            g = 10 ** (eb / 10)
            Q = lambda x: 0.5 * math.erfc(x / math.sqrt(2))
            if t == 0:
                p = Q(math.sqrt(g))
            elif t in (1, 2):
                p = Q(math.sqrt(2 * g))
            elif t == 3:
                p = 2 / k * Q(math.sqrt(2 * k * g) * math.sin(math.pi / M))
            elif t == 7:
                p = 0.5 * math.exp(-g / 2)
            else:
                p = min(0.5, 4 / k * (1 - 1 / math.sqrt(M)) * Q(math.sqrt(3 * k * g / (M - 1))))
            chk(r, 'R', D / k)
            chk(r, 'TEB', p, 5e-6, 1e-300)
        V.case('Modulations numériques', {'#Modulation': t, 'D': D, '\\alpha': al, 'E_b/N_0': eb}, c)
    for w in range(7):
        A, V0, duty = V.lu(0.1, 100), random.uniform(-10, 10), random.uniform(1, 99)

        def c(r, w=w, A=A, V0=V0, duty=duty):
            t = (np.arange(400000) + 0.5) / 400000
            x = t
            if w == 0:
                v = V0 + A * np.sin(2 * np.pi * x)
            elif w == 1:
                v = V0 + np.where(x < 0.5, A, -A)
            elif w == 2:
                v = V0 + np.where(x < duty / 100, A, 0)
            elif w == 3:
                v = V0 + np.where(x < 0.25, 4 * A * x, np.where(x < 0.75, A * (2 - 4 * x), A * (4 * x - 4)))
            elif w == 4:
                v = V0 + A * (2 * x - 1)
            elif w == 5:
                v = V0 + np.maximum(A * np.sin(2 * np.pi * x), 0)
            else:
                v = V0 + A * np.abs(np.sin(2 * np.pi * x))
            chk(r, 'V_{moy}', v.mean(), 1e-4, 1e-4 * A)
            chk(r, 'V_{eff}', math.sqrt((v * v).mean()), 1e-4)
        inp = {'#Forme': w, 'A': A, 'V_0': V0}
        if w == 2:
            inp['\\alpha'] = duty
        V.case('Valeurs moyenne et efficace', inp, c)
    for _ in range(6):
        typ, fs = random.randint(0, 1), V.lu(100, 1e6)
        fc, N = fs * random.uniform(0.001, 0.2), random.randint(2, 64)

        def c(r, typ=typ, fs=fs, fc=fc, N=N):
            def H(f):
                w = 2 * math.pi * f / fs
                z1 = cmath.exp(-1j * w)
                if typ == 0:
                    a = 1 - math.exp(-2 * math.pi * fc / fs)
                    return a / (1 - (1 - a) * z1)
                return sum(z1 ** k for k in range(N)) / N
            if typ == 0:
                chk(r, '\\alpha', 1 - math.exp(-2 * math.pi * fc / fs))
            fgrid = np.linspace(1e-9, fs / 2 * 0.999999, 200001)
            mags = np.array([abs(H(f)) for f in fgrid[:: 50]])
            idx = np.argmax(mags < 1 / math.sqrt(2))
            lo_, hi_ = fgrid[max(0, (idx - 1) * 50)], fgrid[idx * 50]
            for _ in range(80):
                md = (lo_ + hi_) / 2
                if abs(H(md)) > 1 / math.sqrt(2):
                    lo_ = md
                else:
                    hi_ = md
            chk(r, 'f_{-3dB}', lo_, 1e-7)
        inp = {'#Filtre': typ, 'f_e': fs}
        inp['f_c' if typ == 0 else 'N'] = fc if typ == 0 else N
        V.case('Filtre numérique du premier ordre', inp, c)
