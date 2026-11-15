// Fiches : transformateurs et bobines.
extern const char CH_MAG[];
const char CH_MAG[] = R"TEX(
@page Transformateur idéal
@card Relations
@tags rapport transformation
@math m=\frac{N_2}{N_1}=\frac{V_2}{V_1}=\frac{I_1}{I_2}
@math S=V_1I_1=V_2I_2\qquad R_L'=\frac{R_L}{m^2}
@note Points de polarité : les tensions repérées par les points sont en phase.

@page Transformateur réel (modèle de Kapp)
@card Modèle ramené au secondaire
@tags kapp modele chute
@math R_s=R_2+m^2R_1\qquad X_s=X_2+m^2X_1=L_s\omega
@math \underline{V_2}=m\underline{V_1}-(R_s+jX_s)\underline{I_2}
@math \Delta V_2\approx R_sI_2\cos\varphi_2+X_sI_2\sin\varphi_2
@card Pertes et rendement
@tags rendement pertes fer joule
@math P_{fer}=\frac{V_1^2}{R_f}\qquad P_J=R_sI_2^2\qquad\eta=\frac{P_2}{P_2+P_J+P_{fer}}
@card Courant à vide
@tags magnetisant
@math \underline{I_{10}}=\frac{\underline{V_1}}{R_f}+\frac{\underline{V_1}}{jL_\mu\omega}\qquad\underline{I_1}=m\underline{I_2}+\underline{I_{10}}

@page Dimensionnement d'un transformateur
@card Formule de Boucherot
@tags boucherot spires volt
@math V=4{,}44\,N\,f\,B_{\max}A_e\qquad n_V=\frac{N}{V}=\frac{1}{4{,}44\,f\,B_{\max}A_e}
@card Fils
@tags section diametre densite courant
@math s=\frac{I}{J}\qquad d=\sqrt{\frac{4s}{\pi}}
@note Repère empirique à 50 Hz : $A_e\approx\sqrt{S}$ (cm², S en VA). On ajoute quelques % de spires au secondaire pour compenser la chute en charge.

@page Inductances couplées
@card Coefficient de couplage
@tags mutuelle couplage
@math M=k\sqrt{L_1L_2}\qquad0\le k\le1
@math v_1=L_1\frac{di_1}{dt}+M\frac{di_2}{dt}\qquad v_2=M\frac{di_1}{dt}+L_2\frac{di_2}{dt}
@card Associations
@tags serie parallele
@math L_{s\pm}=L_1+L_2\pm2M\qquad L_{p\pm}=\frac{L_1L_2-M^2}{L_1+L_2\mp2M}
@card Modèle en T et énergie
@tags energie fuite
@math L_1-M,\quad L_2-M,\quad M\qquad L_{f1}=(1-k^2)L_1
@math W=\frac12L_1I_1^2+\frac12L_2I_2^2+MI_1I_2

@page Inductance de mode commun
@card Principe
@tags cem filtre secteur
@text Les deux enroulements sont bobinés en sens inverse sur un même tore : les courants utiles (aller et retour) créent des flux qui s'annulent, alors qu'un courant de mode commun (même sens sur les deux fils) voit toute l'inductance.
@math L_{mc}=A_LN^2\qquad L_{md}=k_fL_{mc}
@card Impédance et atténuation
@tags insertion resonance
@math Z_{mc}=jL_{mc}\omega\parallel\frac{1}{jC_p\omega}\qquad f_r=\frac{1}{2\pi\sqrt{L_{mc}C_p}}
@math IL=20\log\left|1+\frac{Z_{mc}}{2R}\right|

@page Inductance d'une bobine
@card Solénoïde à air (Wheeler)
@tags solenoide air
@math L=\frac{\mu_0N^2A}{\ell+0{,}45\,d}\qquad A=\frac{\pi d^2}{4}
@card Tore
@tags tore
@math L=\frac{\mu_0\mu_rN^2h}{2\pi}\ln\frac{D_e}{D_i}
@card Noyau avec A_L
@tags al
@math L=A_LN^2\qquad N=\sqrt{\frac{L}{A_L}}\qquad W=\frac12LI^2
)TEX";
