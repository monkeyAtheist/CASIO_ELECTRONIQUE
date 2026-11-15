// Fiches : code couleur et series E.
extern const char CH_CODE[];
const char CH_CODE[] = R"TEX(
@page Code couleur → valeur
@card Chiffres
@tags couleur chiffres
@table[clcl] 0 & \text{noir} & 5 & \text{vert}
@row 1 & \text{marron} & 6 & \text{bleu}
@row 2 & \text{rouge} & 7 & \text{violet}
@row 3 & \text{orange} & 8 & \text{gris}
@row 4 & \text{jaune} & 9 & \text{blanc}
@note Moyen mnémotechnique : « Ne Mangez Rien Ou Je Vous Brûle Votre Grande Barbe ».
@card Lecture
@tags lecture anneaux multiplicateur
@math R=\overline{ab}\times10^{m}\ \text{(4 anneaux)}\qquad R=\overline{abc}\times10^{m}\ \text{(5 anneaux)}
@text Multiplicateur : couleur du chiffre $m$, or $=10^{-1}$, argent $=10^{-2}$.
@card Tolérance
@tags tolerance
@table[cccc] \text{marron} & \text{rouge} & \text{vert} & \text{bleu}
@row \pm1\,\% & \pm2\,\% & \pm0{,}5\,\% & \pm0{,}25\,\%
@table[cccc] \text{violet} & \text{or} & \text{argent} & \text{aucun}
@row \pm0{,}1\,\% & \pm5\,\% & \pm10\,\% & \pm20\,\%

@page Valeur → code couleur
@card Méthode
@tags codage
@li Écrire la valeur avec 2 chiffres significatifs (3 pour 5 anneaux).
@li Le multiplicateur est le nombre de zéros à ajouter (or : ÷10, argent : ÷100).
@li Exemple : $4{,}7\ \text{k}\Omega=47\times10^2$ → jaune, violet, rouge.

@page Séries E (E6 à E96)
@card Principe
@tags serie normalisee
@math E_n:\ \text{valeurs}\approx10^{k/n},\ k=0\ldots n-1
@text Chaque valeur est environ $10^{1/n}$ fois la précédente : E12 → ×1,21 ; E24 → ×1,10.
@card Valeurs E12 (±10 %)
@tags e12
@text 1,0 – 1,2 – 1,5 – 1,8 – 2,2 – 2,7 – 3,3 – 3,9 – 4,7 – 5,6 – 6,8 – 8,2
@card Valeurs E24 (±5 %)
@tags e24
@text E12 plus : 1,1 – 1,3 – 1,6 – 2,0 – 2,4 – 3,0 – 3,6 – 4,3 – 5,1 – 6,2 – 7,5 – 9,1
@note E6 : 1,0 – 1,5 – 2,2 – 3,3 – 4,7 – 6,8. E48 (±2 %) et E96 (±1 %) : trois chiffres significatifs.

@page Marquages CMS et condensateurs
@card Code à 3 ou 4 chiffres
@tags cms code
@math \overline{abc}\to\overline{ab}\times10^{c}\qquad\overline{abcd}\to\overline{abc}\times10^{d}
@text Résistances en ohms : 472 = 4,7 kΩ ; 1002 = 10 kΩ. Condensateurs en picofarads : 104 = 100 nF ; 223 = 22 nF.
@note Lettre R = virgule (4R7 = 4,7 Ω). Chiffre final 8 ou 9 : ÷100 ou ÷10. Tolérance des condensateurs : J ±5 %, K ±10 %, M ±20 %.
)TEX";
