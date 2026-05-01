#include <stdio.h>
#include "pico/stdlib.h"
#include "audio_player.h"

int main() {
    stdio_init_all();
    
    // Oferim 3 secunde utilizatorului să deschidă Serial Monitor-ul
    sleep_ms(9000); 

    printf("\n=============================================\n");
    printf("   TEST I2S HARDWARE - ECHO NOTE PICO 2      \n");
    printf("=============================================\n");
    printf("Tasteaza un caracter in Serial Monitor:\n");
    printf(" [1] -> Reda sunet 440 Hz (Nota La)\n");
    printf(" [2] -> Reda sunet 880 Hz (Nota La inalta)\n");
    printf(" [0] -> Opreste sunetul\n");
    printf("=============================================\n\n");

    // Inițializăm strict pinii I2S, MAX98357A, DMA și PIO
    audio_init(); 

    while (true) {
        // Menține bufferele DMA pline (necesar pentru redare continuă)
        audio_task(); 

        // Citim non-blocking caractere de pe portul serial (USB)
        int c = getchar_timeout_us(0);
        if (c != PICO_ERROR_TIMEOUT) {
            switch(c) {
                case '1':
                    printf(">> Pornire generare: Unda patrata 440 Hz...\n");
                    audio_play_test_tone(440);
                    break;
                case '2':
                    printf(">> Pornire generare: Unda patrata 880 Hz...\n");
                    audio_play_test_tone(880);
                    break;
                case '0':
                    printf(">> Oprire sunet.\n");
                    audio_stop();
                    break;
                case '\n':
                case '\r':
                    // Ignorăm Enter-ul
                    break;
                default:
                    printf("Comanda necunoscuta: %c\n", c);
                    break;
            }
        }

        // Previne monopolizarea CPU-ului
        sleep_us(500); 
    }

    return 0;
}