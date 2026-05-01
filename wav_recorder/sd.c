#include "sd.h"
#include "ff.h"
#include "sd_card.h"
#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"

// FIX: marit de la 32 la 256 pentru a suporta nume lungi (LFN)
char wav_files[MAX_FILES_TO_DISPLAY][256];
FATFS sd_fs;
volatile int total_files_found = 0;

void init_sd_card(void) {
    sd_init_driver();
    sleep_ms(200);

    FRESULT res = f_mount(&sd_fs, "0:", 1);
    if (res != FR_OK) {
        printf("Eroare FatFs: Nu am putut monta cardul SD la initializare! (Cod: %d)\n", res);
    }
}

void scan_sd_for_wavs(void) {
    DIR     dir;
    FILINFO fno;
    FRESULT res;

    total_files_found = 0;

    // FIX: eliminat al doilea f_mount() - SD-ul este deja montat de init_sd_card()
    // Daca totusi SD-ul nu e montat (ex: prima rulare fara init), incercam o data.
    res = f_opendir(&dir, "/");
    if (res != FR_OK) {
        // Incercam sa montam daca directorul nu e accesibil
        res = f_mount(&sd_fs, "0:", 1);
        if (res != FR_OK) {
            printf("Eroare FatFs: Nu am putut monta cardul SD! (Cod: %d)\n", res);
            return;
        }
        res = f_opendir(&dir, "/");
    }

    if (res == FR_OK) {
        while (total_files_found < MAX_FILES_TO_DISPLAY) {
            res = f_readdir(&dir, &fno);

            if (res != FR_OK || fno.fname[0] == 0) {
                break;
            }

            if (!(fno.fattrib & AM_DIR)) {
                int len = strlen(fno.fname);
                if (len > 4 &&
                    (strcmp(&fno.fname[len - 4], ".WAV") == 0 ||
                     strcmp(&fno.fname[len - 4], ".wav") == 0)) {

                    // FIX: folosim strncpy cu limita pentru a evita overflow
                    strncpy(wav_files[total_files_found], fno.fname, 255);
                    wav_files[total_files_found][255] = '\0';
                    total_files_found++;
                }
            }
        }
        f_closedir(&dir);
    } else {
        printf("Eroare FatFs: Nu am putut deschide directorul principal!\n");
    }
}