// Fiches : filtres.
extern const char CH_FILTRES[];
const char CH_FILTRES[] = R"TEX(
@page Filtre RC du 1er ordre
@card Passe-bas (R série, C en sortie)
@tags passe bas
@math H=\frac{1}{1+j\,\dfrac{f}{f_c}}\qquad f_c=\frac{1}{2\pi RC}
@math |H|=\frac{1}{\sqrt{1+(f/f_c)^2}}\qquad\varphi=-\arctan\frac{f}{f_c}
@card Passe-haut (C série, R en sortie)
@tags passe haut
@math H=\frac{j\,\dfrac{f}{f_c}}{1+j\,\dfrac{f}{f_c}}\qquad\varphi=90°-\arctan\frac{f}{f_c}
@card Repères
@tags bode asymptote
@li À $f_c$ : $|H|=1/\sqrt2$, soit $-3$ dB, et $\varphi=\mp45°$.
@li Pente des asymptotes : $\pm20$ dB par décade ($\pm6$ dB par octave).
@li Une décade au-delà de $f_c$ : $-20$ dB (passe-bas).

@page Filtre RL du 1er ordre
@card Fréquence de coupure
@tags coupure bobine
@math f_c=\frac{R}{2\pi L}\qquad\tau=\frac{L}{R}
@card Fonctions de transfert
@tags passe bas haut
@text Passe-bas (L série, R en sortie) :
@math H=\frac{R}{R+jL\omega}=\frac{1}{1+j\,f/f_c}
@text Passe-haut (R série, L en sortie) :
@math H=\frac{jL\omega}{R+jL\omega}=\frac{j\,f/f_c}{1+j\,f/f_c}

@page Filtre RLC du 2e ordre
@card Forme canonique
@tags canonique
@math x=\frac{f}{f_0}\qquad D=1-x^2+j\,\frac{x}{Q}
@math f_0=\frac{1}{2\pi\sqrt{LC}}\qquad Q=\frac{1}{R}\sqrt{\frac{L}{C}}\qquad\xi=\frac{1}{2Q}
@card Les quatre sorties
@tags passe bas haut bande coupe
@table[ll] \text{Sortie} & H
@row \text{sur }C\text{ : passe-bas} & 1/D
@row \text{sur }L\text{ : passe-haut} & -x^2/D
@row \text{sur }R\text{ : passe-bande} & (jx/Q)/D
@row \text{sur }L+C\text{ : coupe-bande} & (1-x^2)/D
@card Résonance (passe-bas)
@tags resonance surtension
@math f_r=f_0\sqrt{1-\frac{1}{2Q^2}}\qquad|H|_{\max}=\frac{Q}{\sqrt{1-\frac{1}{4Q^2}}}
@note Seulement si $Q>1/\sqrt2$. $Q=1/\sqrt2$ : Butterworth (pas de bosse). Passe-bande : $B=f_0/Q$. Pentes $\pm40$ dB/décade.

@page Sallen-Key passe-bas
@card Montage
@tags sallen key
@text $R_1$ et $R_2$ en série vers l'entrée +, $C_1$ du point milieu vers la sortie, $C_2$ de l'entrée + vers la masse ; AOP en non-inverseur de gain $K=1+R_b/R_a$ (suiveur : $K=1$).
@card Fonction de transfert
@tags transfert
@math H=\frac{K}{1+j\omega\,a_1-\omega^2a_2}
@math a_1=(R_1+R_2)C_2+(1-K)R_1C_1\qquad a_2=R_1R_2C_1C_2
@math f_0=\frac{1}{2\pi\sqrt{a_2}}\qquad Q=\frac{\sqrt{a_2}}{a_1}
@card Butterworth (Q = 0,707)
@tags butterworth
@li Gain unité, $R_1=R_2=R$ : $C_1=2\,C_2$.
@li Composants égaux : $K=3-\sqrt2\approx1{,}586$.
@note Le montage devient instable si le dénominateur de $Q$ s'annule ($K$ trop grand).

@page Sallen-Key passe-haut
@card Montage
@tags sallen key
@text $C_1$ et $C_2$ en série vers l'entrée +, $R_1$ du point milieu vers la sortie, $R_2$ de l'entrée + vers la masse.
@card Fonction de transfert
@tags transfert
@math H=\frac{-K\omega^2a_2}{1+j\omega\,a_1-\omega^2a_2}
@math a_1=R_1(C_1+C_2)+(1-K)R_2C_2\qquad a_2=R_1R_2C_1C_2
@math f_0=\frac{1}{2\pi\sqrt{a_2}}\qquad Q=\frac{\sqrt{a_2}}{a_1}
@card Butterworth
@tags butterworth
@text Gain unité et $C_1=C_2$ : $Q=\frac12\sqrt{R_2/R_1}$, donc $R_2=2\,R_1$.
)TEX";
