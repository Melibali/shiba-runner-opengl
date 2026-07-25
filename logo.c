/*!\file logo.c
 * \brief Ecran d'introduction et credits de la demo SHIBA RUNNER.
 * \author Melissa
 */
#include <GL4D/gl4dh.h>
#include <GL4D/gl4dg.h>
#include <GL4D/gl4du.h>
#include <SDL_ttf.h>
#include <assert.h>
static GLuint _quad = 0;
static GLuint _tex  = 0;
static GLuint _pId  = 0;

/* Fonction helper : dessine un texte centre horizontalement */
static void drawText(SDL_Surface *dest,TTF_Font *font,const char *text,SDL_Color color,int y)
{
  SDL_Surface *txt;
  SDL_Rect r;
  txt = TTF_RenderUTF8_Blended(font, text, color);
  assert(txt);
  r.x = (dest->w - txt->w) / 2;
  r.y = y;
  SDL_BlitSurface(txt, NULL, dest, &r);
  SDL_FreeSurface(txt);
}
/* Scene logo                                                */
void logo(int state) {
  switch(state) {
  /* INIT                                                   */
  case GL4DH_INIT: {

    if(!_quad)
      _quad = gl4dgGenQuadf();
    /* shader */
    _pId = gl4duCreateProgram("<vs>shaders/basic.vs","<fs>shaders/basic.fs",NULL);

    /* SDL_ttf */
    TTF_Init();

    /* polices */
    TTF_Font *fontBig   = TTF_OpenFont("DejaVuSans-Bold.ttf", 72);
    TTF_Font *fontSmall = TTF_OpenFont("DejaVuSans-Bold.ttf", 28);
    TTF_Font *fontTiny  = TTF_OpenFont("DejaVuSans-Bold.ttf", 20);

    assert(fontBig && fontSmall && fontTiny);

    /* image finale */
    int w = 1024, h = 768;
    SDL_Surface *surf = SDL_CreateRGBSurface(0, w, h, 32,R_MASK, G_MASK, B_MASK, A_MASK);
    assert(surf);

    /* fond noir */
    SDL_FillRect(
      surf,
      NULL,
      SDL_MapRGBA(surf->format, 0, 0, 0, 255));

    /* couleurs */
    SDL_Color white = {255,255,255,255};
    SDL_Color gold  = {255,210,0,255};
    SDL_Color gray  = {160,160,160,255};

    /* textes */
    drawText(surf, fontSmall, "DEMO", white, 200);
    drawText(surf, fontBig, "SHIBA RUNNER", gold, 310);
    drawText(surf, fontTiny,"Music : \"happy 96\" by dwayne wayne",gray, 660);
    drawText(surf, fontTiny,"Mod Archive Distribution License - modarchive.org",gray, 688);
    drawText(surf, fontTiny,"Model 3D : \"Shiba\" par zixisun02 - sketchfab.com - CC-BY 4.0",gray, 716);
    drawText(surf, fontTiny,"Code : Melissa",gray, 744);
    /* texture OpenGL */
    glGenTextures(1, &_tex);
    glBindTexture(GL_TEXTURE_2D, _tex);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,surf->pixels);
    /* nettoyage */
    SDL_FreeSurface(surf);

    TTF_CloseFont(fontBig);
    TTF_CloseFont(fontSmall);
    TTF_CloseFont(fontTiny);

    return;
  }

  /* FREE                                                   */
  case GL4DH_FREE:

    if(_tex)
      glDeleteTextures(1, &_tex);

    TTF_Quit();

    return;

  /* DRAW                                                   */
  default: {

    static float t = 0.0f;
    t += 0.02f;

    glDisable(GL_DEPTH_TEST);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(_pId);

    /* texture */
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _tex);

    glUniform1i(glGetUniformLocation(_pId, "myTexture"),0);

    /* inversion verticale */
    glUniform1i(glGetUniformLocation(_pId, "inv"),1);
    /* temps animation shader */
    glUniform1f(glGetUniformLocation(_pId, "time"),t);

    /* zoom progressif */
    float zoom = (t > 2.0f)
      ? 1.0f + (t - 2.0f) * 0.035f
      : 1.0f;

    glUniform1f(
      glGetUniformLocation(_pId, "zoom"),
      zoom);

    /* fade-in puis teinte dorée */
    if(t < 2.0f)
      glUniform4f(
        glGetUniformLocation(_pId, "colorTint"),
        1.0f, 1.0f, 1.0f,
        t / 2.0f);
    else
      glUniform4f(glGetUniformLocation(_pId, "colorTint"),1.0f, 0.82f, 0.15f,1.0f);

    /* draw plein écran */
    gl4dgDraw(_quad);

    return;
  }
  }
}