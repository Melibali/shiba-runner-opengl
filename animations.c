/*!\file animations.c
 * \brief Transitions visuelles (fondu, fondui) et animations simples (rouge).
 * \author Melissa
 */
#include <GL4D/gl4dh.h>
#include "audioHelper.h"
#include <assert.h>
#include <stdlib.h>
#include <GL4D/gl4dg.h>
#include <GL4D/gl4dp.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
static GLuint _quadId = 0;

//charge une image et l’envoie comme texture OpenGL au GPU
/*La fonction charge une image depuis le disque avec SDL_image, 
configure une texture OpenGL puis transfère les pixels vers le GPU avec glTexImage2D afin de pouvoir utiliser l’image dans le rendu 3D.*/
static void _load_texture(GLuint id, const char * filename) {
  SDL_Surface * t;
  glBindTexture(GL_TEXTURE_2D, id);//active la texture OpenGL
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  t = IMG_Load(filename);//charge img depuis chemin
  assert(t);
  SDL_Surface * c = SDL_CreateRGBSurface(0, t->w, t->h, 32, R_MASK, G_MASK, B_MASK, A_MASK);
  SDL_BlitSurface(t, NULL, c, NULL);//image originale ->nouvelle surface compatible
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, c->w, c->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, c->pixels);
  SDL_FreeSurface(c);
  SDL_FreeSurface(t);
}

/* Transitions*/
void fondu(void (* a0)(int), void (* a1)(int), Uint32 t, Uint32 et, int state) {
  int vp[4], i;
  GLint tId;
  static GLuint tex[2], pId;
  switch(state) {
  case GL4DH_INIT:
    glGetIntegerv(GL_VIEWPORT, vp);
    glGenTextures(2, tex);
    for(i = 0; i < 2; i++) {
      glBindTexture(GL_TEXTURE_2D, tex[i]);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, vp[2], vp[3], 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    }
    pId = gl4duCreateProgram("<vs>shaders/basic.vs", "<fs>shaders/mix.fs", NULL);
    return;
  case GL4DH_FREE:
    if(tex[0]) { glDeleteTextures(2, tex); tex[0] = tex[1] = 0; }
    return;
  case GL4DH_UPDATE_WITH_AUDIO://mis a jour les deux scène avec audio
    if(a0) a0(state);
    if(a1) a1(state);
    return;
  default:
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &tId);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex[0], 0);
    if(a0) a0(state);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex[1], 0);

    if(a1) a1(state);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tId, 0);
    glDisable(GL_DEPTH_TEST);
    glUseProgram(pId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tex[1]);

    if(et / (GLfloat)t > 1) { fprintf(stderr, "%d-%d -- %f\n", et, t, et / (GLfloat)t); exit(0); }
    glUniform1f(glGetUniformLocation(pId, "dt"), et / (GLfloat)t);
    glUniform1i(glGetUniformLocation(pId, "tex0"), 0);
    glUniform1i(glGetUniformLocation(pId, "tex1"), 1);
    glUniform1f(glGetUniformLocation(pId, "zoom"), 1.0f);
    glUniform4f(glGetUniformLocation(pId, "colorTint"), 1.0f, 1.0f, 1.0f, 1.0f);
    gl4dgDraw(_quadId);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, 0);
    return;
  }
}

void fondui(void (* a0)(int), void (* a1)(int), Uint32 t, Uint32 et, int state) {
  int vp[4], i;
  GLint tId;
  static GLuint tex[3], pId;
  switch(state) {
  case GL4DH_INIT:
    glGetIntegerv(GL_VIEWPORT, vp);
    glGenTextures(3, tex);
    for(i = 0; i < 3; i++) {
      glBindTexture(GL_TEXTURE_2D, tex[i]);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, vp[2], vp[3], 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    }
    _load_texture(tex[2], "images/fondui.jpg");
    pId = gl4duCreateProgram("<vs>shaders/basic.vs", "<fs>shaders/mixi.fs", NULL);
    return;
  case GL4DH_FREE:
    if(tex[0]) { glDeleteTextures(3, tex); tex[0] = tex[1] = tex[2] = 0; }    return;
  case GL4DH_UPDATE_WITH_AUDIO:
    if(a0) a0(state);
    if(a1) a1(state);
    return;
  default:
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &tId);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex[0], 0);
    if(a0) a0(state);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex[1], 0);
    if(a1) a1(state);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tId, 0);
    glDisable(GL_DEPTH_TEST);
    glUseProgram(pId);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, tex[0]);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, tex[1]);
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, tex[2]);
    if(et / (GLfloat)t > 1) { fprintf(stderr, "%d-%d -- %f\n", et, t, et / (GLfloat)t); exit(0); }
    glUniform1f(glGetUniformLocation(pId, "dt"), et / (GLfloat)t);
    glUniform1i(glGetUniformLocation(pId, "tex0"), 0);
    glUniform1i(glGetUniformLocation(pId, "tex1"), 1);
    glUniform1i(glGetUniformLocation(pId, "tex2"), 2);
    glUniform1f(glGetUniformLocation(pId, "zoom"), 1.0f);
    glUniform4f(glGetUniformLocation(pId, "colorTint"), 1.0f, 1.0f, 1.0f, 1.0f);
    gl4dgDraw(_quadId);
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, 0);
    return;
  }
}

/* Animations couleur simples*/
void rouge(int state) {
  switch(state) {
  case GL4DH_INIT: return;
  case GL4DH_FREE: return;
  case GL4DH_UPDATE_WITH_AUDIO: return;
  default:
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    return;
  }
}

/* Init global*/
void animationsInit(void) {
  if(!_quadId)
    _quadId = gl4dgGenQuadf();
}
