#include "audioHelper.h"
static Mix_Music * _mmusic = NULL;
static Uint8 * _audioStream = NULL;
static int _audioStreamLength = 0;
Uint8 * ahGetAudioStream(void) {
  return _audioStream;
}
int ahGetAudioStreamLength(void) {
  return _audioStreamLength;
}

void ahSetAudioStream(Uint8 * audioStream, int audioStreamLength) {
  _audioStream = audioStream;
  _audioStreamLength = audioStreamLength;
}

static void mixCallback(void *udata, Uint8 *stream, int len) {
  ahSetAudioStream(stream, len);
  gl4dhUpdateWithAudio();
  ahSetAudioStream(NULL, 0);
}
void ahInitAudio(const char * file) {
  int mixFlags = MIX_INIT_OGG | MIX_INIT_MP3, res;
  res = Mix_Init(mixFlags);
  if( (res & mixFlags) != mixFlags ) {
    fprintf(stderr, "Mix_Init: Erreur lors de l'initialisation de la bibliotheque SDL_Mixer\n");
    fprintf(stderr, "Mix_Init: %s\n", Mix_GetError());
    exit(-3);
  }
  if(Mix_OpenAudio(44100, AUDIO_S16LSB, 2, 1024) < 0)
    exit(-4);
  if(!(_mmusic = Mix_LoadMUS(file))) {
    fprintf(stderr, "Erreur lors du Mix_LoadMUS: %s\n", Mix_GetError());
    exit(-5);
  }
  Mix_SetPostMix(mixCallback, NULL);
  if(!Mix_PlayingMusic())
    Mix_PlayMusic(_mmusic, 1);
  gl4dhStartingSignal();
}
void ahClean(void) {
  if(_mmusic) {
    if(Mix_PlayingMusic())
      Mix_HaltMusic();
    Mix_FreeMusic(_mmusic);
    _mmusic = NULL;
  }
  Mix_CloseAudio();
  Mix_Quit();
}
