#ifndef AUDIO_PLAYER_H_
#define AUDIO_PLAYER_H_

#include <stdbool.h>
#include <stdint.h>

void audio_player_init(void);
bool audio_player_play(const char* filename);
void audio_player_pause(void);
void audio_player_resume(void);
void audio_player_stop(void);
bool audio_player_is_playing(void);

#endif