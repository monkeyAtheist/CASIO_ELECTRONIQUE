// Fiches : transistors bipolaires.
extern const char CH_BJT[];
const char CH_BJT[] = R"TEX(
@page Polarisation par pont diviseur
@card Thévenin du pont
@tags thevenin pont
@math V_{th}=V_{CC}\frac{R_2}{R_1+R_2}\qquad R_{th}=R_1\parallel R_2
@card Point de repos (NPN)
@tags repos courant
@math I_B=\frac{V_{th}-V_{BE}}{R_{th}+(\beta+1)R_E}\qquad I_C=\beta I_B
@math V_E=R_EI_E\qquad V_C=V_{CC}-R_CI_C\qquad V_{CE}=V_C-V_E
@note Pour un PNP, mêmes formules avec $V_{EB}$, $V_{EC}$ et des tensions comptées depuis $+V_{CC}$.
@card Régimes
@tags saturation bloque lineaire
@li Bloqué : $V_{th}<V_{BE}$, aucun courant.
@li Linéaire : $V_{CE}>V_{CE,\text{sat}}$.
@li Saturé : $V_{CE}=V_{CE,\text{sat}}$ et $I_C<\beta I_B$ (β forcé).
@note Point de repos stable si $R_E$ est grande devant $R_{th}/\beta$ et si le courant du pont vaut au moins $10\,I_B$.
@card Petits signaux (émetteur commun)
@tags gain transconductance
@math g_m=\frac{I_C}{V_T}\qquad r_\pi=\frac{\beta}{g_m}\qquad V_T=\frac{kT}{q}\approx26\ \text{mV}
@math A_v=-\frac{\beta R_C}{r_\pi+(\beta+1)R_E}\qquad A_v=-g_mR_C\ (R_E\ \text{découplée})
@math Z_e=R_{th}\parallel\left(r_\pi+(\beta+1)R_E\right)\qquad Z_s\approx R_C

@page Polarisation par résistance de base
@card Point de repos
@tags resistance base
@math I_B=\frac{V_{CC}-V_{BE}}{R_B+(\beta+1)R_E}\qquad I_C=\beta I_B
@math V_{CE}=V_{CC}-R_CI_C-R_EI_E
@card Sensibilité à β
@tags stabilite dispersion
@text Sans $R_E$, $I_C$ est proportionnel à β : la dispersion des transistors (β de 100 à 400 pour un même modèle) déplace fortement le point de repos. Le pont diviseur avec $R_E$ corrige ce défaut.

@page Transistor en commutation
@card Saturer le transistor
@tags saturation commutation relais
@math I_{C}=\frac{V_{CC}-V_{CE,\text{sat}}}{R_L}\qquad I_{B\min}=\frac{I_C}{\beta_{\min}}
@math R_B\le\frac{V_{in}-V_{BE,\text{sat}}}{k\,I_{B\min}}\qquad k\approx2\ \text{à}\ 5
@text On prend la valeur normalisée immédiatement inférieure pour garantir la saturation.
@card Pertes
@tags puissance
@math P_T\approx V_{CE,\text{sat}}\,I_C\qquad P_L=R_LI_C^2
@note Charge inductive (relais, moteur) : ajouter une diode de roue libre en parallèle sur la charge.

@page Émetteur commun : petits signaux
@card Modèle en π (petits signaux)
@tags hybride pi transconductance dynamique
@math g_m=\frac{I_C}{V_T}\qquad r_\pi=\frac{\beta}{g_m}\qquad r_o=\frac{V_A}{I_C}
@text En alternatif, les alimentations et les condensateurs de liaison sont des courts-circuits. Le transistor devient $r_\pi$ entre base et émetteur, et une source de courant $g_mv_{be}$ en parallèle avec $r_o$ entre collecteur et émetteur.
@card Gain en tension
@tags gain amplification
@math R_C'=R_C\parallel R_L\parallel r_o
@math A_v=\frac{v_s}{v_e}=-\frac{\beta R_C'}{r_\pi+(\beta+1)R_{E1}}
@math A_v\approx-g_mR_C'\ (R_{E1}=0)\qquad A_v\approx-\frac{R_C'}{R_{E1}}\ (\beta R_{E1}\gg r_\pi)
@card Impédances et gains
@tags impedance entree sortie courant puissance
@math Z_e=R_1\parallel R_2\parallel\left(r_\pi+(\beta+1)R_{E1}\right)\qquad Z_s\approx R_C\parallel r_o
@math A_{vg}=A_v\frac{Z_e}{Z_e+R_g}\qquad A_i=A_v\frac{Z_e}{R_L}\qquad G_p=10\log\left|A_vA_i\right|
@card Coupure basse (liaisons)
@tags liaison decouplage condensateur basse frequence
@math f_{C_e}=\frac{1}{2\pi(R_g+Z_e)C_e}\qquad f_{C_s}=\frac{1}{2\pi(Z_s+R_L)C_s}
@math f_{C_E}=\frac{1}{2\pi R_{C_E}C_E}\qquad R_{C_E}=R_{E2}\parallel\left(R_{E1}+\frac{r_\pi+R_g'}{\beta+1}\right)
@math R_g'=R_g\parallel R_1\parallel R_2\qquad f_b\approx f_{C_e}+f_{C_s}+f_{C_E}
@note Méthode des constantes de temps : chaque condensateur seul, les autres en court-circuit. Le calculateur donne aussi $f_b$ exacte, par recherche numérique sur $|A_{vg}(f)|$.
@card Coupure haute (effet Miller)
@tags miller capacite haute frequence
@math C_M=C_\mu\left(1+g_mR_C'\right)
@math f_h\approx\frac{1}{2\pi\left(R_g'\parallel r_\pi\right)\left(C_\pi+C_M\right)}
@note Formule approchée pour $R_{E1}=0$. Le diagramme de Bode est calculé exactement (analyse nodale du schéma équivalent complet).

@page Collecteur commun : petits signaux
@card Gain et impédances
@tags suiveur gain impedance
@math R_E'=R_E\parallel R_L\parallel r_o\qquad A_v=\frac{(\beta+1)R_E'}{r_\pi+(\beta+1)R_E'}\approx1
@math Z_e=R_1\parallel R_2\parallel\left(r_\pi+(\beta+1)R_E'\right)
@math Z_s=R_E\parallel\frac{r_\pi+R_g\parallel R_1\parallel R_2}{\beta+1}
@card Point de repos
@tags polarisation
@math I_B=\frac{V_{th}-V_{BE}}{R_{th}+(\beta+1)R_E}\qquad V_E=V_B-V_{BE}
@note Point milieu conseillé : $V_E\approx V_{CC}/2$ pour une excursion maximale.
@card Utilisation
@tags adaptateur impedance
@text Adaptateur d'impédance : forte impédance d'entrée, faible impédance de sortie, gain en tension proche de 1 et gain en courant élevé.

@page Base commune : petits signaux
@card Gain et impédances
@tags base commune gain impedance
@math A_v=\frac{v_s}{v_e}\approx g_m\left(R_C\parallel R_L\right)\quad(\text{non inverseur})
@math Z_e=R_E\parallel\frac{r_\pi}{\beta+1}\approx R_E\parallel r_e\qquad r_e=\frac{V_T}{I_E}
@math Z_s\approx R_C\qquad A_i\approx\alpha=\frac{\beta}{\beta+1}
@note La base est découplée par $C_B$ : pas d'effet Miller, d'où une large bande passante (étages HF, montage cascode).

@page Paire différentielle
@card Polarisation
@tags repos queue
@math I_E=\frac{V_{EE}-V_{BE}}{2R_{EE}+R_E}\qquad V_C=V_{CC}-R_CI_C
@card Mode différentiel
@tags differentiel gain
@math v_d=v_{e1}-v_{e2}\qquad A_d=\frac{v_{s1}-v_{s2}}{v_d}=-\frac{\beta R_C}{r_\pi+(\beta+1)R_E}
@math A_{d1}=\frac{v_{s1}}{v_d}=\frac{A_d}{2}\qquad Z_d=2\left(r_\pi+(\beta+1)R_E\right)
@card Mode commun
@tags mode commun trmc cmrr
@math v_{mc}=\frac{v_{e1}+v_{e2}}{2}\qquad A_{mc}=-\frac{\beta R_C}{r_\pi+(\beta+1)(R_E+2R_{EE})}
@math \text{TRMC}=20\log\left|\frac{A_{d1}}{A_{mc}}\right|\approx20\log\left(g_mR_{EE}\right)
@note Une source de courant (grande résistance interne $R_0$) à la place de $R_{EE}$ améliore fortement le TRMC.

@page Paramètres hybrides du bipolaire
@card Paramètres h (émetteur commun)
@tags hybride h11 h21 h22
@math \begin{pmatrix}v_{be}\\i_c\end{pmatrix}=\begin{pmatrix}h_{11}&h_{12}\\h_{21}&h_{22}\end{pmatrix}\begin{pmatrix}i_b\\v_{ce}\end{pmatrix}
@math h_{11}=r_\pi\qquad h_{12}\approx0\qquad h_{21}=\beta\qquad h_{22}=\frac{1}{r_o}
@card Modèle en π
@tags transconductance early
@math g_m=\frac{I_C}{V_T}\qquad r_\pi=\frac{\beta}{g_m}\qquad r_e=\frac{V_T}{I_E}\qquad r_o=\frac{V_A}{I_C}
@math \mu=g_mr_o=\frac{V_A}{V_T}
@card Fréquences
@tags transition ft
@math f_T=\frac{g_m}{2\pi(C_\pi+C_\mu)}\qquad f_\beta=\frac{f_T}{\beta}
@note $f_\beta$ : coupure à −3 dB du gain en courant $\beta(f)$ ; $f_T$ : fréquence où $|\beta(f)|=1$.
)TEX";
