// Fiches : logique combinatoire.
extern const char CH_LOGIC[];
const char CH_LOGIC[] = R"TEX(
@page Table de vérité et équivalence
@card Saisie des expressions
@tags saisie clavier
@li Variables : F1 à F4 (A, B, C, D) ; constantes : SHIFT F1 (0), SHIFT F2 (1).
@li NON : F5 (barre sur le facteur suivant) ; ET : × ou juxtaposition ($AB$) ; OU : + ; OU exclusif : F6.
@li NON-ET : SHIFT F5 ($\uparrow$) ; NON-OU : SHIFT F6 ($\downarrow$) ; parenthèses : touches ( et ).
@note Priorités : NON, puis ET, puis OU exclusif, puis OU, puis NON-ET et NON-OU. En cas de doute, mettre des parenthèses.
@card Équivalence de deux expressions
@tags equivalence verification
@text Deux expressions sont équivalentes si leurs tables de vérité sont identiques. Saisir F et G : le calculateur indique $F\equiv G$ ou donne une combinaison où elles diffèrent (lignes en rouge dans la table).

@page Algèbre de Boole
@card Propriétés de base
@tags propriete neutre absorbant idempotence complement
@table[ll] \text{ET (produit)} & \text{OU (somme)}
@row A\cdot1=A & A+0=A
@row A\cdot0=0 & A+1=1
@row A\cdot A=A & A+A=A
@row A\cdot\overline{A}=0 & A+\overline{A}=1
@row \overline{\overline{A}}=A & \overline{\overline{A}}=A
@card Commutativité, associativité, distributivité
@tags commutativite associativite distributivite
@math A\cdot B=B\cdot A\qquad A+B=B+A
@math A(B+C)=AB+AC\qquad A+BC=(A+B)(A+C)
@card Absorption et simplification
@tags absorption simplification theoreme
@math A+AB=A\qquad A(A+B)=A
@math A+\overline{A}B=A+B\qquad A(\overline{A}+B)=AB
@math AB+A\overline{B}=A\qquad(A+B)(A+\overline{B})=A
@card Consensus
@tags consensus
@math AB+\overline{A}C+BC=AB+\overline{A}C
@math (A+B)(\overline{A}+C)(B+C)=(A+B)(\overline{A}+C)

@page Théorèmes de De Morgan et opérateurs
@card Théorèmes de De Morgan
@tags de morgan complement
@math \overline{A\cdot B}=\overline{A}+\overline{B}\qquad\overline{A+B}=\overline{A}\cdot\overline{B}
@math \overline{A\cdot B\cdot C}=\overline{A}+\overline{B}+\overline{C}\qquad\overline{A+B+C}=\overline{A}\cdot\overline{B}\cdot\overline{C}
@card OU exclusif
@tags xor ou exclusif
@math A\oplus B=A\overline{B}+\overline{A}B=(A+B)\cdot\overline{AB}
@math \overline{A\oplus B}=AB+\overline{A}\,\overline{B}\qquad A\oplus1=\overline{A}\qquad A\oplus A=0
@card Opérateurs universels
@tags nand nor universel equivalence
@table[lll] & \text{NON-ET seules} & \text{NON-OU seules}
@row \overline{A} & \overline{A\cdot A} & \overline{A+A}
@row A\cdot B & \overline{\overline{A\cdot B}} & \overline{\overline{A}+\overline{B}}
@row A+B & \overline{\overline{A}\cdot\overline{B}} & \overline{\overline{A+B}}
@note Toute fonction logique se réalise avec des portes NON-ET seules (ou NON-OU seules).

@page Formes canoniques : mintermes et maxtermes
@card Mintermes
@tags minterme somme canonique
@text Un minterme est un produit de toutes les variables (vraies ou complémentées) qui vaut 1 pour une seule combinaison. Son indice est la combinaison lue en binaire (A poids fort).
@math m_5=A\,\overline{B}\,C\quad(101)\qquad F=\sum m(1,3,5,7)
@card Maxtermes
@tags maxterme produit canonique
@text Un maxterme est une somme de toutes les variables qui vaut 0 pour une seule combinaison.
@math M_5=\overline{A}+B+\overline{C}\qquad F=\prod M(0,2,4,6)
@note Les indices des maxtermes de F sont ceux qui manquent dans la liste des mintermes.

@page Tableau de Karnaugh
@card Principe
@tags karnaugh gray adjacence
@text Les cases sont rangées en code de Gray (00, 01, 11, 10) : deux cases voisines ne diffèrent que d'une variable. Le tableau se referme sur lui-même (bords opposés voisins).
@card Regroupements
@tags regroupement simplification
@li Regrouper les 1 par paquets rectangulaires de 1, 2, 4, 8 cases, les plus grands possibles.
@li Chaque paquet donne un produit : les variables qui changent dans le paquet disparaissent.
@li Les cases X (indifférentes) peuvent servir à agrandir les paquets.
@li Regrouper les 0 donne le produit de sommes (forme duale).
@note Le calculateur trouve un recouvrement minimal exact (nombre de termes, puis de littéraux) par la méthode de Quine et McCluskey.

@page Portes logiques et équivalences
@card Portes de base
@tags porte et ou non
@table[ccccccc] A\,B & \text{ET} & \text{OU} & \text{NON-ET} & \text{NON-OU} & \text{OU-X} & \text{NON-OU-X}
@row 00 & 0 & 0 & 1 & 1 & 0 & 1
@row 01 & 0 & 1 & 1 & 0 & 1 & 0
@row 10 & 0 & 1 & 1 & 0 & 1 & 0
@row 11 & 1 & 1 & 0 & 0 & 0 & 1
@card Symboles
@tags symbole norme cei ansi
@text Norme européenne (CEI 60617) : rectangle avec « & » (ET), « ≥1 » (OU), « =1 » (OU exclusif), « 1 » (NON) ; un rond sur la sortie indique l'inversion. Les symboles américains (ANSI) ont des formes distinctes.
@card OU exclusif en NON-ET
@tags xor nand
@math A\oplus B=\overline{\overline{A\cdot\overline{AB}}\cdot\overline{B\cdot\overline{AB}}}
@note Quatre portes NON-ET : c'est la réalisation classique du demi-additionneur.

@page Bases et codes binaires
@card Numération
@tags binaire hexadecimal octal base
@math N=\sum_{k=0}^{n-1}b_k\,2^k\qquad 0\le N\le2^n-1
@text Hexadécimal : groupes de 4 bits (0 à F) ; octal : groupes de 3 bits.
@card Complément à 2
@tags signe complement
@math -2^{n-1}\le N\le2^{n-1}-1\qquad -N=\overline{N}+1
@note Le bit de poids fort est le bit de signe (1 : négatif).
@card Code de Gray et BCD
@tags gray bcd
@math g=b\oplus(b\gg1)
@text Gray : un seul bit change entre deux valeurs consécutives (codeurs, Karnaugh). BCD : chaque chiffre décimal est codé sur 4 bits.
)TEX";
