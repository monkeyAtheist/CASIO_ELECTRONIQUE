// Chapitre cache : aide (touche F5 de l'accueil).
extern const char CH_AIDE[];
const char CH_AIDE[] = R"TEX(
@page Utilisation
@card Se déplacer
@li **Accueil** : flèches puis **EXE** pour ouvrir une section, puis **EXE** sur un calculateur.
@li **↑ ↓** : choisir une ligne. Le composant correspondant s'affiche en couleur sur le schéma.
@li **F2 F3** : calculateur précédent / suivant de la section. **EXIT** : retour à la liste.
@li **F1** : fiche des formules du calculateur ; **← →** parcourt les fiches de la section, **F1** y ouvre le calculateur.
@card Saisir une valeur
@li Taper directement les chiffres sur une ligne de valeur (ou **EXE** pour modifier).
@li **F1 à F6** valident avec un préfixe : p, n, µ, m, k, M. **SHIFT** puis F1, F5, F6 : f, G, T.
@li **EXE** valide sans préfixe. **×10^x** (EXP) pour une puissance de 10, **(−)** pour le signe, **DEL** et **AC** pour corriger, **EXIT** pour annuler.
@li **← →** sur une valeur : valeur normalisée suivante (séries E24 pour les résistances, E12 pour les condensateurs et les bobines).
@li **← →** ou **EXE** sur un choix (type d'impédance, NPN/PNP…) : changer.
@card Saisir une expression logique
@li **EXE** sur la ligne F (ou G) ouvre la saisie ; la formule s'affiche avec ses barres pendant la frappe.
@li **F1 à F4** : variables A, B, C, D ; **F5** : NON (barre sur le facteur suivant) ; **F6** : OU exclusif.
@li **×** : ET (ou deux variables accolées) ; **+** : OU ; **( )** : parenthèses.
@li **SHIFT** puis **F1 F2** : constantes 0 et 1 ; **SHIFT** puis **F5 F6** : NON-ET (↑) et NON-OU (↓).
@li **DEL** efface le dernier caractère, **AC** tout, **EXE** valide, **EXIT** annule.
@card Tableau de Karnaugh
@li Sur une case : **0**, **1** ou **X,θ,T** (indifférent) donnent la valeur et passent à la case suivante.
@li La case choisie est surlignée dans le tableau ; **F4** agrandit le tableau avec ses regroupements.
@card Graphes
@li **F4** : diagramme de Bode, cycle d'hystérésis ou signaux selon le calculateur.
@li Bode : **← →** déplacent le curseur (**SHIFT** : plus vite), **↑ ↓** ou **F3 F4** zooment, **F1 F2** décalent d'une décade, **F5** reporte la fréquence du curseur dans le calculateur.
@card Autres touches
@li **F5** deux fois : revenir aux valeurs par défaut du calculateur.
@li Associations : **F4** ajoute un composant après la ligne choisie, **F5** ou **DEL** retire le composant choisi (12 au maximum).
@li **F4** à l'accueil ou dans une liste : recherche par mots (lettres rouges sans ALPHA).
@li **MENU** : menu de la calculatrice ; en revenant, on retrouve l'écran et les valeurs.

@page Conventions
@card Composants et unités
@li Composants idéaux ; AOP idéal (courants d'entrée nuls, gain infini) limité par $\pm V_{sat}$.
@li Régime sinusoïdal : les tensions sont des amplitudes ou des valeurs efficaces, au choix, du moment qu'on garde la même convention.
@li Virgule décimale française ; préfixes SI (µ = micro, m = milli…).
@card Schémas
@li Symboles normalisés (résistance rectangulaire) ; flèches de tension bleues, pointées vers le potentiel de référence positif.
@li Valeurs en bleu sur les schémas : potentiels des nœuds par rapport à la masse.
@card Messages
@li ✓ en vert : fonctionnement normal. ⚠ en orange ou rouge : saturation, blocage, valeur impossible…
@li Une ligne de message choisie s'affiche en entier dans la barre d'aide.
@card Résultats complexes
@li Quadripôles, ROS : notation $a+jb$ ou module et argument (choix « Notation »).
@li Une barre de couleur au bout d'une ligne signale une formule trop longue : **F4** l'affiche en entier quand c'est possible.

@page À propos
@card Électronique 1.0
@text Calculateurs d'électronique pour Graph 90+E (gint) : 10 sections, 53 calculateurs avec schémas, diagrammes de Bode et fiches de formules.
@text Calculs en double précision avec des fonctions mathématiques maison (aucune dépendance à une bibliothèque mathématique), formules affichées par le moteur de Formulaire.
@note Polices DejaVu et DejaVu Math TeX Gyre : licences libres (voir README).
)TEX";
