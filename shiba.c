/*!\file shiba.c
 *
 * \brief Shiba marche en avant, des cubes arrivent en face.
 * Utilisez les touches GAUCHE / DROITE (ou Q / D) pour esquiver.
  * \author Melissa
 */
#include <GL4D/gl4duw_SDL2.h>
#include <GL4D/gl4dh.h>
#include <GL4D/gl4dg.h>
#include <SDL_image.h>
#include "SDL_opengl.h"
#include "assimp.h"
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "audioHelper.h"
#include <GL4D/gl4df.h>
/* Constantes du jeu                                                   */
#define MAX_CUBES       20
#define CUBE_SPAWN_Z   -60.0f
#define CUBE_DESPAWN_Z   5.0f
#define CUBE_SPEED       5.0f
#define SPAWN_INTERVAL   1.2f
#define SHIBA_SPEED      5.0f
#define SHIBA_WALK_SPEED 6.0f   /* vitesse d'avance de Shiba dans le couloir */
#define SHIBA_X_MIN     -4.0f
#define SHIBA_X_MAX      4.0f
#define COLLISION_DIST   1.0f
#define WALL_SPACING     5.0f
#define WALL_COUNT       20
#define WALL_X_LEFT     -5.5f
#define WALL_X_RIGHT     5.5f
#define SKY_RADIUS      500.0f

/* Structures                                                          */
typedef struct {
  GLfloat x, z;
  int     active;
} Cube;

typedef struct cam_t cam_t;
struct cam_t {
  GLfloat x, y, z;
};
#define MAX_LEAVES 60

typedef struct {
  float x,y,z;
  float vx,vy,vz;
  float angle,av;
  float scale;
  int active;
} Leaf;
/* Variables statiques                                                 */
static GLuint  _pId       = 0;
static GLuint  _pId_cube  = 0;
static GLuint  _id_scene  = 0;
static GLuint  _cube_geo  = 0;
static GLuint  _sky_geo   = 0;
static GLuint  _plan_geo  = 0;

static cam_t   _cam       = {0.0f, 1.5f, 4.0f};

static GLfloat _shiba_x   = 0.0f;
static GLfloat _shiba_z   = 0.0f;

static Cube    _cubes[MAX_CUBES];
static float   _spawn_timer = 0.0f;
static float   _cam_roll    = 0.0f;  /* inclinaison caméra en degrés */
static float _cube_speed = CUBE_SPEED;
static float _spawn_interval = SPAWN_INTERVAL;

static int     _key_left  = 0;
static int     _key_right = 0;
static int     _game_over = 0;

static GLuint  _cube_tex  = 0;
static GLuint  _wall_tex  = 0;
static GLuint  _sky_tex   = 0;
static GLuint  _sol_tex   = 0;
static GLuint _trunk_tex = 0;
//leaves//
static Leaf _leaves[MAX_LEAVES];
static float _leaf_timer = 0.0f;
static float _game_time = 0.0f;
static float _audio_level = 0.0f;
/* Prototypes                                                          */
static void init(void);
static void draw(void);
static void draw_gameover(void);
static void draw_sky(float cam_x, float cam_y, float cam_z,
                     float up_x, float up_y);
static void update(double dt);
static void update_leaves(double dt);  
static void spawn_cube(void);
static void reset_game(void);
static void load_tex(GLuint id, const char *path);

/* Interface GL4DH                                                     */
void shiba(int state) {
  switch(state) {
  case GL4DH_INIT:              init(); return;
  case GL4DH_FREE:              return;
  case GL4DH_UPDATE_WITH_AUDIO: {
    Uint8 *as = ahGetAudioStream();
    int len   = ahGetAudioStreamLength();
    if(as && len > 0) {
      long sum = 0;
      for(int i = 0; i < len; i++) sum += as[i];
      _audio_level = (float)(sum / len) / 128.0f;
      if(_audio_level > 1.0f) _audio_level = 1.0f;
    }
    return;
  }
  default:                      draw(); return;
  }
}

/* Clavier                                                             */
void shibaKeyDown(int keycode) {
  switch(keycode) {
  case SDLK_LEFT:  case 'q': case 'Q': _key_left  = 1; break;
  case SDLK_RIGHT: case 'd': case 'D': _key_right = 1; break;
  case 'r': case 'R': if(_game_over) reset_game(); break;
  }
}

void shibaKeyUp(int keycode) {
  switch(keycode) {
  case SDLK_LEFT:  case 'q': case 'Q': _key_left  = 0; break;
  case SDLK_RIGHT: case 'd': case 'D': _key_right = 0; break;
  }
}

/* Chargement texture                                                  */
static void load_tex(GLuint id, const char *path) {
  glBindTexture(GL_TEXTURE_2D, id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  SDL_Surface *s = IMG_Load(path);
  if(!s) {
    fprintf(stderr, "Erreur texture: %s -> %s\n", path, IMG_GetError());
    glBindTexture(GL_TEXTURE_2D, 0);
    return;
  }
  SDL_Surface *d = SDL_CreateRGBSurface(0, s->w, s->h, 32,
                     R_MASK, G_MASK, B_MASK, A_MASK);
  SDL_BlitSurface(s, NULL, d, NULL);
  SDL_FreeSurface(s);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, d->w, d->h, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, d->pixels);
  SDL_FreeSurface(d);
  glBindTexture(GL_TEXTURE_2D, 0);
}

/* Init                                                                */
static void init(void) {
  srand((unsigned)time(NULL));
  _id_scene = assimpGenScene("models/shiba/scene.gltf");

  _pId      = gl4duCreateProgram("<vs>shaders/shiba.vs",
                                  "<fs>shaders/shiba.fs", NULL);
  _pId_cube = gl4duCreateProgram("<vs>shaders/skydome.vs",
                                  "<fs>shaders/skydome.fs", NULL);
  _cube_geo = gl4dgGenCubef();
  _plan_geo = gl4dgGenQuadf();
  _sky_geo  = gl4dgGenSpheref(32, 32);
  glGenTextures(1, &_trunk_tex);
  load_tex(_trunk_tex, "images/wood_t.jpeg");

  static int _matrices_created = 0;
  if(!_matrices_created) {
    gl4duGenMatrix(GL_FLOAT, "modelViewMatrix");
    gl4duGenMatrix(GL_FLOAT, "projectionMatrix");
    gl4duGenMatrix(GL_FLOAT, "model");
    gl4duGenMatrix(GL_FLOAT, "view");
    gl4duGenMatrix(GL_FLOAT, "proj");
    _matrices_created = 1;
  }

  GLint vp[4];
  glGetIntegerv(GL_VIEWPORT, vp);

  gl4duBindMatrix("projectionMatrix");
  gl4duLoadIdentityf();
  gl4duFrustumf(-0.005f, 0.005f,-0.005f * vp[3] / vp[2], 0.005f * vp[3] / vp[2],0.01f, 1000.0f);
  glGenTextures(1, &_cube_tex);
  load_tex(_cube_tex, "images/cube.jpg");
  glGenTextures(1, &_wall_tex);
  load_tex(_wall_tex, "images/leaves.png");
  glGenTextures(1, &_sky_tex);
  load_tex(_sky_tex, "images/sky.jpg");
  glGenTextures(1, &_sol_tex);
  load_tex(_sol_tex, "images/wood.jpg");
  glBindTexture(GL_TEXTURE_2D, _sol_tex);
  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glBindTexture(GL_TEXTURE_2D, 0);

  reset_game();
}

/* Reset                                                               */
static void reset_game(void) {
  _shiba_x     = 0.0f;
  _shiba_z     = 0.0f;
  _game_over   = 0;
  _spawn_timer = 0.0f;
  _cam_roll    = 0.0f;
  _cube_speed = CUBE_SPEED;
  _spawn_interval = SPAWN_INTERVAL;
  for(int i = 0; i < MAX_CUBES; i++)
    _cubes[i].active = 0;

  _game_time = 0.0f;
  _leaf_timer = 999.0f;

  for(int i=0;i<MAX_LEAVES;i++)
      _leaves[i].active = 0;

}

/* Spawn cube                                                          */
static void spawn_cube(void) {
  for(int i = 0; i < MAX_CUBES; i++) {
    if(!_cubes[i].active) {
      float r = (float)rand() / RAND_MAX;
      _cubes[i].x = SHIBA_X_MIN + r * (SHIBA_X_MAX - SHIBA_X_MIN);
      _cubes[i].z = _shiba_z + CUBE_SPAWN_Z;
      _cubes[i].active = 1;
      return;
    }
  }
}

/* Update                                                              */
static void update(double dt) {
  if(_game_over) return;
  _game_time += dt;

  _cube_speed = CUBE_SPEED + _game_time * 0.12f;
  _spawn_interval = SPAWN_INTERVAL - _game_time * 0.02f;

  if(_spawn_interval < 0.45f)
      _spawn_interval = 0.45f;
    update_leaves(dt);
    /* Shiba avance en continu dans le couloir (sens -Z) */
  _shiba_z -= SHIBA_WALK_SPEED * dt;

  if(_key_left)  { _shiba_x -= SHIBA_SPEED * dt; if(_shiba_x < SHIBA_X_MIN) _shiba_x = SHIBA_X_MIN; }
  if(_key_right) { _shiba_x += SHIBA_SPEED * dt; if(_shiba_x > SHIBA_X_MAX) _shiba_x = SHIBA_X_MAX; }


  _spawn_timer += dt;
  if(_spawn_timer >= _spawn_interval) {
    _spawn_timer = 0.0f;
    spawn_cube();
  }

  for(int i = 0; i < MAX_CUBES; i++) {
    if(!_cubes[i].active) continue;
    _cubes[i].z += _cube_speed * dt;

    if(_cubes[i].z > _shiba_z + CUBE_DESPAWN_Z) {
      _cubes[i].active = 0;
    }

    float dx = _cubes[i].x - _shiba_x;
    float dz = _cubes[i].z - _shiba_z;
    if(fabsf(dx) < COLLISION_DIST && fabsf(dz) < COLLISION_DIST)
      _game_over = 1;
  }
}

/* Game Over */
static void draw_gameover(void) {
  float t     = (float)gl4dGetElapsedTime() * 0.001f;
  float flash = (sinf(t * 6.0f) + 1.0f) * 0.5f;
  glClearColor(flash * 0.9f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

/* Dessin du ciel — reçoit up_x/up_y pour le roll caméra */
static void draw_sky(float cam_x, float cam_y, float cam_z,
                     float up_x, float up_y) {
  glCullFace(GL_FRONT);
  glDepthMask(GL_FALSE);

  GLint vp[4];
  glGetIntegerv(GL_VIEWPORT, vp);
  gl4duBindMatrix("proj");
  gl4duLoadIdentityf();
  gl4duFrustumf(-0.005f, 0.005f,-0.005f * vp[3] / (float)vp[2],0.005f * vp[3] / (float)vp[2],0.01f, 1000.0f);
  gl4duBindMatrix("view");
  gl4duLoadIdentityf();
  /* La cible de la caméra suit Shiba en Z */
  gl4duLookAtf(cam_x, cam_y, cam_z,_shiba_x, 0.8f, _shiba_z - 5.0f,up_x, up_y, 0.0f);
  gl4duBindMatrix("model");
  gl4duLoadIdentityf();
  gl4duTranslatef(_shiba_x, 0.0f, _shiba_z);
  gl4duScalef(SKY_RADIUS, SKY_RADIUS, SKY_RADIUS);
  gl4duSendMatrices();
  glUniform1i(glGetUniformLocation(_pId_cube, "sky"), 1);
  glUniform1i(glGetUniformLocation(_pId_cube, "fog"), 0);
  glUniform4f(glGetUniformLocation(_pId_cube, "couleur"), 1.0f, 1.0f, 1.0f, 1.0f);
  glUniform2f(glGetUniformLocation(_pId_cube, "tex_mult"),      1.0f, -1.0f);
  glUniform2f(glGetUniformLocation(_pId_cube, "tex_translate"), 0.0f,  0.0f);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _sky_tex);
  glUniform1i(glGetUniformLocation(_pId_cube, "tex"), 0);
  gl4dgDraw(_sky_geo);
  glUniform1i(glGetUniformLocation(_pId_cube, "sky"), 0);
  glDepthMask(GL_TRUE);
  glCullFace(GL_BACK);
}
static void update_leaves(double dt)
{
  _leaf_timer += dt;
  if(_leaf_timer > 0.15f)
  {
    _leaf_timer = 0.0f;
    for(int i=0;i<MAX_LEAVES;i++)
    {
      if(!_leaves[i].active)
      {
        float side =
          (rand()%2==0)?WALL_X_LEFT:WALL_X_RIGHT;

        _leaves[i].x =
          side + ((float)rand()/RAND_MAX-0.5f);
        _leaves[i].y = 3.0f;
        _leaves[i].z =
          _shiba_z - 10.0f - rand()%25;
        _leaves[i].vx =
          ((float)rand()/RAND_MAX-0.5f)*0.4f;
        _leaves[i].vy = -0.8f;
        _leaves[i].vz = 0.3f;
        _leaves[i].angle = 0;
        _leaves[i].av = 90;
        _leaves[i].scale = 0.12f;

        _leaves[i].active = 1;
        break;
      }
    }
  }

  for(int i=0;i<MAX_LEAVES;i++)
  {
    if(!_leaves[i].active) continue;
    _leaves[i].x += _leaves[i].vx * dt;
    _leaves[i].y += _leaves[i].vy * dt;
    _leaves[i].z += _leaves[i].vz * dt;

    _leaves[i].angle += _leaves[i].av * dt;

    if(_leaves[i].y < 0.0f)
      _leaves[i].active = 0;
  }
}
/* Draw principal                                                      */
static void draw(void) {
  static int    first_time = 1;
  static double pt = 0.0;
  double t, dt;
  if(first_time) { first_time = 0; 
  pt = gl4dGetElapsedTime(); }
  t  = gl4dGetElapsedTime();
  dt = (t - pt) / 1000.0;
  pt = t;

  update(dt);

  if(_game_over) {
    draw_gameover();
    return;
  }

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glClearColor(0.1f, 0.40f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  /* La caméra suit Shiba en X et en Z */
  float cam_x = _shiba_x;
  float run = gl4dGetElapsedTime() * 0.01f;
  float cam_y = _cam.y + fabs(sinf(run))*0.05f;
  float cam_z = _shiba_z + _cam.z;
  float target_roll = 0.0f;
  if(_key_left)  target_roll =  5.0f;
  if(_key_right) target_roll = -5.0f;
  _cam_roll += (target_roll - _cam_roll) * 0.12f;

  float roll_rad = _cam_roll * (float)M_PI / 180.0f;
  float up_x = sinf(roll_rad);
  float up_y = cosf(roll_rad);

   /* CIEL*/
  glUseProgram(_pId_cube);
  draw_sky(cam_x, cam_y, cam_z, up_x, up_y);

  /* SHIBA avec éclairage Phong */
  glUseProgram(_pId);

  GLfloat amb[4]  = {0.85f,0.85f,0.85f,1.0f};
  GLfloat diff[4] = {1.0f,1.0f,1.0f,1.0f};
  GLfloat spec[4] = {0.15f,0.15f,0.15f,1.0f};

  glUniform4fv(glGetUniformLocation(_pId, "ambient_color"),  1, amb);
  glUniform4fv(glGetUniformLocation(_pId, "diffuse_color"),  1, diff);
  glUniform4fv(glGetUniformLocation(_pId, "specular_color"), 1, spec);
  glUniform1f (glGetUniformLocation(_pId, "shininess"),      32.0f);
  glUniform1i (glGetUniformLocation(_pId, "hasTexture"),     0);
  gl4duBindMatrix("modelViewMatrix");
  gl4duLoadIdentityf();
  /* La cible de la caméra suit Shiba en Z */
  gl4duLookAtf(cam_x, cam_y, cam_z,_shiba_x, 0.8f, _shiba_z - 5.0f,up_x, up_y, 0.0f);
  float bobY = fabs(sinf(run)) * 0.12f + _audio_level * 0.05f;
  float tiltFront = sinf(run) * 8.0f;
  float tiltSide = 0.0f;
  if(_key_left)  tiltSide = 12.0f;
  if(_key_right) tiltSide = -12.0f;

  gl4duTranslatef(_shiba_x, 0.30f + bobY, _shiba_z);
  gl4duRotatef(180.0f, 0,1,0);
  gl4duRotatef(tiltFront, 1,0,0);
  gl4duRotatef(tiltSide, 0,0,1);
  gl4duSendMatrices();
  assimpDrawScene(_id_scene);

  /* CUBES + ARBRES (shader skydome) */
  glUseProgram(_pId_cube);
  gl4duBindMatrix("view");
  gl4duLoadIdentityf();
  /* La cible de la caméra suit Shiba en Z */
  gl4duLookAtf(cam_x, cam_y, cam_z,_shiba_x, 0.8f, _shiba_z - 5.0f,up_x, up_y, 0.0f);
  glActiveTexture(GL_TEXTURE0);
  glUniform1i(glGetUniformLocation(_pId_cube, "tex"), 0);
  glUniform1i(glGetUniformLocation(_pId_cube, "sky"), 0);
  glUniform1i(glGetUniformLocation(_pId_cube, "fog"), 1);
  glUniform2f(glGetUniformLocation(_pId_cube, "tex_mult"),      1.0f, 1.0f);
  glUniform2f(glGetUniformLocation(_pId_cube, "tex_translate"), 0.0f, 0.0f);

  /*Cubes obstacles*/
  glUniform4f(glGetUniformLocation(_pId_cube, "couleur"), 1.0f, 1.0f, 1.0f, 1.0f);
  glBindTexture(GL_TEXTURE_2D, _cube_tex);
  for(int i = 0; i < MAX_CUBES; i++) {
    if(!_cubes[i].active) continue;
    gl4duBindMatrix("model");
    gl4duLoadIdentityf();
    gl4duTranslatef(_cubes[i].x, 0.3f, _cubes[i].z);
    gl4duScalef(0.5f, 0.5f, 0.5f);
    gl4duSendMatrices();
    gl4dgDraw(_cube_geo);
  }

  /*Arbres du couloir — positions fixes dans le monde*/
int first = (int)((-_shiba_z) / WALL_SPACING) - 2;
if(first < 0) first = 0;
for(int i = first; i < first + WALL_COUNT; i++) {
  float pz = -(float)i * WALL_SPACING;

  /* TRONC GAUCHE */
  glUniform4f(glGetUniformLocation(_pId_cube, "couleur"), 1.0f, 1.0f, 1.0f, 1.0f);
  glBindTexture(GL_TEXTURE_2D, _trunk_tex);
  gl4duBindMatrix("model"); gl4duLoadIdentityf();
  gl4duTranslatef(WALL_X_LEFT, 0.6f, pz);
  gl4duScalef(0.2f, 1.2f, 0.2f);
  gl4duSendMatrices();
  gl4dgDraw(_cube_geo);

  /* FEUILLAGE GAUCHE bas */
  glUniform4f(glGetUniformLocation(_pId_cube, "couleur"), 1.0f, 1.0f, 1.0f, 1.0f);
  glBindTexture(GL_TEXTURE_2D, _wall_tex);
  gl4duBindMatrix("model"); gl4duLoadIdentityf();
  gl4duTranslatef(WALL_X_LEFT, 1.8f, pz);
  gl4duScalef(0.7f, 0.8f, 0.7f);
  gl4duSendMatrices();
  gl4dgDraw(_cube_geo);

  /* FEUILLAGE GAUCHE haut */
  glUniform4f(glGetUniformLocation(_pId_cube, "couleur"), 1.0f, 1.0f, 1.0f, 1.0f);
  glBindTexture(GL_TEXTURE_2D, _wall_tex);
  gl4duBindMatrix("model"); gl4duLoadIdentityf();
  gl4duTranslatef(WALL_X_LEFT, 2.5f, pz);
  gl4duScalef(0.45f, 0.6f, 0.45f);
  gl4duSendMatrices();
  gl4dgDraw(_cube_geo);

  /* TRONC DROIT */
  glUniform4f(glGetUniformLocation(_pId_cube, "couleur"), 1.0f, 1.0f, 1.0f, 1.0f);
  glBindTexture(GL_TEXTURE_2D, _trunk_tex);
  gl4duBindMatrix("model"); gl4duLoadIdentityf();
  gl4duTranslatef(WALL_X_RIGHT, 0.6f, pz);
  gl4duScalef(0.2f, 1.2f, 0.2f);
  gl4duSendMatrices();
  gl4dgDraw(_cube_geo);


  /* FEUILLAGE DROIT bas */
  glUniform4f(glGetUniformLocation(_pId_cube, "couleur"), 1.0f, 1.0f, 1.0f, 1.0f);
  glBindTexture(GL_TEXTURE_2D, _wall_tex);
  gl4duBindMatrix("model"); gl4duLoadIdentityf();
  gl4duTranslatef(WALL_X_RIGHT, 1.8f, pz);
  gl4duScalef(0.7f, 0.8f, 0.7f);
  gl4duSendMatrices();
  gl4dgDraw(_cube_geo);

  /* FEUILLAGE DROIT haut */
  glUniform4f(glGetUniformLocation(_pId_cube, "couleur"), 1.0f, 1.0f, 1.0f, 1.0f);
  glBindTexture(GL_TEXTURE_2D, _wall_tex);
  gl4duBindMatrix("model"); gl4duLoadIdentityf();
  gl4duTranslatef(WALL_X_RIGHT, 2.5f, pz);
  gl4duScalef(0.45f, 0.6f, 0.45f);
  gl4duSendMatrices();
  gl4dgDraw(_cube_geo);
}
  /* SOL */
  {
    const float SOL_S = 120.0f;
    glUniform1i(glGetUniformLocation(_pId_cube, "sky"), 0);
    glUniform1i(glGetUniformLocation(_pId_cube, "fog"), 1);
    glUniform4f(glGetUniformLocation(_pId_cube, "couleur"), 1.0f, 1.0f, 1.0f, 1.0f);
    glUniform2f(glGetUniformLocation(_pId_cube, "tex_mult"),      20.0f, 20.0f);
    glUniform2f(glGetUniformLocation(_pId_cube, "tex_translate"),  0.0f,  0.0f);
    glBindTexture(GL_TEXTURE_2D, _sol_tex);
    gl4duBindMatrix("model");
    gl4duLoadIdentityf();
    /* Le sol suit Shiba en X et en Z pour rester toujours sous lui */
    gl4duTranslatef(_shiba_x, -0.05f, _shiba_z);
    gl4duRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    gl4duScalef(SOL_S, SOL_S, 1.0f);
    gl4duSendMatrices();
    gl4dgDraw(_plan_geo);
    
  }
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  /* FEUILLES QUI TOMBENT */
  glBindTexture(GL_TEXTURE_2D, _wall_tex);
  glUniform1i(glGetUniformLocation(_pId_cube, "sky"), 0);
  glUniform1i(glGetUniformLocation(_pId_cube, "fog"), 0);
  glUniform4f(glGetUniformLocation(_pId_cube, "couleur"),1.0f, 1.0f, 1.0f, 1.0f);
  glUniform2f(glGetUniformLocation(_pId_cube, "tex_mult"), 1.0f, 1.0f);
  glUniform2f(glGetUniformLocation(_pId_cube, "tex_translate"), 0.0f, 0.0f);

  for(int i = 0; i < MAX_LEAVES; i++)
  {
      if(!_leaves[i].active) continue;
      gl4duBindMatrix("model");
      gl4duLoadIdentityf();
      gl4duTranslatef(_leaves[i].x,_leaves[i].y,_leaves[i].z);
      gl4duRotatef(_leaves[i].angle, 0, 1, 0);
      gl4duScalef(_leaves[i].scale,_leaves[i].scale,_leaves[i].scale);
      gl4duSendMatrices();
      gl4dgDraw(_plan_geo);
  }

  glDisable(GL_BLEND);
  glBindTexture(GL_TEXTURE_2D, 0);
  glUseProgram(0);
}
