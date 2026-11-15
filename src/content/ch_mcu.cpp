// Fiches : timers et microcontroleurs.
extern const char CH_MCU[];
const char CH_MCU[] = R"TEX(
@page Timer : délai entre interruptions
@card Fréquence de comptage
@tags prediviseur prescaler horloge tick
@math f_t=\frac{f_{clk}}{N_1N_2N_3}\qquad T_{tick}=\frac{1}{f_t}
@text Chaque front de $f_t$ incrémente le registre de comptage de $n$ bits (0 à $2^n-1$).
@card Mode débordement (overflow)
@tags debordement overflow offset prechargement
@math \text{ticks}=2^n-\text{offset}\qquad T=\text{ticks}\cdot T_{tick}
@note L'offset doit être rechargé dans la routine d'interruption (sinon le registre repart de 0).
@card Mode comparaison (CTC)
@tags comparaison plafond ctc ocr arr
@math \text{ticks}=\text{plafond}-\text{offset}+1\qquad f=\frac{1}{T}
@note Le registre est remis à zéro (ou à l'offset) automatiquement : pas de rechargement logiciel.
@card Exemple
@tags exemple
@math 16\ \text{MHz}\ \div2\ \div4=2\ \text{MHz}\qquad T_{tick}=0{,}5\ \mu\text{s}
@math n=8,\ \text{offset}=57:\ \text{ticks}=256-57=199\qquad T=99{,}5\ \mu\text{s}
@card Délai long
@tags compteur logiciel
@math T_{total}=k\,T
@text On compte $k$ interruptions dans une variable avant d'agir.

@page Timer : réglage d'un délai
@card Méthode
@tags reglage calcul prediviseur
@math N_{clk}=T\,f_{clk}\qquad\text{ticks}=\operatorname{arrondi}\left(\frac{N_{clk}}{N}\right)\le2^n
@text On choisit le plus petit prédiviseur $N$ disponible tel que le nombre de ticks tienne dans le registre : la résolution est alors la meilleure.
@li Débordement : $\text{offset}=2^n-\text{ticks}$.
@li Comparaison : $\text{plafond}=\text{ticks}-1$ (comptage depuis 0).
@li STM32 : $\text{PSC}=N-1$, $\text{ARR}=\text{ticks}-1$.
@card Erreur
@tags erreur precision
@math T_{\text{réel}}=\frac{\text{ticks}\cdot N}{f_{clk}}\qquad\varepsilon=\frac{T_{\text{réel}}-T}{T}

@page PWM
@card Fréquence
@tags frequence top
@math \text{rapide : }f_{PWM}=\frac{f_{clk}}{N(\text{TOP}+1)}\qquad\text{phase correcte : }f_{PWM}=\frac{f_{clk}}{2N\cdot\text{TOP}}
@card Rapport cyclique
@tags rapport cyclique ocr
@math \text{rapide : }\alpha=\frac{\text{OCR}+1}{\text{TOP}+1}\qquad\text{phase correcte : }\alpha=\frac{\text{OCR}}{\text{TOP}}
@math n=\log_2(\text{TOP}+1)\qquad V_{moy}=\alpha V_{CC}
@note La PWM à phase correcte est symétrique (utile pour les moteurs), mais de fréquence deux fois plus basse.

@page Liaison série (UART)
@card Registre de débit
@tags baud ubrr debit
@math \text{UBRR}=\operatorname{arrondi}\left(\frac{f_{clk}}{k\cdot\text{débit}}\right)-1\qquad k=16\ (\text{ou }8)
@math \text{débit réel}=\frac{f_{clk}}{k(\text{UBRR}+1)}\qquad\varepsilon=\frac{\text{réel}-\text{voulu}}{\text{voulu}}
@note Erreur tolérée : environ 2 %. Les quartz de 1,8432 ou 11,0592 MHz donnent des débits exacts.
@card Trame
@tags trame bit start stop parite
@text Repos à 1, un bit de départ à 0, les données (bit de poids faible d'abord), la parité éventuelle, puis 1 ou 2 bits de stop à 1.
@math T_b=\frac{1}{\text{débit}}\qquad T_{trame}=(1+n_d+n_p+n_s)\,T_b
)TEX";
