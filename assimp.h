/*!\file assimp.h
 * \brief Interface du module de chargement de scènes via libassimp.
 */
#ifndef ASSIMP_H
#define ASSIMP_H

#include <GL4D/gl4duw_SDL2.h>

/* Charge une scène depuis un fichier et retourne son identifiant */
GLuint assimpGenScene(const char * filename);

/* Dessine la scène identifiée par id_scene */
void   assimpDrawScene(GLuint id_scene);

/* Libère la scène identifiée par id_scene */
void   assimpDeleteScene(GLuint id_scene);

#endif /* ASSIMP_H */
