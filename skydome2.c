/*!\file skydome2.c */
#include <GL4D/gl4duw_SDL2.h>
#include <GL4D/gl4dm.h>
#include <GL4D/gl4dg.h>
#include <GL4D/gl4dh.h>
#include <math.h>
#include <SDL_image.h>
#include <GL4D/gl4du.h>
#include <SDL_opengl.h>
#include <SDL_surface.h>
#include <stdlib.h>
#include <time.h>
#include "audioHelper.h"

#define WALL_SPACING   5.0f
#define WALL_COUNT     40
#define WALL_X_LEFT   -5.5f
#define WALL_X_RIGHT   5.5f
#define MAX_FALL_CUBES 15
#define FALL_GRAVITY   9.8f
#define MAX_LEAVES     80
#define MAX_STARS      200

typedef struct { float x,y,z,vy,scale; int bounced,active; } FallCube;
typedef struct { float x,y,z,vx,vy,vz,angle,av,scale; int active; } Leaf;
typedef struct { float x,y,z,size; } Star;
typedef struct { GLfloat x,y,z,theta; } cam_t;

static void init(void);
static void draw(void);
+
static void quit(void);
static void update_fall_cubes(double dt);
static void update_leaves(double dt);

/* Variables statiques                                                 */
static GLuint _pId=0, _plan=0, _sky=0, _cube_geo=0;
static GLuint _quad_geo=0, _sphere_geo=0;
static GLuint _texId[3]={0}, _wall_tex=0, _wood_tex=0;
static GLuint _cube_tex=0;
static GLuint _sun_tex=0;
static cam_t  _cam = {0,1,7,0};
static const GLfloat _plan_s = 100.0f;
static int _fog = 1;

static float _demoTime=0;
static int   _scene=1;

static FallCube _fall_cubes[MAX_FALL_CUBES];
static float    _spawn_fall_timer=0;
static Leaf  _leaves[MAX_LEAVES];
static float _spawn_leaf_timer=0;

static Star _stars[MAX_STARS];
static int  _stars_init = 0;

static int _draw_needs_reset=1, _matrices_created=0;
static float _audio_level = 0.0f;
void skydome2(int state) {
  switch(state) {
  case GL4DH_INIT:              init(); return;
  case GL4DH_FREE:              quit(); return;
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

static void init(void) {
  srand((unsigned)time(NULL));
  memset(_fall_cubes,0,sizeof _fall_cubes);
  memset(_leaves,    0,sizeof _leaves);
  _cam.x=0; _cam.y=1; _cam.z=7; _cam.theta=0;
  _spawn_fall_timer=0; _spawn_leaf_timer=0;
  _demoTime=0; _scene=1;
  _draw_needs_reset=1;

  glClearColor(0,0,0.05f,1);

  _plan      = gl4dgGenQuadf();
  _sky       = gl4dgGenSpheref(20,20);
  _cube_geo  = gl4dgGenCubef();
  _quad_geo  = gl4dgGenQuadf();
  _sphere_geo= gl4dgGenSpheref(16,16);

  _pId = gl4duCreateProgram("<vs>shaders/skydome.vs",
                             "<fs>shaders/skydome.fs",NULL);

  if(!_matrices_created) {
    gl4duGenMatrix(GL_FLOAT,"model");
    gl4duGenMatrix(GL_FLOAT,"view");
    gl4duGenMatrix(GL_FLOAT,"proj");
    _matrices_created=1;
  }

  GLint vp[4]; glGetIntegerv(GL_VIEWPORT,vp);
  gl4duBindMatrix("proj"); gl4duLoadIdentityf();
  gl4duFrustumf(-1,1,-vp[3]/(GLfloat)vp[2],vp[3]/(GLfloat)vp[2],2.0,_plan_s);

  /* Texture 0 : blanc 1x1 */
  if(!_texId[0]) glGenTextures(3,_texId);
  glBindTexture(GL_TEXTURE_2D,_texId[0]);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  { GLuint w=0xFFFFFFFF;
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,&w); }

  /* Texture 1 : sol */
  glBindTexture(GL_TEXTURE_2D,_texId[1]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  SDL_Surface *s = IMG_Load("images/wood.jpg"); assert(s);
  SDL_Surface *d = SDL_CreateRGBSurface(0,s->w,s->h,32,R_MASK,G_MASK,B_MASK,A_MASK);
  SDL_BlitSurface(s,NULL,d,NULL); SDL_FreeSurface(s);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,d->w,d->h,0,GL_RGBA,GL_UNSIGNED_BYTE,d->pixels);
  SDL_FreeSurface(d);
  glGenerateMipmap(GL_TEXTURE_2D);

  /* Texture 2 : ciel */
  glBindTexture(GL_TEXTURE_2D,_texId[2]);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  { SDL_Surface *s=IMG_Load("images/sky.jpg"); assert(s);
    SDL_Surface *d=SDL_CreateRGBSurface(0,s->w,s->h,32,R_MASK,G_MASK,B_MASK,A_MASK);
    SDL_BlitSurface(s,NULL,d,NULL); SDL_FreeSurface(s);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,d->w,d->h,0,GL_RGBA,GL_UNSIGNED_BYTE,d->pixels);
    SDL_FreeSurface(d); }
  glBindTexture(GL_TEXTURE_2D,0);

  /*  sol */
  GLfloat maxA;
  glBindTexture(GL_TEXTURE_2D,_texId[1]);
  glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY,&maxA);
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAX_ANISOTROPY,maxA);
  glBindTexture(GL_TEXTURE_2D,0);

  /* Feuillage */
  if(!_wall_tex) glGenTextures(1,&_wall_tex);
  glBindTexture(GL_TEXTURE_2D,_wall_tex);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
  { SDL_Surface *s=IMG_Load("images/leaves.png");
    if(s){ SDL_Surface *d=SDL_CreateRGBSurface(0,s->w,s->h,32,R_MASK,G_MASK,B_MASK,A_MASK);
      SDL_BlitSurface(s,NULL,d,NULL); SDL_FreeSurface(s);
      glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,d->w,d->h,0,GL_RGBA,GL_UNSIGNED_BYTE,d->pixels);
      SDL_FreeSurface(d);
    } else { GLuint g[4]={RGB(34,139,34),RGB(34,139,34),RGB(34,139,34),RGB(34,139,34)};
      glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,2,2,0,GL_RGBA,GL_UNSIGNED_BYTE,g); } }
  glBindTexture(GL_TEXTURE_2D,0);

  /* Tronc */
  if(!_wood_tex) glGenTextures(1,&_wood_tex);
  glBindTexture(GL_TEXTURE_2D,_wood_tex);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
  { SDL_Surface *s=IMG_Load("images/wood_t.jpeg");
    if(s){ SDL_Surface *d=SDL_CreateRGBSurface(0,s->w,s->h,32,R_MASK,G_MASK,B_MASK,A_MASK);
      SDL_BlitSurface(s,NULL,d,NULL); SDL_FreeSurface(s);
      glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,d->w,d->h,0,GL_RGBA,GL_UNSIGNED_BYTE,d->pixels);
      SDL_FreeSurface(d);
    } else { GLuint b[4]={RGB(101,67,33),RGB(101,67,33),RGB(101,67,33),RGB(101,67,33)};
      glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,2,2,0,GL_RGBA,GL_UNSIGNED_BYTE,b); } }
  glBindTexture(GL_TEXTURE_2D,0);
/* Texture cubes tombants */
  if(!_cube_tex) glGenTextures(1,&_cube_tex);
  glBindTexture(GL_TEXTURE_2D,_cube_tex);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
  { 
    SDL_Surface *s=IMG_Load("images/cube.jpg");
    if(s){ SDL_Surface *d=SDL_CreateRGBSurface(0,s->w,s->h,32,R_MASK,G_MASK,B_MASK,A_MASK);
      SDL_BlitSurface(s,NULL,d,NULL); SDL_FreeSurface(s);
      glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,d->w,d->h,0,GL_RGBA,GL_UNSIGNED_BYTE,d->pixels);
      SDL_FreeSurface(d);
    }
  }
  glBindTexture(GL_TEXTURE_2D,0);
  if(!_sun_tex) glGenTextures(1,&_sun_tex);
  glBindTexture(GL_TEXTURE_2D,_sun_tex);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  { SDL_Surface *s=IMG_Load("images/sun_shiny.png");
    if(s){ SDL_Surface *d=SDL_CreateRGBSurface(0,s->w,s->h,32,R_MASK,G_MASK,B_MASK,A_MASK);
      SDL_BlitSurface(s,NULL,d,NULL); SDL_FreeSurface(s);
      glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,d->w,d->h,0,GL_RGBA,GL_UNSIGNED_BYTE,d->pixels);
      SDL_FreeSurface(d);
    } else fprintf(stderr,"Erreur: images/sun_shiny.png non trouve\n");
  }
  glBindTexture(GL_TEXTURE_2D,0);

  /* Générer les étoiles une seule fois */
  if(!_stars_init) {
    _stars_init = 1;
    for(int i=0; i<MAX_STARS; i++) {
      float theta    = ((float)rand()/RAND_MAX) * 2.0f * (float)M_PI;
      float phi      = ((float)rand()/RAND_MAX) * (float)M_PI * 0.45f;
      float r        = 75.0f;
      _stars[i].x    = r * sinf(phi) * cosf(theta);
      _stars[i].y    = r * cosf(phi) + 3.0f;
      _stars[i].z    = r * sinf(phi) * sinf(theta);
      _stars[i].size = 0.12f + ((float)rand()/RAND_MAX) * 0.22f;
    }
  }
}

static void update_fall_cubes(double dt) {
  _spawn_fall_timer+=(float)dt;
  if(_spawn_fall_timer>0.8f){
    _spawn_fall_timer=0;
    for(int i=0;i<MAX_FALL_CUBES;i++) if(!_fall_cubes[i].active){
      float side = (rand()%2==0)? -1.0f : 1.0f;
      _fall_cubes[i].x = side * (3.0f + ((float)rand()/RAND_MAX)*1.8f);
      _fall_cubes[i].y      =18;
      _fall_cubes[i].z = _cam.z - 18.0f - ((float)rand()/RAND_MAX) * 35.0f;  
      _fall_cubes[i].vy     =0;
      _fall_cubes[i].bounced=0;
      _fall_cubes[i].scale  =0.3f+((float)rand()/RAND_MAX)*0.4f;
      _fall_cubes[i].active =1; break;
    }
  }
  float grav=(_scene>=4)?35.0f:(_scene>=3)?22.0f:FALL_GRAVITY;
  for(int i=0;i<MAX_FALL_CUBES;i++){
    if(!_fall_cubes[i].active||_fall_cubes[i].bounced) continue;
    _fall_cubes[i].vy-=grav*(float)dt;
    _fall_cubes[i].y +=_fall_cubes[i].vy*(float)dt;
    float gr=_fall_cubes[i].scale;
    if(_fall_cubes[i].y<=gr){
      _fall_cubes[i].y=gr; _fall_cubes[i].vy*=-0.3f;
      if(fabsf(_fall_cubes[i].vy)<0.5f){_fall_cubes[i].vy=0;_fall_cubes[i].bounced=1;}
    }
  }
}

static void update_leaves(double dt) {
  if(_demoTime < 15.0f)
    return;
  _spawn_leaf_timer+=(float)dt;
  if(_spawn_leaf_timer>0.15f){
    _spawn_leaf_timer=0;
    for(int i=0;i<MAX_LEAVES;i++) if(!_leaves[i].active){
      float side=((rand()%2)==0)?WALL_X_LEFT:WALL_X_RIGHT;
      _leaves[i].x    =side+((float)rand()/RAND_MAX-0.5f)*1.2f;
      _leaves[i].y    =1.5f+((float)rand()/RAND_MAX)*2.0f;
      _leaves[i].z    =_cam.z-2-((float)rand()/RAND_MAX)*25;
      _leaves[i].vx   =((float)rand()/RAND_MAX-0.5f)*0.4f;
      _leaves[i].vy   =-(0.4f+(float)rand()/RAND_MAX*0.6f);
      _leaves[i].vz   =((float)rand()/RAND_MAX-0.5f)*0.3f;
      _leaves[i].angle=((float)rand()/RAND_MAX)*360;
      _leaves[i].av   =60+((float)rand()/RAND_MAX)*120;
      _leaves[i].scale=0.08f+((float)rand()/RAND_MAX)*0.08f;
      _leaves[i].active=1; break;
    }
  }

  for(int i=0;i<MAX_LEAVES;i++){
    if(!_leaves[i].active) continue;
    _leaves[i].x+=_leaves[i].vx*(float)dt;
    _leaves[i].y+=_leaves[i].vy*(float)dt;
    _leaves[i].z+=_leaves[i].vz*(float)dt;
    _leaves[i].angle+=_leaves[i].av*(float)dt;
    _leaves[i].x+=sinf(_leaves[i].angle*0.05f)*0.005f;
    if(_leaves[i].y<=0.02f) _leaves[i].active=0;
  }
}
//avanceer la cam automatiquement
static void deplacement(double dt){
  _cam.x+=-3.0f*(float)dt*sinf(_cam.theta);
  _cam.z+=-3.0f*(float)dt*cosf(_cam.theta);
}

static void draw(void) {
  static double pt=0;
  if(_draw_needs_reset){_draw_needs_reset=0; pt=gl4dGetElapsedTime();}
  double t=gl4dGetElapsedTime(), dt=(t-pt)/1000.0; pt=t;

  _demoTime+=(float)dt;

  if     (_demoTime<15) _scene=1;
  else if(_demoTime<30) _scene=2;
  else if(_demoTime<45) _scene=3;
  else if(_demoTime<60) _scene=4;
  else if(_demoTime<75) _scene=5;
  else                  _scene=6;

  deplacement(dt);
  update_fall_cubes(dt);
  update_leaves(dt);
  float k = _demoTime / 20.0f;
  if(k > 1.0f) k = 1.0f;

  float sky_r = 0.0f  + 0.40f * k;
  float sky_g = 0.0f  + 0.70f * k;
  float sky_b = 0.05f + 0.95f * k;
  float bright = 0.15f + 0.85f * k;

  glClearColor(sky_r, sky_g, sky_b, 1);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

  glUseProgram(_pId);

  glActiveTexture(GL_TEXTURE0);
  glUniform1i(glGetUniformLocation(_pId,"tex"),0);
  glUniform1i(glGetUniformLocation(_pId,"sky"),0);
  glUniform1i(glGetUniformLocation(_pId,"fog"),_fog);
  glUniform2f(glGetUniformLocation(_pId,"tex_mult"),1,1);
  glUniform2f(glGetUniformLocation(_pId,"tex_translate"),0,0);

  GLint cloc=glGetUniformLocation(_pId,"couleur");
  GLint vp[4]; glGetIntegerv(GL_VIEWPORT,vp);

  gl4duBindMatrix("proj"); gl4duLoadIdentityf();
  gl4duFrustumf(-1,1,-vp[3]/(GLfloat)vp[2],vp[3]/(GLfloat)vp[2],2.0f,_plan_s);

  gl4duBindMatrix("view"); gl4duLoadIdentityf();
  gl4duLookAtf(_cam.x,_cam.y,_cam.z, _cam.x,_cam.y,_cam.z-10, 0,1,0);

  gl4duBindMatrix("model");

  /*SOL*/
  gl4duLoadIdentityf();
  gl4duTranslatef(_cam.x,0,_cam.z);
  gl4duRotatef(-90,1,0,0);
  gl4duScalef(_plan_s,_plan_s,1);
  gl4duSendMatrices();
  glUniform4f(cloc, bright, bright, bright, 1);
  glUniform2f(glGetUniformLocation(_pId,"tex_mult"),200,200);
  glBindTexture(GL_TEXTURE_2D,_texId[1]);
  gl4dgDraw(_plan);

  /*SKYDOME*/
  glCullFace(GL_FRONT);
  gl4duLoadIdentityf();
  gl4duTranslatef(_cam.x,0,_cam.z);
  gl4duScalef(_plan_s,_plan_s,_plan_s);
  gl4duSendMatrices();
  glUniform1i(glGetUniformLocation(_pId,"sky"),1);
  glUniform1i(glGetUniformLocation(_pId,"fog"),0);
  glUniform2f(glGetUniformLocation(_pId,"tex_mult"),1,-1);
  glUniform4f(cloc, sky_r, sky_g, sky_b, 1);
  glBindTexture(GL_TEXTURE_2D,_texId[2]);
  gl4dgDraw(_sky);
  glUniform1i(glGetUniformLocation(_pId,"sky"),0);
  glUniform1i(glGetUniformLocation(_pId,"fog"),_fog);
  glCullFace(GL_BACK);

  /* ÉTOILES (disparaissent avec k)  */
  float star_alpha = 1.0f - k;
  if(star_alpha > 0.01f) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glBindTexture(GL_TEXTURE_2D,_texId[0]);
    glUniform2f(glGetUniformLocation(_pId,"tex_mult"),1,1);
    glUniform2f(glGetUniformLocation(_pId,"tex_translate"),0,0);
    glUniform1i(glGetUniformLocation(_pId,"sky"),0);
    glUniform1i(glGetUniformLocation(_pId,"fog"),0);
    for(int i=0;i<MAX_STARS;i++){
      float twinkle=0.6f+0.4f*sinf(_demoTime*2.5f+i*1.3f);
      glUniform4f(cloc,1,1,0.95f,star_alpha*twinkle);
      gl4duLoadIdentityf();
      gl4duTranslatef(_cam.x+_stars[i].x, _stars[i].y, _cam.z+_stars[i].z);
      gl4duScalef(_stars[i].size,_stars[i].size,_stars[i].size);
      gl4duSendMatrices();
      gl4dgDraw(_sphere_geo);
    }
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
  }

  /* SOLEIL*/
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_CULL_FACE);

  float sunK=(_demoTime-3.0f)/17.0f;
  if(sunK < 0) sunK = 0;
  if(sunK > 1) sunK = 1;
  float sunX=_cam.x, sunY=-4.0f+22.0f*sunK, sunZ=_cam.z-80.0f;

  glBindTexture(GL_TEXTURE_2D,_sun_tex);
  glUniform4f(cloc,1,1,1,sunK);
  glUniform2f(glGetUniformLocation(_pId,"tex_mult"),1,1);
  glUniform2f(glGetUniformLocation(_pId,"tex_translate"),0,0);
  glUniform1i(glGetUniformLocation(_pId,"sky"),0);
  glUniform1i(glGetUniformLocation(_pId,"fog"),0);
  gl4duLoadIdentityf();
  gl4duTranslatef(sunX,sunY,sunZ);
  float pulse = 6.0f + sinf(_demoTime * 6.0f) * 0.4f + _audio_level * 2.0f;
  gl4duScalef(pulse, pulse, pulse);
  gl4duSendMatrices();
  gl4dgDraw(_quad_geo);
  glDisable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);

  /* Remettre uniforms normaux */
  glUniform1i(glGetUniformLocation(_pId,"fog"),_fog);
  glUniform2f(glGetUniformLocation(_pId,"tex_mult"),1,1);
  glUniform2f(glGetUniformLocation(_pId,"tex_translate"),0,0);

  /*ARBRES*/
  float grow=_demoTime/15.0f; if(grow>1) grow=1;

  for(int i=0;i<WALL_COUNT;i++){
    float pz=_cam.z-i*WALL_SPACING;

    glBindTexture(GL_TEXTURE_2D,_wood_tex);
    glUniform4f(cloc,bright,bright,bright,1);
    gl4duLoadIdentityf(); gl4duTranslatef(WALL_X_LEFT,grow,pz);
    gl4duScalef(0.25f,grow,0.25f); gl4duSendMatrices(); gl4dgDraw(_cube_geo);

    glBindTexture(GL_TEXTURE_2D,_wall_tex);
    gl4duLoadIdentityf(); gl4duTranslatef(WALL_X_LEFT,2*grow+0.5f,pz);
    gl4duScalef(0.9f*grow,0.9f*grow,0.9f*grow);
    gl4duSendMatrices(); gl4dgDraw(_cube_geo);

    glBindTexture(GL_TEXTURE_2D,_wood_tex);
    glUniform4f(cloc,bright,bright,bright,1);
    gl4duLoadIdentityf(); gl4duTranslatef(WALL_X_RIGHT,grow,pz);
    gl4duScalef(0.25f,grow,0.25f); gl4duSendMatrices(); gl4dgDraw(_cube_geo);

    glBindTexture(GL_TEXTURE_2D,_wall_tex);
    gl4duLoadIdentityf(); gl4duTranslatef(WALL_X_RIGHT,2*grow+0.5f,pz);
    gl4duScalef(0.9f*grow,0.9f*grow,0.9f*grow);
    gl4duSendMatrices(); gl4dgDraw(_cube_geo);
  }

  /*CUBES TOMBANTS*/
  glBindTexture(GL_TEXTURE_2D,_cube_tex);
  glUniform4f(cloc,bright,bright,bright,1);
  glUniform2f(glGetUniformLocation(_pId,"tex_mult"),1,1);
  for(int i=0;i<MAX_FALL_CUBES;i++){
    if(!_fall_cubes[i].active) continue;
    gl4duLoadIdentityf();
    gl4duTranslatef(_fall_cubes[i].x,_fall_cubes[i].y,_fall_cubes[i].z);
    gl4duScalef(_fall_cubes[i].scale,_fall_cubes[i].scale,_fall_cubes[i].scale);
    gl4duSendMatrices(); gl4dgDraw(_cube_geo);
  }

  /*FEUILLES*/
  glDisable(GL_CULL_FACE);
  glBindTexture(GL_TEXTURE_2D,_wall_tex);
  for(int i=0;i<MAX_LEAVES;i++){
    if(!_leaves[i].active) continue;
    glUniform4f(cloc,0.2f*bright,0.9f*bright,0.2f*bright,1);
    gl4duLoadIdentityf();
    gl4duTranslatef(_leaves[i].x,_leaves[i].y,_leaves[i].z);
    gl4duRotatef(_leaves[i].angle,0,1,0);
    gl4duScalef(_leaves[i].scale,_leaves[i].scale,_leaves[i].scale);
    gl4duSendMatrices(); gl4dgDraw(_quad_geo);
  }
  glEnable(GL_CULL_FACE);
  glBindTexture(GL_TEXTURE_2D,0);
  glUseProgram(0);
}

static void quit(void){
  if(_texId[0] ){glDeleteTextures(3,_texId);_texId[0]=0;}
  if(_wall_tex){glDeleteTextures(1,&_wall_tex);_wall_tex=0;}
  if(_wood_tex){glDeleteTextures(1,&_wood_tex);_wood_tex=0;}
  if(_sun_tex) {glDeleteTextures(1,&_sun_tex); _sun_tex=0;}
  if(_cube_tex){glDeleteTextures(1,&_cube_tex);_cube_tex=0;}
}

