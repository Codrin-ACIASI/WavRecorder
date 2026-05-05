#include "audio_player.h"
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "i2s_tx.pio.h"
#include "ff.h"
#include <string.h>
#include <stdio.h>

#define I2S_DATA_PIN 13
#define I2S_BCLK_PIN 14
#define I2S_LRCLK_PIN 15
#define I2S_AMP_SD   16

#define AUDIO_FRAMES 2048

uint32_t audio_buffer_0[AUDIO_FRAMES];
uint32_t audio_buffer_1[AUDIO_FRAMES];

static int dma_ch_0;
static int dma_ch_1;
static uint pio_sm;
static PIO pio_inst = pio0;
static uint pio_offset;

volatile bool buffer_0_needs_fill = false;
volatile bool buffer_1_needs_fill = false;
volatile bool is_playing = false;

// --- NOU: Memorie tampon mica doar pentru citirea in transe ---
static int16_t sd_read_buffer[AUDIO_FRAMES]; 
static uint32_t wav_data_bytes_remaining = 0; // Tine minte cat mai avem din melodie

static uint16_t wav_channels = 1;
static uint32_t wav_sample_rate = 16000;

static FIL current_wav;

/* ================= WAV PARSER ================= */

bool parse_wav_header(FIL *file) {
    uint8_t header[44];
    UINT br;

    f_lseek(file, 0);
    if (f_read(file, header, 44, &br) != FR_OK || br != 44)
        return false;

    if (memcmp(header, "RIFF", 4) != 0 || memcmp(header + 8, "WAVE", 4) != 0)
        return false;

    wav_channels = header[22] | (header[23] << 8);
    wav_sample_rate = header[24] | (header[25] << 8) |
                      (header[26] << 16) | (header[27] << 24);

    uint16_t bits = header[34] | (header[35] << 8);
    if (bits != 16) return false;

    uint32_t offset = 12;
    uint32_t data_size = 0;

    while (offset < 2048) {
        f_lseek(file, offset);
        if (f_read(file, header, 8, &br) != FR_OK || br != 8)
            break;

        uint32_t chunk_size = header[4] | (header[5] << 8) |
                              (header[6] << 16) | (header[7] << 24);

        if (memcmp(header, "data", 4) == 0) {
            data_size = chunk_size;
            break;
        }

        offset += 8 + chunk_size;
    }

    if (data_size == 0)
        return false;

    // --- NOU: Doar mutam "cursorul" fisierului la inceputul muzicii ---
    f_lseek(file, offset + 8);
    
    // Salvam dimensiunea ca sa stim cand se termina piesa
    wav_data_bytes_remaining = data_size; 

    // ATENTIE: NU mai inchidem fisierul cu f_close aici! Trebuie sa ramana 
    // deschis ca sa il citim in timp ce canta.
    return true;
}

/* ================= BUFFER (STREAMING LOGIC) ================= */

void fill_buffer(uint32_t *dest) {
    if (!is_playing) {
        memset(dest, 0, AUDIO_FRAMES * 4);
        return;
    }

    // Daca am ajuns la finalul fisierului SD
    if (wav_data_bytes_remaining == 0) {
        memset(dest, 0, AUDIO_FRAMES * 4);
        audio_stop();
        return;
    }

    // Calculam cati bytes trebuie sa citim in aceasta tura (maxim 4096 bytes / 2048 frame-uri)
    uint32_t bytes_to_read = AUDIO_FRAMES * 2; 
    if (bytes_to_read > wav_data_bytes_remaining) {
        bytes_to_read = wav_data_bytes_remaining; // Citim doar ce a mai ramas la final de melodie
    }

    // Curatam buffer-ul temporar in caz ca e finalul piesei si ramane loc gol
    memset(sd_read_buffer, 0, sizeof(sd_read_buffer));

    UINT bytes_read = 0;
    // Citim "o cana cu apa" de pe cardul SD direct in RAM
    FRESULT fr = f_read(&current_wav, sd_read_buffer, bytes_to_read, &bytes_read);
    
    // Daca a fost o eroare de card sau nu s-a citit nimic
    if (fr != FR_OK || bytes_read == 0) {
        memset(dest, 0, AUDIO_FRAMES * 4);
        audio_stop();
        return;
    }

    wav_data_bytes_remaining -= bytes_read;
    int samples_read = bytes_read / 2; // Un sample de 16-biti = 2 bytes

    // Impachetam sunetul Mono in formatul cerut de PIO pentru canal dublu (Stanga/Dreapta)
    for (int i = 0; i < AUDIO_FRAMES; i++) {
        int16_t sample = 0;
        
        if (i < samples_read) {
            sample = sd_read_buffer[i];
        }

        dest[i] = ((uint32_t)(uint16_t)sample << 16) | (uint16_t)sample;
    }
}

/* ================= DMA IRQ ================= */

void dma_irq_handler() {
    if (dma_hw->ints0 & (1u << dma_ch_0)) {
        dma_hw->ints0 = (1u << dma_ch_0);
        dma_channel_set_read_addr(dma_ch_0, audio_buffer_0, false);
        dma_channel_set_trans_count(dma_ch_0, AUDIO_FRAMES, false);
        buffer_0_needs_fill = true; // "Galeta 0 s-a golit, trebuie umpluta!"
    }

    if (dma_hw->ints0 & (1u << dma_ch_1)) {
        dma_hw->ints0 = (1u << dma_ch_1);
        dma_channel_set_read_addr(dma_ch_1, audio_buffer_1, false);
        dma_channel_set_trans_count(dma_ch_1, AUDIO_FRAMES, false);
        buffer_1_needs_fill = true; // "Galeta 1 s-a golit, trebuie umpluta!"
    }
}

/* ================= INIT ================= */

void audio_init() {
    gpio_init(I2S_AMP_SD);
    gpio_set_dir(I2S_AMP_SD, GPIO_OUT);
    gpio_put(I2S_AMP_SD, 0);

    pio_offset = pio_add_program(pio_inst, &i2s_tx_program);
    pio_sm = pio_claim_unused_sm(pio_inst, true);

    pio_sm_config c = i2s_tx_program_get_default_config(pio_offset);

    sm_config_set_out_pins(&c, I2S_DATA_PIN, 1);
    sm_config_set_sideset_pins(&c, I2S_BCLK_PIN);
    sm_config_set_out_shift(&c, false, true, 32);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);

    pio_gpio_init(pio_inst, I2S_DATA_PIN);
    pio_gpio_init(pio_inst, I2S_BCLK_PIN);
    pio_gpio_init(pio_inst, I2S_LRCLK_PIN);

    pio_sm_set_consecutive_pindirs(pio_inst, pio_sm, I2S_DATA_PIN, 1, true);
    pio_sm_set_consecutive_pindirs(pio_inst, pio_sm, I2S_BCLK_PIN, 3, true);
    pio_sm_init(pio_inst, pio_sm, pio_offset, &c);

    dma_ch_0 = dma_claim_unused_channel(true);
    dma_ch_1 = dma_claim_unused_channel(true);

    dma_channel_config c0 = dma_channel_get_default_config(dma_ch_0);
    channel_config_set_transfer_data_size(&c0, DMA_SIZE_32);
    channel_config_set_read_increment(&c0, true);
    channel_config_set_write_increment(&c0, false);
    channel_config_set_dreq(&c0, pio_get_dreq(pio_inst, pio_sm, true));
    channel_config_set_chain_to(&c0, dma_ch_1);

    dma_channel_configure(dma_ch_0, &c0,
        &pio_inst->txf[pio_sm],
        audio_buffer_0,
        AUDIO_FRAMES,
        false);

    dma_channel_config c1 = dma_channel_get_default_config(dma_ch_1);
    channel_config_set_transfer_data_size(&c1, DMA_SIZE_32);
    channel_config_set_read_increment(&c1, true);
    channel_config_set_write_increment(&c1, false);
    channel_config_set_dreq(&c1, pio_get_dreq(pio_inst, pio_sm, true));
    channel_config_set_chain_to(&c1, dma_ch_0);

    dma_channel_configure(dma_ch_1, &c1,
        &pio_inst->txf[pio_sm],
        audio_buffer_1,
        AUDIO_FRAMES,
        false);

    dma_channel_set_irq0_enabled(dma_ch_0, true);
    dma_channel_set_irq0_enabled(dma_ch_1, true);

    irq_set_exclusive_handler(DMA_IRQ_0, dma_irq_handler);
    irq_set_enabled(DMA_IRQ_0, true);
}

/* ================= PLAY / STOP ================= */

bool audio_play_wav(const char *filename) {
    if (is_playing) audio_stop();

    if (f_open(&current_wav, filename, FA_READ) != FR_OK)
        return false;

    if (!parse_wav_header(&current_wav)){
        f_close(&current_wav);
        return false;
    }

    uint32_t sys_clk = clock_get_hz(clk_sys);
    uint32_t bclk = wav_sample_rate * 2 * 16;
    float div = (float)sys_clk / (bclk * 2.0f);
    pio_sm_set_clkdiv(pio_inst, pio_sm, div);

    pio_sm_restart(pio_inst, pio_sm);
    pio_sm_exec(pio_inst, pio_sm, pio_encode_jmp(pio_offset));

    buffer_0_needs_fill = false;
    buffer_1_needs_fill = false;

    gpio_put(I2S_AMP_SD, 1);
    is_playing = true;

    fill_buffer(audio_buffer_0);
    fill_buffer(audio_buffer_1);

    dma_channel_set_read_addr(dma_ch_0, audio_buffer_0, false);
    dma_channel_set_trans_count(dma_ch_0, AUDIO_FRAMES, false);
    dma_channel_set_read_addr(dma_ch_1, audio_buffer_1, false);
    dma_channel_set_trans_count(dma_ch_1, AUDIO_FRAMES, false);

    pio_sm_set_enabled(pio_inst, pio_sm, true);
    dma_channel_start(dma_ch_0);

    return true;
}

void audio_stop() {
    if (!is_playing) return;

    is_playing = false;
    wav_data_bytes_remaining = 0;

    dma_channel_abort(dma_ch_0);
    dma_channel_abort(dma_ch_1);

    pio_sm_set_enabled(pio_inst, pio_sm, false);
    pio_sm_clear_fifos(pio_inst, pio_sm);

    pio_sm_restart(pio_inst, pio_sm);
    pio_sm_exec(pio_inst, pio_sm, pio_encode_jmp(pio_offset));

    gpio_put(I2S_AMP_SD, 0);

    // Am pastrat fisierul deschis (comentat) pentru functionalitatea de Auto-Rewind a UI-ului tau
    // f_close(&current_wav); 
    
    buffer_0_needs_fill = false;
    buffer_1_needs_fill = false;
}

void audio_pause(bool pause) {
    if (!is_playing) return;

    if (pause) {
        pio_sm_set_enabled(pio_inst, pio_sm, false);
        gpio_put(I2S_AMP_SD, 0);
    } else {
        gpio_put(I2S_AMP_SD, 1);
        pio_sm_set_enabled(pio_inst, pio_sm, true);
    }
}

void audio_task() {
    // Aceasta functie e strigata in bucla principala (Main Loop)
    // Aici are loc streaming-ul efectiv cand procesorul e "liber"
    
    if (buffer_0_needs_fill) {
        fill_buffer(audio_buffer_0);
        buffer_0_needs_fill = false;
    }

    if (buffer_1_needs_fill) {
        fill_buffer(audio_buffer_1);
        buffer_1_needs_fill = false;
    }
}

bool audio_is_playing() {
    return is_playing;
}