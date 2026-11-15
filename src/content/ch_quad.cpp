// Fiches : quadripoles.
extern const char CH_QUAD[];
const char CH_QUAD[] = R"TEX(
@page Conversion des matrices Z, Y, H, G, T, S
@card Définitions
@tags definition convention impedance admittance hybride transfert
@text Convention récepteur aux deux accès : $I_1$ et $I_2$ entrent dans le quadripôle.
@math \begin{pmatrix}V_1\\V_2\end{pmatrix}=\begin{pmatrix}Z_{11}&Z_{12}\\Z_{21}&Z_{22}\end{pmatrix}\begin{pmatrix}I_1\\I_2\end{pmatrix}
@math \begin{pmatrix}I_1\\I_2\end{pmatrix}=[Y]\begin{pmatrix}V_1\\V_2\end{pmatrix}\qquad\begin{pmatrix}V_1\\I_2\end{pmatrix}=[H]\begin{pmatrix}I_1\\V_2\end{pmatrix}
@math \begin{pmatrix}I_1\\V_2\end{pmatrix}=[G]\begin{pmatrix}V_1\\I_2\end{pmatrix}\qquad\begin{pmatrix}V_1\\I_1\end{pmatrix}=\begin{pmatrix}A&B\\C&D\end{pmatrix}\begin{pmatrix}V_2\\-I_2\end{pmatrix}
@card Passages entre matrices
@tags conversion inverse
@math [Y]=[Z]^{-1}\qquad[G]=[H]^{-1}\qquad\Delta_Z=Z_{11}Z_{22}-Z_{12}Z_{21}
@math h_{11}=\frac{\Delta_Z}{Z_{22}}\qquad h_{12}=\frac{Z_{12}}{Z_{22}}
@math h_{21}=-\frac{Z_{21}}{Z_{22}}\qquad h_{22}=\frac{1}{Z_{22}}
@math A=\frac{Z_{11}}{Z_{21}}\quad B=\frac{\Delta_Z}{Z_{21}}\quad C=\frac{1}{Z_{21}}\quad D=\frac{Z_{22}}{Z_{21}}
@card Paramètres S
@tags repartition onde s scattering
@math a_k=\frac{V_k+Z_0I_k}{2\sqrt{Z_0}}\qquad b_k=\frac{V_k-Z_0I_k}{2\sqrt{Z_0}}
@math [S]=\left([Z]-Z_0\,\mathrm{Id}\right)\left([Z]+Z_0\,\mathrm{Id}\right)^{-1}
@note $|S_{11}|$ : coefficient de réflexion à l'entrée (sortie adaptée) ; $|S_{21}|^2$ : gain en puissance.
@card Propriétés
@tags reciproque symetrique passif
@li Réciproque (passif) : $Z_{12}=Z_{21}$, $Y_{12}=Y_{21}$, $h_{12}=-h_{21}$, $AD-BC=1$.
@li Symétrique : réciproque et $Z_{11}=Z_{22}$ (ou $A=D$).
@card Quadripôle chargé
@tags charge entree sortie gain
@math Z_e=\frac{AZ_L+B}{CZ_L+D}\qquad Z_s=\frac{DZ_g+B}{CZ_g+A}
@math A_v=\frac{V_2}{V_1}=\frac{Z_L}{AZ_L+B}\qquad A_i=\frac{-I_2}{I_1}=\frac{1}{CZ_L+D}
@card Associations
@tags cascade serie parallele
@li Cascade : $[T]=[T_1][T_2]$.
@li Série-série : $[Z]=[Z_1]+[Z_2]$.
@li Parallèle-parallèle : $[Y]=[Y_1]+[Y_2]$.
@li Série-parallèle : $[H]=[H_1]+[H_2]$.

@page Quadripôles en T, en Π et en L
@card Cellule en T
@tags t etoile
@math [Z]=\begin{pmatrix}Z_1+Z_2&Z_2\\Z_2&Z_2+Z_3\end{pmatrix}
@card Cellule en Π
@tags pi triangle
@math [Y]=\begin{pmatrix}Y_1+Y_2&-Y_2\\-Y_2&Y_2+Y_3\end{pmatrix}\qquad Y_k=\frac{1}{Z_k}
@card Matrices de transfert élémentaires
@tags transfert cascade
@math \text{série : }\begin{pmatrix}1&Z\\0&1\end{pmatrix}\qquad\text{parallèle : }\begin{pmatrix}1&0\\Y&1\end{pmatrix}
@text Une cellule en L, T ou Π est le produit de ces matrices, de l'entrée vers la sortie.
@card Impédances images
@tags image iterative caracteristique
@math Z_{i1}=\sqrt{\frac{AB}{CD}}\qquad Z_{i2}=\sqrt{\frac{DB}{CA}}
@note Cellule symétrique : $Z_{i1}=Z_{i2}=\sqrt{B/C}$ (impédance caractéristique).

@page Atténuateurs en T et en Π
@card Atténuateur en T adapté
@tags t attenuation
@math K=10^{A/20}\qquad R_1=R_2=Z_0\frac{K-1}{K+1}\qquad R_3=\frac{2Z_0K}{K^2-1}
@card Atténuateur en Π adapté
@tags pi
@math R_1=R_2=Z_0\frac{K+1}{K-1}\qquad R_3=Z_0\frac{K^2-1}{2K}
@card Impédances différentes
@tags adaptation
@math R_3=\frac{2K\sqrt{Z_1Z_2}}{K^2-1}\qquad R_1=Z_1\frac{K^2+1}{K^2-1}-R_3
@math R_2=Z_2\frac{K^2+1}{K^2-1}-R_3
@math A_{\min}=20\log\left(\sqrt{\frac{Z_1}{Z_2}}+\sqrt{\frac{Z_1}{Z_2}-1}\right)
@note Formule valable pour $Z_1>Z_2$ (sinon échanger $Z_1$ et $Z_2$).
@note Le Π s'obtient depuis le T par la transformation étoile → triangle.
)TEX";
