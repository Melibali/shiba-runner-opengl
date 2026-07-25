/*!\file animations.h
 * \brief Déclarations des transitions et animations de la démo.
 * \author Melissa
 */
#ifndef ANIMATIONS_H
#define ANIMATIONS_H

#include <GL4D/gl4dh.h>

void animationsInit(void);

/* Animations */
void rouge(int state);

/* Transitions */
void fondu(void (*a0)(int), void (*a1)(int), Uint32 t, Uint32 et, int state);
void fondui(void (*a0)(int), void (*a1)(int), Uint32 t, Uint32 et, int state);

/* Scènes externes */
void skydome2(int state);
void shiba(int state);
void shibaKeyDown(int keycode);
void shibaKeyUp(int keycode);

#endif