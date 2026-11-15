// Fiches : antennes et radiofrequences.
extern const char CH_RF[];
const char CH_RF[] = R"TEX(
@page Longueur d'onde
@card Relations
@tags lambda celerite frequence
@math \lambda_0=\frac{c}{f}\qquad c=299\,792\,458\ \text{m/s}\qquad\lambda=k\,\lambda_0
@math \lambda\,[\text{m}]\approx\frac{300}{f\,[\text{MHz}]}\qquad\beta=\frac{2\pi}{\lambda}
@note Dans un câble coaxial, l'onde est plus lente : $k\approx0{,}66$ (polyéthylène plein), 0,8 à 0,85 (mousse).
@card Bandes de fréquences
@tags bande hf vhf uhf
@table[ll] \text{Bande} & \text{Fréquences}
@row \text{LF, MF} & 30\ \text{kHz}\ \text{à}\ 3\ \text{MHz}
@row \text{HF} & 3\ \text{à}\ 30\ \text{MHz}
@row \text{VHF} & 30\ \text{à}\ 300\ \text{MHz}
@row \text{UHF} & 0{,}3\ \text{à}\ 3\ \text{GHz}
@row \text{SHF} & 3\ \text{à}\ 30\ \text{GHz}

@page Antennes filaires
@card Dipôle demi-onde
@tags dipole demi onde
@math L=K\,\frac{\lambda}{2}\qquad L\,[\text{m}]\approx\frac{142{,}5}{f\,[\text{MHz}]}\quad(K=0{,}95)
@math Z\approx73\ \Omega\qquad G=2{,}15\ \text{dBi}\ (0\ \text{dBd})
@note Dipôle replié : même longueur, $Z\approx4\times73\approx292\ \Omega$.
@card Quart d'onde (fouet)
@tags quart onde monopole radian plan sol
@math L=K\,\frac{\lambda}{4}\qquad Z\approx36\ \Omega\ (\text{plan de sol})\qquad Z\approx50\ \Omega\ (\text{radians à }45°)
@card Surface effective
@tags gain ouverture
@math A_e=\frac{G\lambda^2}{4\pi}\qquad G_{\text{dBi}}=G_{\text{dBd}}+2{,}15

@page Bilan de liaison
@card Formule de Friis
@tags friis bilan
@math P_r=P_e-L_e+G_e-\text{FSL}-L_{sup}+G_r-L_r
@note Puissances en dBm, gains en dBi, pertes en dB.
@math \text{PIRE}=P_e-L_e+G_e
@card Affaiblissement en espace libre
@tags fsl espace libre
@math \text{FSL}=20\log\frac{4\pi d f}{c}
@math \text{FSL}\approx32{,}44+20\log d_{\text{km}}+20\log f_{\text{MHz}}
@card Marge et portée
@tags marge portee sensibilite
@math M=P_r-S\qquad d_{\max}=d\cdot10^{M/20}
@note Doubler la distance coûte 6 dB. Prévoir une marge (évanouissements, obstacles) de 10 à 20 dB.

@page ROS et abaque de Smith
@card Coefficient de réflexion
@tags reflexion gamma
@math \Gamma=\frac{Z_L-Z_0}{Z_L+Z_0}\qquad\text{ROS}=\frac{1+|\Gamma|}{1-|\Gamma|}\qquad|\Gamma|=\frac{\text{ROS}-1}{\text{ROS}+1}
@card Pertes
@tags return loss desadaptation
@math RL=-20\log|\Gamma|\qquad ML=-10\log\left(1-|\Gamma|^2\right)
@card Impédance ramenée par une ligne
@tags ligne impedance ramenee quart onde
@math Z_e=Z_0\frac{Z_L+jZ_0\tan\beta\ell}{Z_0+jZ_L\tan\beta\ell}
@note Ligne quart d'onde : $Z_e=Z_0^2/Z_L$ (transformateur d'impédance). Demi-onde : $Z_e=Z_L$.

@page Adaptation par réseau en L
@card Calcul
@tags adaptation l q
@math Q=\sqrt{\frac{R_{haute}}{R_{basse}}-1}\qquad X_s=Q\,R_{basse}\qquad X_p=\frac{R_{haute}}{Q}
@math L=\frac{X}{\omega}\qquad C=\frac{1}{\omega X}
@note L'élément série se place du côté de la résistance la plus faible, l'élément parallèle du côté de la plus forte. La bande passante diminue quand le rapport des résistances augmente.

@page Bruit thermique et sensibilité
@card Bruit thermique
@tags ktb bruit thermique
@math N=kTB\qquad N\,[\text{dBm}]=-174+10\log B\quad(T=290\ \text{K})
@math v_n=\sqrt{4kTRB}\qquad k=1{,}381\cdot10^{-23}\ \text{J/K}
@card Sensibilité
@tags sensibilite facteur bruit
@math S\,[\text{dBm}]=-174+10\log B+NF+SNR_{\min}
@card Formule de Friis (bruit)
@tags friis cascade
@math F=F_1+\frac{F_2-1}{G_1}+\frac{F_3-1}{G_1G_2}\qquad NF=10\log F
@note Le premier étage (préamplificateur à faible bruit) fixe l'essentiel du bruit.

@page Horizon radio et zone de Fresnel
@card Horizon
@tags horizon visibilite
@math d\approx3{,}57\left(\sqrt{h_1}+\sqrt{h_2}\right)\ \text{km (optique)}
@math d\approx4{,}12\left(\sqrt{h_1}+\sqrt{h_2}\right)\ \text{km (radio, }k=4/3)
@note Hauteurs en mètres.
@card Zone de Fresnel
@tags fresnel degagement
@math r_1=\sqrt{\frac{\lambda\,d_1d_2}{d_1+d_2}}\qquad r_{1,\text{milieu}}=\frac12\sqrt{\lambda d}
@math h_t=\frac{d^2}{8kR}\qquad R=6371\ \text{km}
@note Laisser libre au moins 60 % de la première zone de Fresnel.
)TEX";
