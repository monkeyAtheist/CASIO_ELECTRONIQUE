// Fiches : lois de base.
extern const char CH_BASES[];
const char CH_BASES[] = R"TEX(
@page Loi d'Ohm et puissance
@card Loi d'Ohm
@tags ohm
@math U=R\,I\qquad I=\frac{U}{R}\qquad R=\frac{U}{I}
@card Puissance
@tags puissance joule
@math P=U\,I=R\,I^2=\frac{U^2}{R}
@math U=\sqrt{P\,R}\qquad I=\sqrt{\frac{P}{R}}
@note En alternatif sinusoïdal : $P=U_{\text{eff}}I_{\text{eff}}\cos\varphi$.

@page Diviseur de tension
@card À vide
@tags diviseur
@math V_s=V_e\,\frac{R_2}{R_1+R_2}\qquad R_{th}=R_1\parallel R_2
@card Avec une charge
@tags charge
@math V_s=V_e\,\frac{R_2\parallel R_L}{R_1+R_2\parallel R_L}
@note La charge est négligeable si $R_L\gg R_{th}$ (au moins 10 fois).
@card Choisir R₂
@tags calcul inverse
@math R_2=R_1\,\frac{V_s}{V_e-V_s}
@text Avec charge : on obtient d'abord $R_p=R_2\parallel R_L$, puis $R_2=\dfrac{1}{1/R_p-1/R_L}$.

@page Pont de Wheatstone
@card Tension de déséquilibre
@tags wheatstone
@math V_A=E\frac{R_2}{R_1+R_2}\qquad V_B=E\frac{R_4}{R_3+R_4}
@math V_{AB}=E\left(\frac{R_2}{R_1+R_2}-\frac{R_4}{R_3+R_4}\right)
@card Équilibre
@tags equilibre mesure
@math V_{AB}=0\iff R_1R_4=R_2R_3\iff R_4=\frac{R_2R_3}{R_1}
@note Utilisé pour mesurer une résistance inconnue ou les jauges de contrainte.

@page Résistance d'une LED
@card Résistance série
@tags led
@math R=\frac{V_{CC}-n\,V_F}{I_F}\qquad P_R=R\,I_F^2
@table[lc] \text{Couleur} & V_F\ \text{typique}
@row \text{infrarouge} & 1{,}3\ \text{V}
@row \text{rouge} & 2{,}0\ \text{V}
@row \text{jaune, verte} & 2{,}1\text{ à }2{,}2\ \text{V}
@row \text{bleue, blanche} & 3{,}0\text{ à }3{,}3\ \text{V}
@note Prendre la valeur normalisée supérieure (courant légèrement plus faible) et une résistance de puissance au moins double.

@page Décibels
@card Définitions
@tags decibel
@math G_{\text{dB}}=20\log\frac{V_s}{V_e}=10\log\frac{P_s}{P_e}
@math \frac{V_s}{V_e}=10^{G/20}\qquad\frac{P_s}{P_e}=10^{G/10}
@card Repères
@tags repere
@table[ccc] \text{dB} & \text{tension} & \text{puissance}
@row +3 & \times1{,}41 & \times2
@row +6 & \times2 & \times4
@row +20 & \times10 & \times100
@row -3 & \times0{,}707 & \times0{,}5
@note Les gains en dB s'ajoutent quand on met des étages en cascade.

@page Niveaux dBm, dBV, dBu
@card Niveaux absolus
@tags dbm dbv dbu
@math L_{\text{dBm}}=10\log\frac{P}{1\ \text{mW}}\qquad L_{\text{dBW}}=10\log\frac{P}{1\ \text{W}}
@math L_{\text{dBV}}=20\log\frac{U_{\text{eff}}}{1\ \text{V}}\qquad L_{\text{dBu}}=20\log\frac{U_{\text{eff}}}{0{,}775\ \text{V}}
@note 0 dBu = 0,775 V : tension qui donne 1 mW dans 600 Ω. $L_{\text{dBu}}=L_{\text{dBV}}+2{,}21$ dB.
@card Sinusoïde
@tags crete efficace
@math U_{\max}=U_{\text{eff}}\sqrt2\qquad U_{cc}=2\sqrt2\,U_{\text{eff}}

@page Dissipateur thermique
@card Loi d'Ohm thermique
@tags thermique
@math T_j=T_a+P\left(R_{th\,jc}+R_{th\,cs}+R_{th\,sa}\right)
@text La puissance joue le rôle d'un courant, les températures celui des potentiels et les résistances thermiques (en °C/W) s'ajoutent en série.
@card Choisir le dissipateur
@tags dissipateur radiateur
@math R_{th\,sa}\le\frac{T_{j\max}-T_a}{P}-R_{th\,jc}-R_{th\,cs}
@note Sans dissipateur : $T_j=T_a+P\,R_{th\,ja}$ (notice). Garder une marge sur $T_{j\max}$.

@page Énergie stockée
@card Condensateur
@tags condensateur energie charge
@math W_C=\tfrac12CU^2\qquad Q=CU
@card Bobine
@tags bobine energie flux
@math W_L=\tfrac12LI^2\qquad\Phi=LI
@note La tension d'un condensateur et le courant d'une bobine sont continus : ils ne peuvent pas changer instantanément.
)TEX";
