// Fiches : oscillateurs et temporisations.
extern const char CH_OSC[];
const char CH_OSC[] = R"TEX(
@page NE555 astable
@card Durées
@tags 555 astable
@math t_H=\ln2\,(R_A+R_B)\,C\qquad t_L=\ln2\,R_B\,C
@math f=\frac{1}{\ln2\,(R_A+2R_B)\,C}\approx\frac{1{,}44}{(R_A+2R_B)\,C}
@math \alpha=\frac{t_H}{T}=\frac{R_A+R_B}{R_A+2R_B}
@card Principe
@tags seuils
@text Le condensateur se charge par $R_A+R_B$ et se décharge par $R_B$ entre $V_{CC}/3$ et $2V_{CC}/3$ : chaque phase dure $\tau\ln2$.
@note Avec une diode en parallèle sur $R_B$ : $t_H=\ln2\,R_AC$, rapport cyclique inférieur à 50 % possible.

@page NE555 monostable
@card Durée de l'impulsion
@tags 555 monostable
@math T=\ln3\,RC\approx1{,}1\,RC
@text Le condensateur se charge de 0 à $2V_{CC}/3$ après une impulsion basse sur la broche 2.

@page Multivibrateur astable à AOP
@card Période
@tags multivibrateur relaxation
@math \beta=\frac{R_1}{R_1+R_2}\qquad T=2RC\ln\frac{1+\beta}{1-\beta}=2RC\ln\left(1+\frac{2R_1}{R_2}\right)
@text Trigger de Schmitt inverseur (seuils $\pm\beta V_{sat}$) bouclé par $RC$ : le condensateur oscille entre ces seuils.
@note $R_1=R_2$ : $T=2RC\ln3\approx2{,}2\,RC$.

@page Oscillateur à pont de Wien
@card Fréquence et condition
@tags wien
@math f_0=\frac{1}{2\pi RC}\qquad A=1+\frac{R_f}{R_g}=3
@text À $f_0$, le pont transmet $\beta=1/3$ sans déphasage : il faut un gain de 3 (Barkhausen : $A\beta=1$).
@math \beta(f)=\frac{1}{3+j\left(RC\omega-\dfrac{1}{RC\omega}\right)}
@note En pratique $A$ légèrement supérieur à 3, avec une stabilisation d'amplitude (lampe, diodes, CTN).

@page Oscillateurs Colpitts et Hartley
@card Colpitts
@tags colpitts
@math C_{\text{éq}}=\frac{C_1C_2}{C_1+C_2}\qquad f_0=\frac{1}{2\pi\sqrt{LC_{\text{éq}}}}
@card Hartley
@tags hartley
@math L_{\text{éq}}=L_1+L_2+2M\qquad f_0=\frac{1}{2\pi\sqrt{L_{\text{éq}}C}}
@note Condition d'entretien (Barkhausen) : le gain de l'amplificateur compense l'atténuation du diviseur capacitif (ou inductif).
)TEX";
