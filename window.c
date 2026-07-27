/*!\file window.c
 * \brief Fenêtre principale et gestion de la timeline de la démo.
 * \author Melissa
 */
#include <stdlib.h>
#include <GL4D/gl4du.h>
#include <GL4D/gl4dh.h>
#include <GL4D/gl4duw_SDL2.h>
#include "animations.h"
#include "audioHelper.h"
static void init(void);
static void quit(void);
static void resize(int w, int h);
static void keydown(int keycode);
static void keyup(int keycode);
void logo(int state);
void shiba_spiral(int state);

static GL4DHanime _animations[] = {
 { 7000, logo, NULL, NULL },
 { 2000, logo, shiba_spiral, fondu },
 { 6500, shiba_spiral, NULL, NULL },
 { 800, shiba_spiral, skydome2, fondu },
 { 30000, skydome2, NULL, NULL },
 { 2000, skydome2, rouge, fondu },
 { 1000, rouge, NULL, NULL },
 { 2000, rouge, shiba, fondu },
 { 50000, shiba, NULL, NULL },
 { 0, NULL, NULL, NULL }
};

static GLfloat _dim[] = {1024, 1024};

int main(int argc, char ** argv) {
  if(!gl4duwCreateWindow(argc, argv, "Ateliers API8 - demo",
       GL4DW_POS_UNDEFINED, GL4DW_POS_UNDEFINED,
       _dim[0], _dim[1], GL4DW_SHOWN))
    return 1;
  init();
  atexit(quit);
  gl4duwResizeFunc(resize);
  gl4duwKeyDownFunc(keydown);
  gl4duwKeyUpFunc(keyup);
  gl4duwDisplayFunc(gl4dhDraw);
  ahInitAudio("takeonme.mod");
  gl4duwMainLoop();
  return 0;
}

static void init(void) {
  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  gl4dhInit(_animations, _dim[0], _dim[1], animationsInit);
  resize(_dim[0], _dim[1]);
}

static void resize(int w, int h) {
  _dim[0] = w; _dim[1] = h;
  glViewport(0, 0, w, h);
}

static void keydown(int keycode) {
  switch(keycode) {
  case SDLK_ESCAPE:
    exit(0);
    break;
  default:
    shibaKeyDown(keycode);
    break;
  }
}
static void keyup(int keycode) {
  shibaKeyUp(keycode);
}

static void quit(void) {
  ahClean();
  gl4duClean(GL4DU_ALL);
}
