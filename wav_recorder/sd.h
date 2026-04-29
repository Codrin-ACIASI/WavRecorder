#ifndef SD_H_
#define SD_H_

#define MAX_FILES_TO_DISPLAY 10

#include "ff.h"
#include "sd_card.h"
#include <string.h>

extern volatile int total_files_found;
extern char wav_files[MAX_FILES_TO_DISPLAY][32]; 
extern FATFS sd_fs; // Obiectul de sistem de fișiere
void scan_sd_for_wavs(void);


#endif

