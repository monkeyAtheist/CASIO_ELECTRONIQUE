// Fiches : transistors a effet de champ.
extern const char CH_FET[];
const char CH_FET[] = R"TEX(
@page MOSFET : polarisation
@card Modèle (enrichissement, canal N)
@tags mosfet modele quadratique
@li Bloqué, $V_{GS}<V_{th}$ : $I_D=0$.
@li Ohmique, $V_{DS}<V_{GS}-V_{th}$ :
@math I_D=K\left[2(V_{GS}-V_{th})V_{DS}-V_{DS}^2\right]
@li Saturation, $V_{DS}\ge V_{GS}-V_{th}$ :
@math I_D=K\,(V_{GS}-V_{th})^2
@note $K=\frac12\mu C_{ox}\frac{W}{L}$ (A/V²). Canal P : mêmes relations avec $V_{SG}$, $V_{SD}$ et $|V_{th}|$.
@card Point de repos (pont diviseur)
@tags repos pont
@math V_G=V_{DD}\frac{R_2}{R_1+R_2}\qquad V_{GS}=V_G-R_SI_D
@math V_{DS}=V_{DD}-(R_D+R_S)\,I_D
@text En saturation, $I_D$ est racine de $I_D=K\,(V_G-R_SI_D-V_{th})^2$.
@card Petits signaux
@tags transconductance gain
@math g_m=2K(V_{GS}-V_{th})=2\sqrt{K\,I_D}
@math A_v=-\frac{g_mR_D}{1+g_mR_S}\qquad Z_e=R_1\parallel R_2

@page JFET : polarisation
@card Équation de Shockley
@tags jfet shockley idss pincement
@math I_D=I_{DSS}\left(1-\frac{V_{GS}}{V_P}\right)^2\qquad V_P\le V_{GS}\le0
@note Canal N : $V_P<0$ ; canal P : $V_P>0$. Saturation si $|V_{DS}|\ge|V_{GS}-V_P|$.
@card Autopolarisation
@tags autopolarisation
@math V_G=0\qquad V_{GS}=-R_SI_D
@math R_S=\frac{-V_{GS}}{I_D}\qquad V_{DS}=V_{DD}-(R_D+R_S)I_D
@card Transconductance
@tags transconductance gain
@math g_{m0}=\frac{2I_{DSS}}{|V_P|}\qquad g_m=g_{m0}\left(1-\frac{V_{GS}}{V_P}\right)=g_{m0}\sqrt{\frac{I_D}{I_{DSS}}}
@math A_v=-\frac{g_mR_D}{1+g_mR_S}

@page MOSFET en commutation
@card Pertes
@tags pertes rdson commutation
@math P_{\text{cond}}=\alpha\,R_{DS(on)}\,I_D^2
@math t_{\text{com}}\approx\frac{Q_g}{I_G}\qquad P_{\text{com}}\approx V_{DD}\,I_D\,t_{\text{com}}\,f_s
@note Estimation : chaque commutation dure environ $t_{\text{com}}$, avec une puissance moyenne $\frac12V_{DD}I_D$ ; deux commutations par période.
@card Température de jonction
@tags temperature dissipateur
@math T_j=T_a+P\,R_{th\,ja}\qquad P=P_{\text{cond}}+P_{\text{com}}
@note $R_{DS(on)}$ augmente avec la température (souvent ×1,5 à 100 °C).

@page Source commune : petits signaux
@card Transconductance
@tags gm transconductance
@math \text{JFET : }g_m=\frac{2I_{DSS}}{|V_P|}\left(1-\frac{V_{GS}}{V_P}\right)
@math \text{MOSFET : }g_m=2K\left(V_{GS}-V_{th}\right)=2\sqrt{KI_D}\qquad r_{ds}=\frac{1}{\lambda I_D}
@card Gain et impédances
@tags gain impedance
@math R_D'=R_D\parallel R_L\parallel r_{ds}\qquad A_v=-\frac{g_mR_D'}{1+g_mR_{S1}}
@math Z_e=R_1\parallel R_2\ (\text{ou }R_G)\qquad Z_s\approx R_D\parallel r_{ds}
@card Coupures
@tags miller frequence decouplage
@math f_{C_S}=\frac{1}{2\pi R_{C_S}C_S}\qquad R_{C_S}=R_{S2}\parallel\left(R_{S1}+\frac{1}{g_m}\right)
@math C_M=C_{gd}\left(1+g_mR_D'\right)\qquad f_h\approx\frac{1}{2\pi\left(R_g\parallel R_G\right)(C_{gs}+C_M)}
@note La grille ne consomme pas de courant : $Z_e$ est fixée par le pont (souvent 1 MΩ).

@page Drain commun : petits signaux
@card Gain et impédances
@tags suiveur source
@math R_S'=R_S\parallel R_L\parallel r_{ds}\qquad A_v=\frac{g_mR_S'}{1+g_mR_S'}<1
@math Z_e=R_1\parallel R_2\qquad Z_s=R_S\parallel\frac{1}{g_m}

@page Grille commune : petits signaux
@card Gain et impédances
@tags grille commune
@math A_v\approx g_m\left(R_D\parallel R_L\right)\qquad Z_e=R_S\parallel\frac{1}{g_m}\qquad Z_s\approx R_D
@note Faible impédance d'entrée, pas d'effet Miller : étages HF et cascode.
)TEX";
