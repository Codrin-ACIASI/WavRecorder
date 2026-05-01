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
#define I2S_BCLK_PIN 14 // LRCLK va fi pinul 15 implicit
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

// --- RAM BUFFER LOGIC ---
// Pico 2 are suficient RAM (~520KB). Vom aloca un buffer masiv pt a încăpea un fișier întreg scurt (ex: 3 sec de 16kHz mono = ~96KB)
#define MAX_RAM_WAV_BYTES 120000 
static int16_t ram_wav_buffer[MAX_RAM_WAV_BYTES / 2]; // Împărțim la 2 pt că sunt int16_t
static uint32_t ram_wav_samples_total = 0;
static uint32_t ram_wav_current_sample_index = 0;
static uint16_t wav_channels = 1;
static uint32_t wav_sample_rate = 16000;
// -------------------------

static FIL current_wav;
static bool play_test_tone = false;
static uint32_t tone_freq = 440;
static uint32_t tone_phase = 0;

static bool parse_wav_header(FIL *file) {
    uint8_t header[44];
    UINT br;
    
    f_lseek(file, 0); 
    if (f_read(file, header, 44, &br) != FR_OK || br != 44) {
        printf("[AUDIO ERR] Esec citire header!\n");
        return false;
    }
    
    if (memcmp(header, "RIFF", 4) != 0 || memcmp(header + 8, "WAVE", 4) != 0) {
        return false;
    }

    wav_channels = header[22] | (header[23] << 8);
    wav_sample_rate = header[24] | (header[25] << 8) | (header[26] << 16) | (header[27] << 24);
    uint16_t bits_per_sample = header[34] | (header[35] << 8);
    
    if (bits_per_sample != 16) return false;

    uint32_t offset = 12; 
    uint32_t data_chunk_size = 0;
    while (offset < 2048) { 
        f_lseek(file, offset);
        if (f_read(file, header, 8, &br) != FR_OK || br != 8) break;
        
        uint32_t chunk_size = header[4] | (header[5] << 8) | (header[6] << 16) | (header[7] << 24);
        if (memcmp(header, "data", 4) == 0) {
            data_chunk_size = chunk_size;
            printf("[AUDIO INFO] Found 'data' chunk at offset %lu, size: %lu\n", offset, chunk_size);
            break; // Found it, stop searching
        }
        offset += 8 + chunk_size;
    }

    if (data_chunk_size == 0) {
        printf("[AUDIO ERR] Nu am gasit chunk data!\n");
        return false;
    }

    // --- ÎNCĂRCARE COMPLETĂ ÎN RAM ---
    // Citim absolut toate datele din fișier, direct în memoria RAM, acum, la start.
    uint32_t bytes_to_read = data_chunk_size;
    if (bytes_to_read > MAX_RAM_WAV_BYTES) {
        printf("[AUDIO WARN] Fisierul este prea mare pt RAM (%lu bytes). Vom citi doar primii %d bytes.\n", bytes_to_read, MAX_RAM_WAV_BYTES);
        bytes_to_read = MAX_RAM_WAV_BYTES;
    }

    f_lseek(file, offset + 8); // Salt la începutul efectiv al datelor PCM
    
    printf("[AUDIO] Incarcam %lu bytes in RAM... Asteptati...\n", bytes_to_read);
    
    FRESULT res = f_read(file, ram_wav_buffer, bytes_to_read, &br);
    if (res != FR_OK) {
         printf("[AUDIO ERR] Eroare citire in RAM: %d\n", res);
         return false;
    }
    
    ram_wav_samples_total = br / sizeof(int16_t);
    ram_wav_current_sample_index = 0;
    
    printf("[AUDIO] Gata! S-au incarcat %lu esantioane.\n", ram_wav_samples_total);

    // Am terminat cu cardul SD pentru redarea acestui fișier
    f_close(file); 
    
    return true;
}

static void fill_buffer(uint32_t *dest_buffer) {
    if (!is_playing) {
        memset(dest_buffer, 0, AUDIO_FRAMES * sizeof(uint32_t));
        return;
    }

    if (play_test_tone) {
        int half_period = 44100 / (tone_freq * 2);
        for (int i = 0; i < AUDIO_FRAMES; i++) {
            int16_t sample = (tone_phase < half_period) ? 10000 : -10000;
            dest_buffer[i] = ((uint32_t)(uint16_t)sample << 16) | (uint16_t)sample;
            tone_phase++;
            if (tone_phase >= half_period * 2) tone_phase = 0;
        }
        return;
    }

    // --- CITIM DIN RAM, NU DE PE SD ---
    if (ram_wav_current_sample_index >= ram_wav_samples_total) {
        memset(dest_buffer, 0, AUDIO_FRAMES * sizeof(uint32_t));
        audio_stop(); // Am ajuns la finalul fișierului din memorie
        return;
    }

    int samples_to_read = AUDIO_FRAMES;
    if (wav_channels == 2) {
         samples_to_read = AUDIO_FRAMES * 2;
    }

    // Câte eșantioane mai avem disponibile?
    uint32_t samples_left = ram_wav_samples_total - ram_wav_current_sample_index;
    if (samples_to_read > samples_left) {
        samples_to_read = samples_left;
    }

    int volume_multiplier = 1; 

    if (wav_channels == 2) {
        for (int i = 0; i < AUDIO_FRAMES; i++) {
            if (i * 2 < samples_to_read) {
                // Citim din buffer-ul static
                int32_t sample_l = ram_wav_buffer[ram_wav_current_sample_index++] * volume_multiplier;
                int32_t sample_r = ram_wav_buffer[ram_wav_current_sample_index++] * volume_multiplier;
                
                if(sample_l > 32767) sample_l = 32767; else if(sample_l < -32768) sample_l = -32768;
                if(sample_r > 32767) sample_r = 32767; else if(sample_r < -32768) sample_r = -32768;
                
                dest_buffer[i] = ((uint32_t)(uint16_t)sample_l << 16) | (uint16_t)sample_r;
            } else {
                dest_buffer[i] = 0;
            }
        }
    } else { 
        // MONO (Cum e scriptul tău)
        for (int i = 0; i < AUDIO_FRAMES; i++) {
            if (i < samples_to_read) {
                // Citim din buffer-ul static
                int32_t sample = ram_wav_buffer[ram_wav_current_sample_index++] * volume_multiplier;
                
                if(sample > 32767) sample = 32767; else if(sample < -32768) sample = -32768;
                
                dest_buffer[i] = ((uint32_t)(uint16_t)sample << 16) | (uint16_t)sample;
            } else {
                dest_buffer[i] = 0;
            }
        }
    }
}

static void dma_irq_handler() {
    if (dma_hw->ints0 & (1u << dma_ch_0)) {
        dma_hw->ints0 = (1u << dma_ch_0); 
        dma_channel_set_read_addr(dma_ch_0, audio_buffer_0, false);
        dma_channel_set_trans_count(dma_ch_0, AUDIO_FRAMES, false);
        buffer_0_needs_fill = true;
    }
    if (dma_hw->ints0 & (1u << dma_ch_1)) {
        dma_hw->ints0 = (1u << dma_ch_1); 
        dma_channel_set_read_addr(dma_ch_1, audio_buffer_1, false);
        dma_channel_set_trans_count(dma_ch_1, AUDIO_FRAMES, false); 
        buffer_1_needs_fill = true;
    }
}

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
    pio_gpio_init(pio_inst, I2S_BCLK_PIN + 1); 
    
    pio_sm_set_consecutive_pindirs(pio_inst, pio_sm, I2S_DATA_PIN, 1, true);
    pio_sm_set_consecutive_pindirs(pio_inst, pio_sm, I2S_BCLK_PIN, 2, true);

    pio_sm_init(pio_inst, pio_sm, pio_offset, &c);

    dma_ch_0 = dma_claim_unused_channel(true);
    dma_ch_1 = dma_claim_unused_channel(true);

    dma_channel_config c0 = dma_channel_get_default_config(dma_ch_0);
    channel_config_set_transfer_data_size(&c0, DMA_SIZE_32);
    channel_config_set_read_increment(&c0, true);
    channel_config_set_write_increment(&c0, false);
    channel_config_set_dreq(&c0, pio_get_dreq(pio_inst, pio_sm, true));
    channel_config_set_chain_to(&c0, dma_ch_1); 

    dma_channel_configure(dma_ch_0, &c0, &pio_inst->txf[pio_sm], audio_buffer_0, AUDIO_FRAMES, false);

    dma_channel_config c1 = dma_channel_get_default_config(dma_ch_1);
    channel_config_set_transfer_data_size(&c1, DMA_SIZE_32);
    channel_config_set_read_increment(&c1, true);
    channel_config_set_write_increment(&c1, false);
    channel_config_set_dreq(&c1, pio_get_dreq(pio_inst, pio_sm, true));
    channel_config_set_chain_to(&c1, dma_ch_0); 

    dma_channel_configure(dma_ch_1, &c1, &pio_inst->txf[pio_sm], audio_buffer_1, AUDIO_FRAMES, false);

    dma_channel_set_irq0_enabled(dma_ch_0, true);
    dma_channel_set_irq0_enabled(dma_ch_1, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_irq_handler);
    irq_set_enabled(DMA_IRQ_0, true);
}

void audio_play_test_tone(uint32_t freq) {
    if (is_playing) audio_stop();

    play_test_tone = true;
    tone_freq = freq;
    tone_phase = 0;
    wav_sample_rate = 44100;
    wav_channels = 2;

    uint32_t sys_clk = clock_get_hz(clk_sys);
    uint32_t bclk_freq = wav_sample_rate * 2 * 16; 
    float div = (float)sys_clk / (bclk_freq * 2.0f);
    pio_sm_set_clkdiv(pio_inst, pio_sm, div);

    gpio_put(I2S_AMP_SD, 1); 
    is_playing = true;

    fill_buffer(audio_buffer_0);
    fill_buffer(audio_buffer_1);

    buffer_0_needs_fill = false;
    buffer_1_needs_fill = false;

    dma_channel_set_read_addr(dma_ch_0, audio_buffer_0, false);
    dma_channel_set_trans_count(dma_ch_0, AUDIO_FRAMES, false);
    dma_channel_set_read_addr(dma_ch_1, audio_buffer_1, false);
    dma_channel_set_trans_count(dma_ch_1, AUDIO_FRAMES, false);

    pio_sm_set_enabled(pio_inst, pio_sm, true);
    dma_channel_start(dma_ch_0); 
}

bool audio_play_wav(const char *filename) {
    if (is_playing) audio_stop();
    play_test_tone = false;

    printf("\n[AUDIO] Deschidere: %s\n", filename);

    FRESULT fr = f_open(&current_wav, filename, FA_READ);
    if (fr != FR_OK) {
        printf("[AUDIO ERR] Failed f_open: %d\n", fr);
        return false;
    }

    // Funcția parse va încărca acum fișierul complet în RAM
    if (!parse_wav_header(&current_wav)) {
        printf("[AUDIO ERR] Failed parse_wav_header. Closing file.\n");
        // E posibil ca funcția să o fi închis deja, dar mai facem un safety check
        f_close(&current_wav); 
        return false;
    }

    uint32_t sys_clk = clock_get_hz(clk_sys);
    uint32_t bclk_freq = wav_sample_rate * 2 * 16; 
    float div = (float)sys_clk / (bclk_freq * 2.0f);
    pio_sm_set_clkdiv(pio_inst, pio_sm, div);

    gpio_put(I2S_AMP_SD, 1);
    is_playing = true;

    // Umplem cele două buffere DMA pentru prima dată (din RAM)
    fill_buffer(audio_buffer_0);
    fill_buffer(audio_buffer_1);

    buffer_0_needs_fill = false;
    buffer_1_needs_fill = false;

    // Setăm parametrii corecți pentru plecarea inițială
    dma_channel_set_read_addr(dma_ch_0, audio_buffer_0, false);
    dma_channel_set_trans_count(dma_ch_0, AUDIO_FRAMES, false);
    dma_channel_set_read_addr(dma_ch_1, audio_buffer_1, false);
    dma_channel_set_trans_count(dma_ch_1, AUDIO_FRAMES, false);

    pio_sm_set_enabled(pio_inst, pio_sm, true);
    dma_channel_start(dma_ch_0);

    return true;
}

void audio_stop() {
    is_playing = false;
    play_test_tone = false;
    
    dma_channel_abort(dma_ch_0);
    dma_channel_abort(dma_ch_1);
    
    pio_sm_set_enabled(pio_inst, pio_sm, false);
    pio_sm_clear_fifos(pio_inst, pio_sm);
    pio_sm_restart(pio_inst, pio_sm);
    pio_sm_exec(pio_inst, pio_sm, pio_encode_jmp(pio_offset));
    
    gpio_put(I2S_AMP_SD, 0); 
    
    // Deoarece închidem fișierul la sfârșitul încărcării în RAM, 
    // nu mai avem nevoie să-l închidem aici dacă a rulat cu succes, 
    // dar îl lăsăm pt fallback în cazul test_tone-ului
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