// Fiches : alimentations.
extern const char CH_ALIM[];
const char CH_ALIM[] = R"TEX(
@page Régulateur LM317
@card Tension de sortie
@tags lm317 regulateur
@math V_s=V_{ref}\left(1+\frac{R_2}{R_1}\right)+I_{adj}R_2\qquad V_{ref}=1{,}25\ \text{V}
@math R_2=\frac{V_s-V_{ref}}{V_{ref}/R_1+I_{adj}}
@note $I_{adj}\approx50\ \mu$A est souvent négligé : $V_s\approx1{,}25\,(1+R_2/R_1)$.
@card Conditions de fonctionnement
@tags chute dissipation
@li Chute $V_e-V_s\ge3$ V environ (régulateur non « LDO »).
@li Courant minimal de sortie ≈ 5 à 10 mA : $R_1\le240\ \Omega$.
@li Puissance : $P=(V_e-V_s)\,I_s$, dissipateur au-delà de 1 à 2 W.

@page Régulation par diode Zener
@card Résistance série
@tags zener
@math R_s\le\frac{V_{e\min}-V_Z}{I_{L\max}+I_{Z\min}}
@text Le pire cas pour la régulation : tension d'entrée minimale et charge maximale.
@card Puissances
@tags puissance
@math I_{Z\max}=\frac{V_{e\max}-V_Z}{R_s}\qquad P_Z=V_ZI_{Z\max}
@math P_{R_s}=\frac{(V_{e\max}-V_Z)^2}{R_s}
@note Pire cas pour la Zener : tension maximale et charge débranchée.

@page Redressement et filtrage
@card Tension crête
@tags redressement diode
@math V_{\max}=V_{\text{eff}}\sqrt2-n\,V_D
@text $n=1$ en simple alternance et en point milieu, $n=2$ avec un pont de Graetz.
@card Ondulation
@tags filtrage condensateur ondulation
@math \Delta V\approx\frac{I}{f_{\text{ond}}\,C}\qquad C\ge\frac{I}{f_{\text{ond}}\,\Delta V}
@math V_{\text{moy}}\approx V_{\max}-\frac{\Delta V}{2}
@text $f_{\text{ond}}=f$ en simple alternance, $2f$ en double alternance (100 Hz sur le secteur 50 Hz).
@card Tension inverse des diodes
@tags tension inverse
@li Pont de Graetz : $V_{\text{inv}}\approx V_{\text{eff}}\sqrt2$.
@li Simple alternance (avec C) et point milieu : $V_{\text{inv}}\approx2\,V_{\text{eff}}\sqrt2$.
)TEX";
