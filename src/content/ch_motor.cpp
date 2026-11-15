// Fiches : optocoupleurs et moteurs.
extern const char CH_MOTOR[];
const char CH_MOTOR[] = R"TEX(
@page Optocoupleur
@card Entrée
@tags led resistance
@math R_{in}=\frac{V_{in}-V_F}{I_F}\qquad P_{LED}=V_FI_F
@card Sortie saturée
@tags ctr phototransistor saturation
@math I_{C\max}=\text{CTR}\cdot I_F\qquad R_L\ge k\,\frac{V_{CC}-V_{CE,sat}}{\text{CTR}\cdot I_F}
@note Le CTR baisse avec le vieillissement et la température : prendre une marge $k\approx2$. Une grande $R_L$ ralentit la commutation.

@page Moteur à courant continu
@card Équations
@tags fem couple vitesse
@math E=K\Omega\qquad C_{em}=KI\qquad U=E+RI
@math \Omega=\frac{U-RI}{K}\qquad N=\frac{60\,\Omega}{2\pi}
@card Puissances
@tags rendement puissance
@math P_a=UI\qquad P_J=RI^2\qquad P_u=C_r\Omega\qquad\eta=\frac{P_u}{P_a}
@card Démarrage et caractéristique
@tags demarrage caracteristique
@math I_d=\frac{U}{R}\qquad C_d=K(I_d-I_0)\qquad P_{u,\max}=\frac{C_d\,\Omega_0}{4}
@note La caractéristique $C(N)$ est une droite ; la puissance utile est maximale à mi-vitesse.

@page Moteur pas à pas
@card Résolution et vitesse
@tags pas micropas vitesse
@math n_{pas}=\frac{360}{\theta}\qquad N=\frac{60\,f}{n_{pas}\cdot\mu}\qquad v=\frac{N}{60}\,p
@card Limite en vitesse
@tags inductance constante temps
@math \tau=\frac{L}{R}\qquad t_m=-\tau\ln\left(1-\frac{RI_n}{V}\right)\qquad f_{\max}\approx\frac{1}{2t_m}
@note Alimenter le pilote sous une tension bien supérieure à $RI_n$ (pilote à hachage) permet d'établir plus vite le courant, donc de tourner plus vite.

@page Moteur asynchrone triphasé
@card Vitesse et glissement
@tags synchronisme glissement
@math n_s=\frac{60f}{p}\qquad g=\frac{n_s-N}{n_s}
@card Puissances et couple
@tags couple puissance courant
@math P_a=\sqrt3\,UI\cos\varphi\qquad P_u=\eta P_a\qquad C_u=\frac{P_u}{\Omega}
@math Q=P_a\tan\varphi\qquad S=\sqrt3\,UI
@card Couplage
@tags etoile triangle
@li Étoile : tension d'enroulement $U/\sqrt3$, courant d'enroulement $I$.
@li Triangle : tension d'enroulement $U$, courant d'enroulement $I/\sqrt3$.
)TEX";
