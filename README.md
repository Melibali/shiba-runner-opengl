SHIBA RUNNER

Projet réalisé pour le cours de Synthèse d’Images, Animation et Sons.

Auteur

Melissa

Description

SHIBA RUNNER est une démo interactive 3D réalisée en C/OpenGL avec GL4Dummies.

Le projet met en scène plusieurs séquences animées synchronisées sur une timeline, combinant rendu temps réel, animation, audio et interactions.

La démo enchaîne plusieurs scènes :

écran d’introduction
transitions visuelles
spirale animée
environnement 3D dynamique
skydome avec cycle jour/nuit
mini-jeu runner interactif

Le joueur contrôle un Shiba Inu avançant automatiquement dans un couloir bordé d’arbres et doit éviter les obstacles apparaissant sur le chemin.

Le projet utilise :

shaders GLSL
textures 2D
modèle 3D importé avec Assimp
animations temps réel
caméra dynamique
effets de particules simples
audio synchronisé
effets visuels réactifs au son
éclairage et effets atmosphériques
Compilation

Sous Linux :

make
Exécution
./demo
Contrôles
Flèche Gauche / Q : déplacement à gauche
Flèche Droite / D : déplacement à droite
R : recommencer après le Game Over
Échap : quitter
Technologies utilisées
C
OpenGL
SDL2
SDL_image
SDL_ttf
GL4Dummies
Assimp


Organisation du projet
logo.c : écran d’introduction et crédits
shiba_spiral.c : spirale animée et apparition du Shiba
skydome2.c : environnement dynamique avec ciel, soleil, étoiles et effets visuels
shiba.c : mini-jeu runner interactif
animations.c : transitions et fondus
assimp.c : chargement des modèles 3D
window.c : gestion de la fenêtre et timeline principale
Crédits
Code

Melissa

Modèle 3D

Musique

Fichier audio MOD utilisé dans la démo.
Source : modarchive.org

Textures

Textures utilisées dans le dossier images/.

Licence

Projet réalisé dans un cadre universitaire.