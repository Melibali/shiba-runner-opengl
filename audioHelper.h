/*!\file audioHelper.h
 * \brief Interface du module audio SDL_Mixer.
 */
#ifndef AUDIOHELPER_H
#define AUDIOHELPER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <GL4D/gl4dh.h>

Uint8 * ahGetAudioStream(void);
int     ahGetAudioStreamLength(void);
void    ahSetAudioStream(Uint8 * audioStream, int audioStreamLength);
void    ahInitAudio(const char * file);
void    ahClean(void);

#endif /* AUDIOHELPER_H */
