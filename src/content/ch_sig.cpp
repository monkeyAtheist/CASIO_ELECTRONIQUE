// Fiches : traitement du signal.
extern const char CH_SIG[];
const char CH_SIG[] = R"TEX(
@page Échantillonnage et repliement
@card Théorème de Shannon
@tags shannon nyquist
@math f_e>2f_{\max}\qquad f_N=\frac{f_e}{2}
@text Le spectre du signal échantillonné est périodique, de période $f_e$ : une raie à $f$ réapparaît aux fréquences $|f\pm kf_e|$.
@card Fréquence apparente
@tags repliement aliasing
@math f_a=\left|f-k\,f_e\right|\qquad k=\operatorname{arrondi}\left(\frac{f}{f_e}\right)
@note Un filtre anti-repliement (passe-bas) avant le CAN supprime les composantes au-delà de $f_e/2$.

@page Convertisseur analogique-numérique
@card Quantification
@tags quantum lsb code
@math q=\frac{V_{ref+}-V_{ref-}}{2^n}\qquad N=\left\lfloor\frac{V_e-V_{ref-}}{q}\right\rfloor
@math V_N=V_{ref-}+N\,q\qquad|\varepsilon|\le q\ (\text{troncature}),\ \frac{q}{2}\ (\text{arrondi})
@card Bruit et dynamique
@tags snr bruit quantification enob
@math \sigma_q=\frac{q}{\sqrt{12}}\qquad SNR=6{,}02\,n+1{,}76\ \text{dB}
@math ENOB=\frac{SINAD-1{,}76}{6{,}02}\qquad D=n\,f_e
@note Le SNR idéal suppose un sinus de pleine échelle et un bruit de quantification uniforme.

@page Acquisition et FFT
@card Résolution
@tags resolution duree points
@math T_a=\frac{N}{f_e}\qquad\Delta f=\frac{f_e}{N}=\frac{1}{T_a}\qquad f_k=k\,\Delta f
@card Fenêtres de pondération
@tags fenetre hann hamming blackman fuite
@math w(n)=\sum_k(-1)^ka_k\cos\frac{2\pi kn}{N}
@table[lccc] \text{Fenêtre} & ENBW & \text{lobe} & \text{feston.}
@row \text{rectangulaire} & 1{,}00 & -13\ \text{dB} & 3{,}9\ \text{dB}
@row \text{Hann} & 1{,}50 & -31\ \text{dB} & 1{,}4\ \text{dB}
@row \text{Hamming} & 1{,}36 & -43\ \text{dB} & 1{,}8\ \text{dB}
@row \text{Blackman} & 1{,}73 & -58\ \text{dB} & 1{,}1\ \text{dB}
@note Une raie qui ne tombe pas sur un canal « fuit » dans les canaux voisins : la fenêtre réduit cette fuite, au prix d'un lobe principal plus large.

@page Modulation d'amplitude
@card Signal et spectre
@tags am indice spectre
@math s(t)=V_p\left(1+m\cos\omega_mt\right)\cos\omega_pt\qquad m=\frac{V_{\max}-V_{\min}}{V_{\max}+V_{\min}}
@math \text{raies : }f_p\ (V_p),\quad f_p\pm f_m\ \left(\frac{mV_p}{2}\right)\qquad B=2f_m
@card Puissances
@tags puissance rendement
@math P_t=P_p\left(1+\frac{m^2}{2}\right)\qquad\eta=\frac{m^2}{2+m^2}\le33\ \%
@card Démodulation par détecteur d'enveloppe
@tags demodulation detecteur enveloppe
@math T_p\ll RC\le\frac{\sqrt{1-m^2}}{m\,\omega_m}
@note DBSP et BLU (sans porteuse) se démodulent par détection synchrone (multiplication par une porteuse locale).

@page Modulation de fréquence et de phase
@card Indice et bande
@tags indice carson excursion
@math \beta=\frac{\Delta f}{f_m}\qquad B_C=2(\Delta f+f_m)=2f_m(\beta+1)
@math s(t)=V_p\cos\left(\omega_pt+\beta\sin\omega_mt\right)
@card Raies de Bessel
@tags bessel spectre
@math s(t)=V_p\sum_{n=-\infty}^{+\infty}J_n(\beta)\cos\left(\omega_p+n\omega_m\right)t
@note La porteuse s'annule pour β = 2,405 ; 5,520 ; 8,654…
@card Phase
@tags pm phase
@math \beta=\Delta\varphi\qquad\Delta f=\beta f_m
@note Démodulation FM : discriminateur, boucle à verrouillage de phase (PLL) ou comptage de passages par zéro.

@page Modulations numériques
@card Débit et rapidité
@tags debit rapidite symbole
@math k=\log_2M\qquad R=\frac{D}{k}\qquad B=R(1+\alpha)
@card Taux d'erreur binaire
@tags teb ber erreur
@math \text{BPSK, QPSK : }TEB=Q\left(\sqrt{\frac{2E_b}{N_0}}\right)\qquad Q(x)=\frac12\operatorname{erfc}\frac{x}{\sqrt2}
@text $M$-QAM carrée :
@math TEB\approx\frac4k\left(1-\frac{1}{\sqrt M}\right)Q\left(\sqrt{\frac{3k\,E_b}{(M-1)N_0}}\right)
@math \frac{S}{N}=\frac{E_b}{N_0}\cdot\frac{D}{B}
@note Codage de Gray : deux symboles voisins ne diffèrent que d'un bit.

@page Valeurs moyenne et efficace
@card Définitions
@tags moyenne efficace rms
@math V_{moy}=\frac1T\int_0^Tv\,dt\qquad V_{eff}=\sqrt{\frac1T\int_0^Tv^2\,dt}
@math V_{eff}^2=V_{moy}^2+V_{eff,AC}^2
@card Formes usuelles (amplitude A)
@tags sinus carre triangle redresse
@table[lcc] \text{Forme} & V_{moy} & V_{eff}
@row \text{sinus} & 0 & A/\sqrt2
@row \text{carré}\ \pm A & 0 & A
@row \text{triangle} & 0 & A/\sqrt3
@row \text{redressé simple} & A/\pi & A/2
@row \text{redressé double} & 2A/\pi & A/\sqrt2
@row \text{rectangle}\ \alpha & \alpha A & A\sqrt\alpha

@page Filtre numérique du premier ordre
@card Moyenne exponentielle
@tags iir rc numerique
@math y_n=y_{n-1}+\alpha\left(x_n-y_{n-1}\right)\qquad\alpha=1-e^{-2\pi f_c/f_e}
@math H(z)=\frac{\alpha}{1-(1-\alpha)z^{-1}}
@card Moyenne glissante
@tags fir moyenne glissante
@math y_n=\frac1N\sum_{k=0}^{N-1}x_{n-k}\qquad f_0=\frac{f_e}{N}\qquad f_{-3\text{dB}}\approx0{,}443\,\frac{f_e}{N}
@note La moyenne glissante élimine complètement les fréquences multiples de $f_e/N$ (par exemple le 50 Hz secteur).
)TEX";
