// Fiches : associations de composants.
extern const char CH_ASSOC[];
const char CH_ASSOC[] = R"TEX(
@page Résistances
@card En série
@tags serie somme
@math R_{\text{éq}}=R_1+R_2+\cdots+R_n=\sum_{k=1}^{n}R_k
@text Le même courant traverse toutes les résistances ; les tensions s'ajoutent. $R_{\text{éq}}$ est plus grande que la plus grande des $R_k$.
@card En parallèle
@tags parallele derivation
@math \frac{1}{R_{\text{éq}}}=\sum_{k=1}^{n}\frac{1}{R_k}\qquad G_{\text{éq}}=\sum G_k
@math R_1\parallel R_2=\frac{R_1R_2}{R_1+R_2}
@text Même tension aux bornes de toutes les résistances ; les courants s'ajoutent. $R_{\text{éq}}$ est plus petite que la plus petite des $R_k$.
@note $n$ résistances égales $R$ : $nR$ en série, $R/n$ en parallèle.
@card Diviseurs
@tags diviseur courant tension
@math U_k=U\,\frac{R_k}{\sum R}\qquad I_k=I\,\frac{G_k}{\sum G}
@note Diviseur de tension en série, diviseur de courant en parallèle ($G=1/R$).

@page Condensateurs
@card En parallèle
@tags parallele somme capacite
@math C_{\text{éq}}=C_1+C_2+\cdots+C_n
@text Les charges s'ajoutent sous la même tension.
@card En série
@tags serie capacite
@math \frac{1}{C_{\text{éq}}}=\sum_{k=1}^{n}\frac{1}{C_k}\qquad C_1\,\text{série}\,C_2=\frac{C_1C_2}{C_1+C_2}
@text Même charge $Q$ sur chaque condensateur : $U_k=Q/C_k$. Le plus petit condensateur porte la plus grande tension.
@note Règles « inversées » par rapport aux résistances : $Z_C=1/(jC\omega)$.

@page Inductances
@card Sans couplage
@tags serie parallele bobine
@math L_{\text{série}}=\sum L_k\qquad\frac{1}{L_{\text{par.}}}=\sum\frac{1}{L_k}
@text Mêmes règles que les résistances : $Z_L=jL\omega$.
@card Avec couplage (mutuelle M)
@tags mutuelle couplage
@math L=L_1+L_2\pm 2M
@note Signe + si les flux s'ajoutent (bobines dans le même sens), − sinon. $M=k\sqrt{L_1L_2}$, $0\le k\le 1$.

@page Résistance à associer
@card Trouver R_x
@tags cible
@text En série : $R=R_1+R_x$, d'où
@math R_x=R-R_1\qquad(R>R_1)
@text En parallèle : $\dfrac{1}{R}=\dfrac{1}{R_1}+\dfrac{1}{R_x}$, d'où
@math R_x=\frac{R_1R}{R_1-R}\qquad(R<R_1)
@note On prend ensuite la valeur normalisée la plus proche (série E24) et on vérifie l'écart obtenu.
)TEX";
