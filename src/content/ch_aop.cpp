// Fiches : amplificateurs operationnels.
extern const char CH_AOP[];
const char CH_AOP[] = R"TEX(
@page Amplificateur inverseur
@card Fonction de transfert
@tags inverseur
@math H=\frac{V_s}{V_e}=-\frac{Z_2}{Z_1}\qquad Z_e=Z_1
@text Avec des résistances : $A_v=-\dfrac{R_2}{R_1}$ (gain réel, déphasage 180°).
@card Hypothèses (AOP idéal)
@tags ideal contre reaction
@li Courants d'entrée nuls : $i_+=i_-=0$.
@li Régime linéaire : $V_+=V_-$ (contre-réaction sur l'entrée −).
@li Saturation : $|V_s|\le V_{sat}$, sinon la sortie vaut $\pm V_{sat}$.
@card Impédances usuelles
@tags impedance condensateur bobine
@math Z_R=R\qquad Z_C=\frac{1}{jC\omega}\qquad Z_L=jL\omega
@math Z_{R+C}=R+\frac{1}{jC\omega}\qquad Z_{R\parallel C}=\frac{R}{1+jRC\omega}
@note Exemple : $Z_2=R_2\parallel C_2$ donne un passe-bas actif de gain $-R_2/R_1$ et de coupure $1/(2\pi R_2C_2)$.

@page Amplificateur non inverseur
@card Fonction de transfert
@tags non inverseur
@math H=1+\frac{Z_2}{Z_1}\qquad A_v=1+\frac{R_2}{R_1}
@text $Z_1$ relie l'entrée − à la masse, $Z_2$ l'entrée − à la sortie. Impédance d'entrée infinie (AOP idéal).
@note Le gain est toujours au moins égal à 1 avec des résistances.

@page Suiveur
@card Montage suiveur
@tags suiveur tampon
@math V_s=V_e\qquad Z_e\to\infty\qquad Z_s\to0
@text Sert d'adaptateur d'impédance : la source n'est pas chargée, la charge est alimentée par l'AOP.
@note Courant de sortie limité (quelques dizaines de mA pour un AOP courant).

@page Sommateur inverseur
@card Tension de sortie
@tags somme additionneur
@math V_s=-R_f\left(\frac{V_1}{R_1}+\frac{V_2}{R_2}+\frac{V_3}{R_3}\right)
@text L'entrée − est une masse virtuelle : les courants $V_k/R_k$ s'ajoutent dans $R_f$.
@note Résistances égales : $V_s=-\dfrac{R_f}{R}\,(V_1+V_2+V_3)$.

@page Soustracteur
@card Tension de sortie
@tags soustracteur differentiel
@math V_s=\frac{R_4}{R_3+R_4}\left(1+\frac{R_2}{R_1}\right)V_2-\frac{R_2}{R_1}V_1
@text Si $R_3=R_1$ et $R_4=R_2$ :
@math V_s=\frac{R_2}{R_1}\,(V_2-V_1)
@card Mode commun
@tags mode commun trmc
@math V_d=V_2-V_1\qquad V_{mc}=\frac{V_1+V_2}{2}
@math V_s=A_dV_d+A_{mc}V_{mc}\qquad\text{TRMC}=20\log\left|\frac{A_d}{A_{mc}}\right|
@note Un défaut d'appairage des résistances crée un gain de mode commun.

@page Intégrateur
@card Intégrateur idéal
@tags integrateur
@math v_s(t)=-\frac{1}{R_1C}\int v_e\,dt\qquad H=-\frac{1}{jR_1C\omega}
@math |H|=\frac{f_0}{f}\qquad f_0=\frac{1}{2\pi R_1C}\qquad\varphi=+90°
@note $\varphi=+90°$ ici car le signe − ajoute 180° au −90° de l'intégration.
@card Intégrateur pratique
@tags fuite derive
@text Une résistance $R_2$ en parallèle sur $C$ limite le gain en continu (sinon la moindre tension de décalage sature l'AOP) :
@math H=-\frac{R_2/R_1}{1+jR_2C\omega}\qquad A_0=-\frac{R_2}{R_1}\qquad f_c=\frac{1}{2\pi R_2C}
@note Le montage intègre pour $f\gg f_c$.

@page Dérivateur
@card Dérivateur idéal
@tags derivateur
@math v_s(t)=-R_2C_1\,\frac{dv_e}{dt}\qquad H=-jR_2C_1\omega
@card Dérivateur pratique
@tags bruit stabilite
@text Une résistance $R_1$ en série avec $C_1$ limite le gain aux hautes fréquences (bruit, stabilité) :
@math H=-\frac{jR_2C_1\omega}{1+jR_1C_1\omega}\qquad A_\infty=-\frac{R_2}{R_1}
@math f_c=\frac{1}{2\pi R_1C_1}
@note Le montage dérive pour $f\ll f_c$.

@page Trigger de Schmitt inverseur
@card Seuils
@tags schmitt hysteresis seuil
@text $V_e$ sur l'entrée −, $R_1$ entre l'entrée + et $V_{ref}$, $R_2$ entre l'entrée + et la sortie :
@math V_+=\frac{R_2V_{ref}+R_1V_s}{R_1+R_2}
@math V_{T+}=\frac{R_2V_{ref}+R_1V_{sat}^+}{R_1+R_2}\qquad V_{T-}=\frac{R_2V_{ref}+R_1V_{sat}^-}{R_1+R_2}
@math \Delta V=\frac{R_1}{R_1+R_2}\left(V_{sat}^+-V_{sat}^-\right)
@card Fonctionnement
@tags cycle
@li $V_s=V_{sat}^+$ tant que $V_e<V_{T+}$ ;
@li $V_s$ bascule à $V_{sat}^-$ quand $V_e$ dépasse $V_{T+}$ ;
@li $V_s$ revient à $V_{sat}^+$ quand $V_e$ redescend sous $V_{T-}$.
@note L'hystérésis rend le comparateur insensible au bruit dont l'amplitude est inférieure à $\Delta V$.

@page Trigger de Schmitt non inverseur
@card Seuils
@tags schmitt hysteresis seuil
@text $V_e$ par $R_1$ sur l'entrée +, $R_2$ entre l'entrée + et la sortie, $V_{ref}$ sur l'entrée − :
@math V_+=\frac{R_2V_e+R_1V_s}{R_1+R_2}
@math V_{T+}=\frac{(R_1+R_2)V_{ref}-R_1V_{sat}^-}{R_2}
@math V_{T-}=\frac{(R_1+R_2)V_{ref}-R_1V_{sat}^+}{R_2}
@math \Delta V=\frac{R_1}{R_2}\left(V_{sat}^+-V_{sat}^-\right)
@note La sortie passe à l'état haut quand $V_e$ dépasse $V_{T+}$, à l'état bas quand $V_e$ passe sous $V_{T-}$.

@page Amplificateur d'instrumentation
@card Gain
@tags instrumentation gain
@math G=\left(1+\frac{2R_1}{R_G}\right)\frac{R_3}{R_2}\qquad V_s=G\,(V_2-V_1)
@text Le gain se règle avec une seule résistance $R_G$. Les entrées ont une impédance infinie.
@card Étage d'entrée
@tags etage entree
@math V_{A1}=V_1-\frac{R_1}{R_G}(V_2-V_1)\qquad V_{A2}=V_2+\frac{R_1}{R_G}(V_2-V_1)
@note Le mode commun traverse l'étage d'entrée avec un gain 1 et est rejeté par le soustracteur.

@page Convertisseur courant-tension
@card Transimpédance
@tags courant tension photodiode
@math V_s=-R\,I_e\qquad\frac{V_s}{I_e}=-R
@text L'entrée − est une masse virtuelle : le capteur (photodiode) travaille à tension nulle.
@card Condensateur de compensation
@tags bande passante
@math Z=\frac{R}{1+jRC\omega}\qquad f_c=\frac{1}{2\pi RC}
)TEX";
