# Électronique — calculateurs d'électronique pour Casio Graph 90+E

Application gint (fxSDK) qui réunit **96 calculateurs d'électronique** classés
en 17 sections. Chaque calculateur affiche **le schéma du montage** (symboles
normalisés, composant sélectionné mis en couleur), **les champs de saisie et
les résultats**, une **fiche de formules** rendue par le moteur de formules de
Formulaire, et selon le cas un **diagramme de Bode** avec curseur, un cycle
d'hystérésis ou des chronogrammes.

## Contenu (17 sections, 96 calculateurs)

| Section | Calculateurs |
|---|---|
| Associations | résistances, condensateurs, inductances en série ou en parallèle (jusqu'à 12, on en ajoute ou on en retire), résistance à associer pour obtenir une valeur cible |
| RC, RL, RLC | circuit RC, circuit RL, circuit LC (f₀, ou C, ou L), RLC série (f₀, Q, ξ, bande passante, f₁, f₂, surtension), RLC parallèle, charge et décharge d'un condensateur (courbe) |
| Filtres | RC et RL du 1er ordre (passe-bas / passe-haut), RLC du 2e ordre (sortie sur C, L, R ou L+C : passe-bas, passe-haut, passe-bande, coupe-bande), Sallen-Key passe-bas et passe-haut (gain unité ou K = 1 + Rb/Ra) — tous avec Bode |
| AOP | inverseur et non-inverseur avec **Z₁, Z₂ quelconques** (R, C, L, R+C, R∥C, R+L, R∥L), suiveur, sommateur (2 ou 3 entrées), soustracteur (TRMC), intégrateur et dérivateur (idéaux ou pratiques), triggers de Schmitt inverseur et non inverseur (cycle d'hystérésis), amplificateur d'instrumentation, convertisseur courant-tension |
| Bipolaires | polarisation par pont diviseur (NPN/PNP, bloqué / linéaire / saturé, petits signaux), polarisation par résistance de base, transistor en commutation (R_B pour saturer) ; **régime dynamique** : émetteur commun, collecteur commun, base commune (point de repos, modèle en π, A_v, A_vg, A_i, G_p, Z_e, Z_s, coupures dues à chaque condensateur, f_b et f_h exactes, Bode, **schéma équivalent petits signaux**), paire différentielle (A_d, A_mc, TRMC), paramètres hybrides h et modèle en π |
| MOSFET, JFET | MOSFET canal N/P (pont diviseur, 3 régimes, g_m, A_v), JFET canal N/P (autopolarisation ou pont), MOSFET en commutation (pertes, T_j) ; **régime dynamique** : source, drain et grille communes (JFET ou MOSFET, mêmes résultats et Bode) |
| Lois de base | loi d'Ohm et puissance (2 grandeurs connues parmi U, I, R, P), diviseur de tension (avec charge, calcul de R₂), pont de Wheatstone, résistance d'une LED, décibels, niveaux dBm/dBW/dBV/dBu, dissipateur thermique, énergie stockée |
| Alimentations | LM317 (V_s ou R₂), régulation Zener, redressement (simple alternance, pont de Graetz, point milieu) et filtrage (chronogrammes) |
| Code couleur | anneaux → valeur (4 ou 5 anneaux, dessin de la résistance), valeur → anneaux, séries E6 à E96, marquages CMS et condensateurs |
| Oscillateurs | NE555 astable (chronogrammes) et monostable, multivibrateur à AOP (chronogrammes), pont de Wien (Bode du réseau β), Colpitts et Hartley |
| Quadripôles | conversion entre matrices Z, Y, H, G, T (ABCD) et S, valeurs réelles ou complexes, réciprocité et symétrie, quadripôle chargé (Z_e, Z_s, A_v, A_i) ; cellules en T, Π, L et L inversé avec impédances quelconques (matrices, impédances images, Bode) ; atténuateurs en T et en Π (impédances égales ou différentes, valeurs E24/E96) |
| Transfos, bobines | transformateur idéal, transformateur réel (modèle de Kapp : chute de tension, pertes, rendement, courant à vide), dimensionnement (spires par volt, fils), inductances couplées (M, k, associations, modèle en T, énergie), inductance de mode commun (impédance, résonance, atténuation, Bode), inductance d'une bobine (solénoïde, tore, A_L) |
| Logique | table de vérité d'une expression booléenne et test d'équivalence de deux expressions, formes canoniques Σm / ΠM, formes simplifiées minimales ; tableau de Karnaugh (2 à 4 variables, cases indifférentes, regroupements dessinés) ; portes logiques et réalisation en NON-ET ou NON-OU seules (symboles américains ou CEI, niveaux logiques en couleur) ; bases et codes (binaire, hexadécimal, octal, Gray, BCD, complément à 2) ; fiches d'algèbre de Boole et de De Morgan |
| Timers, µC | timer : horloge → 3 diviseurs → registre de n bits avec **offset** et **plafond** → interruption (ticks, délai, fréquence, chronogramme) ; réglage d'un délai (prédiviseur AVR, PIC, STM32… et registre) ; PWM ; liaison série (UBRR, erreur, trame) |
| Opto, moteurs | optocoupleur (R d'entrée, CTR, charge de sortie), moteur à courant continu (point de fonctionnement, courbes C, P, η), moteur pas à pas (micropas, vitesse, pas/mm, vitesse maximale), moteur asynchrone triphasé |
| Antennes, RF | longueur d'onde, antennes filaires (dipôle, quart d'onde), **bilan de liaison** (PIRE, affaiblissement en espace libre, marge, portée, courbe P_r(d)), ROS et abaque de Smith, adaptation par réseau en L, bruit et sensibilité (Friis), horizon radio et zone de Fresnel |
| Signal | échantillonnage et repliement (chronogramme, spectre), CAN/CNA, acquisition et FFT (fenêtres, fuite spectrale), modulation d'amplitude et détecteur d'enveloppe, modulation de fréquence et de phase (Carson, raies de Bessel), modulations numériques (constellations, TEB), valeurs moyenne et efficace, filtre numérique du 1er ordre (Bode) |

Toutes les formules ont été vérifiées numériquement par des méthodes
indépendantes (2 964 vérifications, voir « Tests »).

## Compiler et installer

```sh
cd ELECTRONIQUE_GRAPH90E
fxsdk build-cg
```

Copier `Electronique.g3a` à la racine de la mémoire de stockage de la
calculatrice (câble USB, mode « Clé USB »). L'application ne dépend que de
gint : pas de CASIO_GUI, pas de libm, aucun fichier à installer à côté.

## Utilisation

| Touche | Action |
|---|---|
| Flèches + **EXE** | accueil : choisir une section, puis un calculateur |
| **↑ ↓** | choisir une ligne ; le composant correspondant passe en couleur sur le schéma |
| chiffres, **.**, **×10ˣ**, **(−)** | saisir une valeur (directement sur la ligne, ou **EXE** d'abord) |
| **F1 … F6** pendant la saisie | valider avec un préfixe : p, n, µ, m, k, M (**SHIFT** : f, G, T) ; **EXE** valide sans préfixe |
| **← →** sur une valeur | valeur normalisée suivante (E24 pour R, E12 pour C et L, 1-2-5 pour f…) |
| **← →** ou **EXE** sur un choix | changer (type d'impédance, NPN/PNP, passe-bas/passe-haut…) |
| **F1** | fiche de formules du calculateur (**← →** : autres fiches de la section, **F1** : calculer, **F5** : zoom) |
| **F2 F3** | calculateur précédent / suivant |
| **F4** | Bode / graphe (associations : ajouter un composant) |
| **F5** | deux fois : valeurs par défaut (associations : retirer un composant, aussi **DEL**) |
| **EXIT** / **F6** | retour à la liste, puis à l'accueil |
| **F4** à l'accueil | recherche (lettres rouges sans ALPHA) |
| **F5** à l'accueil | aide |
| **MENU** | menu de la calculatrice ; en revenant, on retrouve l'écran et toutes les valeurs |
| Expression logique | **F1–F4** : A B C D, **F5** : NON, **F6** : ⊕, **×** : ET, **+** : OU, **( )**, **SHIFT F1/F2** : 0/1, **SHIFT F5/F6** : NON-ET/NON-OU |
| Karnaugh | **0**, **1** ou **X,θ,T** sur une case : valeur et case suivante |

**Bode** : **← →** déplacent le curseur (avec **SHIFT** : plus vite), **↑ ↓** ou
**F3 F4** zooment, **F1 F2** décalent d'une décade, **F5** reporte la fréquence
du curseur dans le champ f du calculateur. La ligne du haut donne f, |H| en dB
et en valeur, et φ.

### Conventions

- Composants idéaux ; AOP idéal limité par ±V_sat (un message signale la saturation).
- Régime sinusoïdal : amplitudes ou valeurs efficaces, au choix, de façon cohérente.
- Virgule décimale française ; préfixes SI ; flèches de tension bleues ;
  valeurs bleues sur les schémas = potentiels des nœuds.
- Messages : ✓ en vert (fonctionnement normal), ⚠ en orange ou rouge
  (saturation, blocage, valeur impossible…). Une ligne de message choisie
  s'affiche en entier dans la barre d'aide.

## Organisation du code

```
src/main.cpp            boucle principale (MENU géré comme CASIO_GAME / EM_FIELD / Formulaire)
src/num/                FMath (sqrt, exp, ln, sin, atan2… maison), complexes, formatage SI, saisie, séries E
src/calc/               modèle des calculateurs (Calc.hpp), registre, une section par fichier sec_*.cpp
                        (sec_amp : moteur nodal petits signaux ; sec_logic : expressions, Quine-McCluskey)
src/sch/                symboles des schémas (Sch), icônes de l'accueil
src/ui/                 App : accueil, liste, calculateur et saisie, formules, Bode/graphes, recherche
src/content/            fiches de formules (une par section) et aide
src/gfx/ src/font/      Canvas (+ traits, disques, triangles anticrénelés), polices, petites formules (Tex)
src/math/ src/doc/      moteur de formules et mise en page des fiches (repris de Formulaire)
src/gen/                polices générées (ne pas modifier à la main)
tools/fontgen/          polices (gen_fonts.py), icônes, typographie des fiches
tools/host_sim/         simulateur PC (faux gint, clavier scripté, captures, fuzz)
tools/tests/            vérification des fiches et des écrans, tests numériques
```

### Un calculateur

Chaque calculateur est un petit espace de noms dans `src/calc/sec_*.cpp` :

```cpp
namespace inv {
enum { R1, C1, L1, R2, C2, L2, F, VE, VSAT, OH, ODB, OPHI, OAV, OVS, OZE };  // cases de valeurs
static void def(CalcState &s);                  // valeurs par défaut
static void rows(const CalcState &s, RowList &r);   // lignes : r.in(), r.choice(), r.out(), r.msg()
static void calc(CalcState &s);                 // calcul des résultats
static void sch(Sch &g, const CalcState &s);    // schéma (g.res, g.cap, g.opamp, g.bjt…)
static bool tf(const CalcState &s, double f, Cx &H);   // fonction de transfert (Bode), facultatif
}
```

puis une entrée dans le tableau de la section (titre, sous-titre, mots clés,
section, type de graphe, fonctions). La fiche de formules est la page
`@page` **du même titre** dans `src/content/ch_*.cpp` (format de Formulaire :
`@card`, `@math`, `@text`, `@li`, `@note`, `@table`/`@row`, formules en LaTeX).

Autres lignes : `r.outC()` (résultat complexe, a + jb ou module ∠ argument),
`r.outI()` (entier exact), `r.texOut()` (résultat sous forme de formule, par
exemple une expression simplifiée), `r.text()` (saisie d'un texte, avec un
clavier `TextKeys` propre au calculateur : expressions logiques).

Les étiquettes des lignes sont en LaTeX (`"R_1"`, `"V_{sat}"`, `"\\varphi"`) ;
une étiquette qui commence par `#` est du texte simple, où `X_Y` ou `X_{YZ}`
s'écrit en indice. Le `tag` d'une ligne relie le champ au composant du schéma
mis en couleur.

### Le noyau numérique

La Graph 90+E n'a pas d'unité de calcul flottant : les `double` sont émulés par
libgcc. `src/num/FMath.cpp` fournit sqrt, exp, ln, log10, pow10, sin, cos, atan,
atan2 et hypot (erreur relative < 6·10⁻¹⁶ face à la libm du PC) : mêmes
résultats sur le simulateur et sur la calculatrice, et aucune dépendance à
OpenLibm. Les valeurs s'affichent avec 4 chiffres significatifs et le préfixe
SI adapté (« 4,7 kΩ », « 1,592 kHz », « −3,01 dB »).

## Tests

- `tools/tests/run_tests.sh` : tests du noyau numérique (contre libm), mise en
  page de toutes les fiches (erreurs LaTeX, largeur, glyphes, capacités), puis
  dessin de **chaque écran de chaque calculateur** (chaque ligne, chaque valeur
  de chaque choix, saisie, Bode avec zoom, fiche). Option `-png DIR`.
- `tools/tests/verify_calcs.py` (+ `verify_ext.py`) : 2 964 vérifications
  indépendantes sur 674 cas, souvent aléatoires : analyse nodale (numpy) des
  montages à AOP, des filtres et des Sallen-Key ; analyse nodale modifiée
  (sources nulles pour les courts-circuits) des schémas petits signaux, avec
  recherche numérique de f_b et f_h ; formules classiques de passage entre
  matrices de quadripôles ; tables de vérité et recouvrements minimaux trouvés
  par recherche exhaustive ; FFT numpy pour les fenêtres, scipy pour Bessel et
  erfc, intégration numérique des valeurs efficaces ; intégration RK4 de la
  charge RC et du multivibrateur ; séries E retapées d'après la norme.
- `tools/tests/shots.py SORTIE` : capture de chaque calculateur et de son graphe.
- `tools/host_sim/build.sh` puis `tools/host_sim/electronique_sim script.txt`
  (`fuzz 60000 graine` : touches au hasard ; plus de 330 000 touches sans
  erreur ASan/UBSan, dans toutes les sections).

## Contraintes respectées

Aucune allocation dynamique, aucun constructeur global, pas de `std::`, pas
d'exceptions ni de RTTI, pas de libm. Mémoire statique : états des 96
calculateurs (~55 Ko, place pour 104), listes d'affichage des fiches (~64 Ko),
zones du moteur de formules (~56 Ko). Polices : ~500 Ko en ROM.

## Licences

Polices DejaVu Sans et DejaVu Math TeX Gyre : licences libres (voir
`licenses/DejaVu-LICENSE.txt`). Moteur de formules, polices pré-rendues et
mise en page des fiches repris de Formulaire.
