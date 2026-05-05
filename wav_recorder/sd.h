#ifndef SD_H_
#define SD_H_

#define MAX_FILES_TO_DISPLAY 20
#define MAX_FILENAME_LEN     256   // <-- definit o singura data aici

#include "ff.h"
#include "sd_card.h"
#include <string.h>

extern volatile int total_files_found;
extern char wav_files[MAX_FILES_TO_DISPLAY][MAX_FILENAME_LEN];

void init_sd_card(void);
void scan_sd_for_wavs(void);

#endif