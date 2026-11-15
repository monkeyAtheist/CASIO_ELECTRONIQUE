// Fiches : circuits RC, RL, LC, RLC.
extern const char CH_RLC[];
const char CH_RLC[] = R"TEX(
@page Circuit RC
@card Constante de temps et coupure
@tags tau frequence coupure
@math \tau=RC\qquad f_c=\frac{1}{2\pi RC}\qquad\omega_c=\frac{1}{\tau}
@card Impédances
@tags reactance impedance
@math Z_C=\frac{1}{jC\omega}=-\frac{j}{C\omega}\qquad X_C=\frac{1}{C\omega}
@math \underline{Z}=R+\frac{1}{jC\omega}\qquad|Z|=\sqrt{R^2+X_C^2}
@math \varphi=\arg\underline{Z}=-\arctan\frac{X_C}{R}
@text À $f_c$ : $X_C=R$, $|Z|=R\sqrt2$ et $\varphi=-45°$.
@card Valeurs efficaces
@tags courant tension
@math I=\frac{U}{|Z|}\qquad U_R=RI\qquad U_C=X_CI\qquad U^2=U_R^2+U_C^2
@note Les tensions sur R et C sont en quadrature : elles ne s'additionnent pas arithmétiquement.

@page Circuit RL
@card Constante de temps et coupure
@tags tau frequence coupure
@math \tau=\frac{L}{R}\qquad f_c=\frac{R}{2\pi L}
@card Impédances
@tags reactance impedance
@math Z_L=jL\omega\qquad X_L=L\omega
@math |Z|=\sqrt{R^2+(L\omega)^2}\qquad\varphi=\arctan\frac{L\omega}{R}
@text Le courant est en retard sur la tension ($\varphi>0$, circuit inductif).
@card Établissement du courant
@tags transitoire
@math i(t)=\frac{E}{R}\left(1-e^{-t/\tau}\right)
@note Le courant dans une bobine ne peut pas varier instantanément.

@page Circuit LC (résonance)
@card Fréquence propre
@tags resonance thomson
@math f_0=\frac{1}{2\pi\sqrt{LC}}\qquad\omega_0=\frac{1}{\sqrt{LC}}
@math LC\omega_0^2=1\qquad X_L=X_C\ \text{à}\ f_0
@card Grandeurs déduites
@tags calcul inverse
@math C=\frac{1}{4\pi^2f_0^2L}\qquad L=\frac{1}{4\pi^2f_0^2C}
@math Z_c=\sqrt{\frac{L}{C}}\qquad T_0=2\pi\sqrt{LC}
@note $Z_c$ : impédance caractéristique, réactance commune de L et C à la résonance.

@page Circuit RLC série
@card Impédance
@tags impedance
@math \underline{Z}=R+j\left(L\omega-\frac{1}{C\omega}\right)
@math |Z|=\sqrt{R^2+\left(L\omega-\frac{1}{C\omega}\right)^2}
@card Résonance
@tags resonance facteur qualite
@math f_0=\frac{1}{2\pi\sqrt{LC}}\qquad Q=\frac{1}{R}\sqrt{\frac{L}{C}}=\frac{L\omega_0}{R}
@math \xi=\frac{1}{2Q}\qquad B=f_2-f_1=\frac{f_0}{Q}
@math f_{1,2}=f_0\left(\sqrt{1+\frac{1}{4Q^2}}\mp\frac{1}{2Q}\right)
@text À $f_0$ : $Z=R$ minimale, $I_0=U/R$ maximal, et $U_C=U_L=Q\,U$ (surtension).
@card Régime libre
@tags transitoire amortissement
@li $Q>\frac{1}{2}$ : pseudo-périodique ;
@li $Q=\frac{1}{2}$ : critique ;
@li $Q<\frac{1}{2}$ : apériodique.
@note Équation : $\ddot{q}+\dfrac{\omega_0}{Q}\dot{q}+\omega_0^2q=0$.

@page Circuit RLC parallèle
@card Admittance
@tags admittance impedance bouchon
@math \underline{Y}=\frac{1}{R}+j\left(C\omega-\frac{1}{L\omega}\right)\qquad\underline{Z}=\frac{1}{\underline{Y}}
@card Anti-résonance
@tags facteur qualite
@math f_0=\frac{1}{2\pi\sqrt{LC}}\qquad Q=R\sqrt{\frac{C}{L}}=\frac{R}{L\omega_0}
@math Z(f_0)=R\ \text{maximale}\qquad B=\frac{f_0}{Q}
@note Circuit « bouchon » : il bloque la fréquence $f_0$. Les courants dans L et C valent $Q$ fois le courant de la source.

@page Charge et décharge RC
@card Équation
@tags differentielle
@math RC\frac{du_C}{dt}+u_C=U_f
@math u_C(t)=U_f+(U_0-U_f)\,e^{-t/\tau}\qquad\tau=RC
@text Charge : $U_f=E$. Décharge : $U_f=0$.
@card Courant et instants
@tags courant temps
@math i(t)=C\frac{du_C}{dt}=\frac{U_f-U_0}{R}\,e^{-t/\tau}
@math t=\tau\ln\frac{U_0-U_f}{u-U_f}
@table[cc] t & \text{chemin parcouru}
@row \tau & 63\,\%
@row 3\tau & 95\,\%
@row 5\tau & 99{,}3\,\%
@card Énergie
@tags energie
@math W=\tfrac12CU^2\qquad q=Cu
@note Lors d'une charge complète sous E, la résistance dissipe autant d'énergie que le condensateur en stocke.
)TEX";
