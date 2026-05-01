#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <stdint.h>
#include <stdbool.h>

void audio_init(void);
bool audio_play_wav(const char *filename);
void audio_stop(void);
void audio_pause(bool pause);
void audio_task(void);
bool audio_is_playing(void);

// NOU: Funcție pentru a reda un sunet sintetic de test
void audio_play_test_tone(uint32_t freq);

#endif // AUDIO_PLAYER_H