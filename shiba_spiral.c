/*!\file shiba_spiral.c
 * \brief Spirale dorée/noire avec apparition du Shiba.
 * Fin : Shiba fonce vers la caméra puis fondu au noir.
 * \author Melissa
 */
#include <GL4D/gl4duw_SDL2.h>
#include <GL4D/gl4dh.h>
#include <GL4D/gl4dg.h>
#include <GL4D/gl4du.h>
#include <math.h>
#include "assimp.h"
static GLuint _pId        = 0;
static GLuint _pIdShiba   = 0;
static GLuint _pIdOverlay = 0;   /* programme pour le fondu noir     */
static GLuint _quad       = 0;
static GLuint _id_scene   = 0;
static float  _time       = 0.0f;
static float  _spiral_t   = 0.0f;   /* temps spirale séparé pour ralentir */
static int    _matrices_created = 0;

static void init(void);
static void draw(void);
static void quit(void);

void shiba_spiral(int state) {
  switch(state) {
  case GL4DH_INIT:              init(); return;
  case GL4DH_FREE:              quit(); return;
  case GL4DH_UPDATE_WITH_AUDIO: return;
  default:                      draw(); return;
  }
}

static void init(void) {
  _time     = 0.0f;
  _spiral_t = 0.0f;
  glClearColor(0, 0, 0, 1);
  _quad     = gl4dgGenQuadf();
  _id_scene = assimpGenScene("models/shiba/scene.gltf");//chargement du model 3d
  _pId = gl4duCreateProgram("<vs>shaders/spiral.vs","<fs>shaders/spiral.fs",NULL);
  _pIdShiba = gl4duCreateProgram("<vs>shaders/shiba.vs","<fs>shaders/shiba.fs",NULL);

  /* programme simple pour dessiner un quad noir semi-transparent     */
  _pIdOverlay = gl4duCreateProgram(
    "<vs>shaders/basic.vs",
    "<fs>shaders/basic.fs",
    NULL);

  if(!_matrices_created) {
    gl4duGenMatrix(GL_FLOAT, "projectionMatrix");//les objet lointaine devient plus petits 
    gl4duGenMatrix(GL_FLOAT, "modelViewMatrix");//obj + cam
    gl4duGenMatrix(GL_FLOAT, "model");//obj rotation +taille+translation
    gl4duGenMatrix(GL_FLOAT, "view");//cam 
    gl4duGenMatrix(GL_FLOAT, "proj");
    _matrices_created = 1;
  }


  GLint vp[4];
  glGetIntegerv(GL_VIEWPORT, vp);
  gl4duBindMatrix("projectionMatrix");
  gl4duLoadIdentityf();
  gl4duFrustumf(-0.005f,  0.005f,
                -0.005f * vp[3]/vp[2],
                 0.005f * vp[3]/vp[2],
                 0.01f, 1000.0f);
}

static void draw(void) {
  _time += 0.02f;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  /*SPIRALE — ralentit et disparaît à la fin*/
  glDisable(GL_DEPTH_TEST);
  glUseProgram(_pId);

  /* la spirale ralentit progressivement après 6s */
  float spiral_speed = 1.0f;
  if(_time > 6.0f) {
    float t2 = (_time - 6.0f) / 3.0f;//interpolation de ralentissement
    if(t2 > 1.0f) t2 = 1.0f;
    spiral_speed = 1.0f - t2;   /* ralentit jusqu'à 0 */
  }
  _spiral_t += 0.02f * spiral_speed;
  glUniform1f(glGetUniformLocation(_pId, "time"), _spiral_t);

  gl4duBindMatrix("model"); gl4duLoadIdentityf();
  gl4duBindMatrix("view");  gl4duLoadIdentityf();
  gl4duBindMatrix("proj");  gl4duLoadIdentityf();
  gl4duSendMatrices();

  gl4dgDraw(_quad);
  glUseProgram(0);

  /* SHIBA — arrive du centre, puis fonce vers la caméra */
  glEnable(GL_DEPTH_TEST);
  glClear(GL_DEPTH_BUFFER_BIT);
  glUseProgram(_pIdShiba);

  GLfloat amb[4]  = {0.8f, 0.8f, 0.8f, 1};
  GLfloat diff[4] = {1.0f, 1.0f, 1.0f, 1};
  GLfloat spec[4] = {0.2f, 0.2f, 0.2f, 1};
  glUniform4fv(glGetUniformLocation(_pIdShiba, "ambient_color"),  1, amb);
  glUniform4fv(glGetUniformLocation(_pIdShiba, "diffuse_color"),  1, diff);
  glUniform4fv(glGetUniformLocation(_pIdShiba, "specular_color"), 1, spec);
  glUniform1f( glGetUniformLocation(_pIdShiba, "shininess"), 32.0f);
  glUniform1i( glGetUniformLocation(_pIdShiba, "hasTexture"), 0);

  gl4duBindMatrix("modelViewMatrix");
  gl4duLoadIdentityf();
  gl4duLookAtf(0.0f, 1.0f, 4.0f,0.0f, 0.0f, 0.0f,0.0f, 1.0f, 0.0f);

  /* Phase 1 : 0 -> 6s Shiba arrive du fond (ease-out) */
  float progress = _time / 6.0f;
  if(progress > 1.0f) progress = 1.0f;
  float ease = 1.0f - (1.0f - progress) * (1.0f - progress);
  float z   = -30.0f + ease * 28.0f; 
  float s   = 0.05f  + ease * 1.1f;
  float bob = fabsf(sinf(_time * 4.0f)) * 0.05f * ease;
  float rot = (1.0f - ease) * _time * 80.0f;

  /* Phase 2 : 6s -> 9s Shiba fonce vers la caméra en accélérant */
  if(_time > 6.0f) {
    float rush = (_time - 6.0f) / 3.0f;   
    if(rush > 1.0f) rush = 1.0f;
    /* accélération quadratique = effet de vitesse croissante */
    float accel = rush * rush;
    z += accel * 18.0f;   
    s += accel * 2.5f;    
    bob = 0.0f;           
    rot = 0.0f;
  }
  
  gl4duTranslatef(0.0f, -0.4f + bob, z);
  gl4duRotatef(rot, 0.0f, 1.0f, 0.0f);
  gl4duScalef(s, s, s);
  gl4duSendMatrices();
  assimpDrawScene(_id_scene);
  glUseProgram(0);

  if(_time > 7.0f) {
    float fade = (_time - 7.0f) / 2.0f;  /* 0 -> 1 en 2s   */
    if(fade > 1.0f) fade = 1.0f;
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(_pIdOverlay);
    /* quad plein écran noir avec alpha croissant */
    glUniform1i(glGetUniformLocation(_pIdOverlay, "inv"),       0);
    glUniform1i(glGetUniformLocation(_pIdOverlay, "myTexture"), 0);
    glUniform1f(glGetUniformLocation(_pIdOverlay, "zoom"),      1.0f);
    glUniform4f(glGetUniformLocation(_pIdOverlay, "colorTint"),
                0.0f, 0.0f, 0.0f, fade);
    gl4dgDraw(_quad);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glUseProgram(0);
  }
}
static void quit(void) {
  _spiral_t = 0.0f;
    _time     = 0.0f;
}

//gère les sommets (positions, transformations, projection)
//gère les pixels, couleurs, textures, lumière